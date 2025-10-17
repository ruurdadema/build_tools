/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "AudioSendersModel.hpp"

AudioSendersModel::AudioSendersModel (rav::RavennaNode& node) : node_ (node)
{
    node_.subscribe (this).wait();
    node_.subscribe_to_ptp_instance (&ptpSubscriber_).wait();
}

AudioSendersModel::~AudioSendersModel()
{
    for (const auto& sender : senders_)
        node_.unsubscribe_from_sender (sender->id, this).wait();
    node_.unsubscribe_from_ptp_instance (&ptpSubscriber_).wait();
    node_.unsubscribe (this).wait();
}

tl::expected<rav::Id, std::string> AudioSendersModel::createSender() const
{
    rav::RavennaSender::Configuration config;
    config.session_name = {}; // Can be left empty in which case RavennaNode will generate a name.
    config.ttl = 15;
    config.packet_time = rav::aes67::PacketTime::ms_1();
    config.payload_type = 98;

    config.audio_format.byte_order = rav::AudioFormat::ByteOrder::be;
    config.audio_format.ordering = rav::AudioFormat::ChannelOrdering::interleaved;
    config.audio_format.sample_rate = 48'000;
    config.audio_format.num_channels = 2;
    config.audio_format.encoding = rav::AudioEncoding::pcm_s16;

    config.destinations.emplace_back (
        rav::RavennaSender::Destination { rav::Rank::primary(), { boost::asio::ip::address_v4::any(), 5004 }, true });
    config.destinations.emplace_back (
        rav::RavennaSender::Destination { rav::Rank::secondary(), { boost::asio::ip::address_v4::any(), 5004 }, true });

    return node_.create_sender (config).get();
}

void AudioSendersModel::removeSender (const rav::Id senderId) const
{
    return node_.remove_sender (senderId).wait();
}

void AudioSendersModel::updateSenderConfiguration (const rav::Id senderId, rav::RavennaSender::Configuration config) const
{
    // Override audio format
    config.audio_format.byte_order = rav::AudioFormat::ByteOrder::be;
    config.audio_format.ordering = rav::AudioFormat::ChannelOrdering::interleaved;
    // Don't override num_channels, sample_rate and encoding

    auto result = node_.update_sender_configuration (senderId, std::move (config)).get();
    if (!result)
    {
        RAV_LOG_ERROR ("Failed to update sender configuration: {}", result.error());
    }
}

bool AudioSendersModel::subscribe (Subscriber* subscriber)
{
    if (subscribers_.add (subscriber))
    {
        for (const auto& sender : senders_)
            subscriber->onAudioSenderUpdated (sender->id, &sender->state);
        return true;
    }
    return false;
}

bool AudioSendersModel::unsubscribe (const Subscriber* subscriber)
{
    return subscribers_.remove (subscriber);
}

void AudioSendersModel::ravenna_sender_added (const rav::RavennaSender& sender)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, senderId = sender.get_id()] {
        RAV_ASSERT (findSender (senderId) == nullptr, "Sender already exists");
        const auto& it = senders_.emplace_back (std::make_unique<Sender>());
        it->id = senderId;
        node_.subscribe_to_sender (senderId, this).wait();
        senderPrepareInput (*it, deviceFormat_);
        updateRealtimeSharedContext();
    });
}

void AudioSendersModel::ravenna_sender_removed (rav::Id sender_id)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, sender_id] {
        // No need to unsubscribe from the receiver (via ravenna_node), as the stream no longer exists at this point.
        for (auto it = senders_.begin(); it != senders_.end(); ++it)
        {
            if ((*it)->id == sender_id)
            {
                const std::unique_ptr<Sender> tmp = std::move (*it); // Keep alive until after the context is updated
                node_.unsubscribe_from_sender (tmp->id, this).wait();
                senders_.erase (it);
                updateRealtimeSharedContext();
                break;
            }
        }
        for (const auto& subscriber : subscribers_)
            subscriber->onAudioSenderUpdated (sender_id, nullptr);
    });
}

void AudioSendersModel::ravenna_sender_configuration_updated (
    const rav::Id sender_id,
    const rav::RavennaSender::Configuration& configuration)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);

    executor_.callAsync ([this, sender_id, configuration] {
        auto* sender = findSender (sender_id);
        if (sender == nullptr)
        {
            RAV_LOG_ERROR ("Sender with id {} not found", sender_id.value());
            return;
        }

        sender->state.senderConfiguration = configuration;
        for (auto* subscriber : subscribers_)
            subscriber->onAudioSenderUpdated (sender_id, &sender->state);
        senderUpdateRealtimeSharedContext (*sender);
    });
}

void AudioSendersModel::audioDeviceIOCallbackWithContext (
    const float* const* inputChannelData,
    const int numInputChannels,
    float* const* outputChannelData,
    const int numOutputChannels,
    const int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    std::ignore = numInputChannels;

    const auto& localClock = ptpSubscriber_.get_local_clock();
    auto ptpNow = localClock.now();

    TRACY_ZONE_SCOPED;

    // If time information is available, use that
    if (context.hostTimeNs != nullptr)
        ptpNow = localClock.get_adjusted_time (*context.hostTimeNs);

    // Clear output buffer
    rav::AudioBufferView (outputChannelData, static_cast<size_t> (numOutputChannels), static_cast<size_t> (numSamples)).clear();

    if (!deviceFormat_.is_valid())
        return;

    if (!localClock.is_calibrated())
        return;

    const auto rtpNow = ptpNow.to_rtp_timestamp32 (deviceFormat_.sample_rate);

    if (!rtpTs_.has_value())
    {
        if (!localClock.is_locked())
            return;
        rtpTs_ = rtpNow;
    }

    // Positive means audio device is ahead of the PTP clock, negative means behind
    const auto drift = rav::WrappingUint32 (rtpNow).diff (*rtpTs_);
    auto ratio = static_cast<double> (deviceFormat_.sample_rate) /
                 static_cast<double> (static_cast<int32_t> (deviceFormat_.sample_rate) + drift);
    ratio = std::clamp (ratio, 0.5, 1.5);

    TRACY_PLOT ("Sender drift", static_cast<double> (drift));
    TRACY_PLOT ("Sender asrc ratio", ratio);

    const auto result = resampleProcess (
        resampler_.get(),
        inputChannelData,
        numSamples,
        resamplerBuffer_.data(),
        static_cast<int> (resamplerBuffer_.num_frames()),
        ratio);

    RAV_ASSERT_DEBUG (result.input_used == static_cast<uint32_t> (numSamples), "Num input frame mismatch");

    auto lock = realtimeSharedContext_.lock_realtime();

    for (auto* sender : lock->senders)
    {
        auto resampleBuffer = resamplerBuffer_.with_num_frames (result.output_generated).const_view();

        auto senderLock = sender->realtimeSharedContext_.lock_realtime();

        if (senderLock->targetSampleRate == 0)
            continue; // TODO: Can this check be deleted? And senderLock->targetSampleRate altogether?

        if (deviceFormat_.sample_rate == senderLock->targetSampleRate)
        {
            std::ignore = node_.send_audio_data_realtime (sender->id, resampleBuffer, *rtpTs_);
            continue;
        }

        if (senderLock->resampler == nullptr)
            continue; // Unexpected

        if (senderLock->rtpTimestamp == std::nullopt)
        {
            senderLock->rtpTimestamp = ptpNow.from_rtp_timestamp32 (*rtpTs_, deviceFormat_.sample_rate)
                                           .to_rtp_timestamp32 (senderLock->targetSampleRate);
        }

        // Check how much the timestamp at the input frequency differs from the one of the output frequency.
        const auto diff = static_cast<int64_t> (ptpNow.from_rtp_timestamp32 (*senderLock->rtpTimestamp, senderLock->targetSampleRate)
                                                    .to_rtp_timestamp32 (deviceFormat_.sample_rate)) -
                          static_cast<int64_t> (*rtpTs_);

        TRACY_PLOT ("Sender timestamp diff", diff);

        const auto resampleResult = resampleProcess (
            senderLock->resampler.get(),
            resampleBuffer.data(),
            static_cast<int> (resampleBuffer.num_frames()),
            senderLock->resampleBuffer.data(),
            static_cast<int> (senderLock->resampleBuffer.num_frames()),
            0.0);

        RAV_ASSERT_DEBUG (resampleResult.input_used == resampleBuffer.num_frames(), "Input used mismatch");

        std::ignore = node_.send_audio_data_realtime (
            sender->id,
            senderLock->resampleBuffer.with_num_frames (resampleResult.output_generated).const_view(),
            *senderLock->rtpTimestamp);

        *senderLock->rtpTimestamp += resampleResult.output_generated;
    }

    // sender->processBlock (resamplerBuffer_.with_num_frames (result.output_generated).const_view(), *rtpTs_, ptpNow);

    *rtpTs_ += result.output_generated;
}

void AudioSendersModel::audioDeviceAboutToStart (juce::AudioIODevice* device)
{
    deviceFormat_ = rav::AudioFormat {
        rav::AudioFormat::ByteOrder::le,
        rav::AudioEncoding::pcm_f32,
        rav::AudioFormat::ChannelOrdering::noninterleaved,
        static_cast<uint32_t> (device->getCurrentSampleRate()),
        static_cast<uint32_t> (device->getActiveInputChannels().countNumberOfSetBits()),
    };

    const auto numInputChannels = static_cast<size_t> (device->getActiveInputChannels().countNumberOfSetBits());

    maxNumFramesPerBlock_ = static_cast<uint32_t> (device->getCurrentBufferSizeSamples());
    resamplerBuffer_.resize (numInputChannels, maxNumFramesPerBlock_ * 2);

    resampler_.reset (resampleInit (static_cast<int> (deviceFormat_.num_channels), 256, 320, 1.0, SUBSAMPLE_INTERPOLATE | BLACKMAN_HARRIS));
    if (resampler_ == nullptr)
        RAV_LOG_ERROR ("Failed to initialize resampler");

    for (const auto& sender : senders_)
        senderPrepareInput (*sender, deviceFormat_);

    updateRealtimeSharedContext();
}

void AudioSendersModel::audioDeviceStopped()
{
    rtpTs_.reset();
    deviceFormat_ = {};
    resampler_.reset();
    maxNumFramesPerBlock_ = 0;

    for (const auto& sender : senders_)
    {
        sender->state.inputFormat = {};
        senderUpdateRealtimeSharedContext (*sender);
    }
}

void AudioSendersModel::audioDeviceError (const juce::String& errorMessage)
{
    RAV_LOG_ERROR ("Audio device error: {}", errorMessage.toStdString());
}

AudioSendersModel::Sender* AudioSendersModel::findSender (const rav::Id senderId) const
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (const auto& sender : senders_)
        if (sender->id == senderId)
            return sender.get();
    return nullptr;
}

void AudioSendersModel::updateRealtimeSharedContext()
{
    auto newContext = std::make_unique<RealtimeSharedContext>();
    for (const auto& sender : senders_)
        newContext->senders.push_back (sender.get());
    if (!realtimeSharedContext_.update (std::move (newContext)))
    {
        RAV_LOG_ERROR ("Failed to update realtime shared context");
    }
}

void AudioSendersModel::senderPrepareInput (Sender& sender, const rav::AudioFormat inputFormat)
{
    sender.state.inputFormat = inputFormat;

    for (auto* subscriber : subscribers_)
        subscriber->onAudioSenderUpdated (sender.id, &sender.state);

    senderUpdateRealtimeSharedContext (sender);
}

void AudioSendersModel::senderUpdateRealtimeSharedContext (Sender& sender) const
{
    if (!sender.state.senderConfiguration.enabled || !sender.state.inputFormat.is_valid() ||
        !sender.state.senderConfiguration.audio_format.is_valid() || maxNumFramesPerBlock_ == 0)
    {
        if (!sender.realtimeSharedContext_.reset())
        {
            RAV_LOG_ERROR ("Failed to update realtime shared context");
        }
        return;
    }

    auto newContext = std::make_unique<Sender::RealtimeSharedContext>();
    newContext->targetSampleRate = sender.state.senderConfiguration.audio_format.sample_rate;
    if (sender.state.inputFormat.sample_rate != sender.state.senderConfiguration.audio_format.sample_rate)
    {
        newContext->resampler.reset (resampleFixedRatioInit (
            static_cast<int> (sender.state.inputFormat.num_channels),
            256,
            320,
            sender.state.inputFormat.sample_rate,
            sender.state.senderConfiguration.audio_format.sample_rate,
            0,
            SUBSAMPLE_INTERPOLATE | BLACKMAN_HARRIS | INCLUDE_LOWPASS));
        const auto ratio = static_cast<double> (sender.state.senderConfiguration.audio_format.sample_rate) /
                           static_cast<double> (sender.state.inputFormat.sample_rate);
        newContext->resampleBuffer.resize (
            sender.state.inputFormat.num_channels,
            static_cast<uint32_t> (maxNumFramesPerBlock_ * ratio) + 8);
    }
    if (!sender.realtimeSharedContext_.update (std::move (newContext)))
    {
        RAV_LOG_ERROR ("Failed to update realtime shared context");
    }
}
