/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#pragma once

#include "ravennakit/ravenna/ravenna_node.hpp"
#include "util/MessageThreadExecutor.hpp"

#include <juce_audio_devices/juce_audio_devices.h>

class AudioSendersModel :
    public rav::RavennaNode::Subscriber,
    public juce::AudioIODeviceCallback,
    public rav::ptp::Instance::Subscriber
{
public:
    struct SenderState
    {
        rav::RavennaSender::Configuration senderConfiguration;
        rav::AudioFormat inputFormat;
    };

    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;

        /**
         * Called when a sender is updated. When a new subscriber is added, this method is called for all existing
         * senders.
         * @param senderId The id of the sender.
         * @param state The state of the sender, or nullptr if the sender was removed.
         */
        virtual void onAudioSenderUpdated (const rav::Id senderId, const SenderState* state)
        {
            std::ignore = senderId;
            std::ignore = state;
        }
    };

    explicit AudioSendersModel (rav::RavennaNode& node);
    ~AudioSendersModel() override;

    /**
     * Creates a sender.
     * @return A valid id of the newly created sender, or an invalid id on failure.
     */
    [[nodiscard]] tl::expected<rav::Id, std::string> createSender() const;

    /**
     * Removes a sender.
     * @param senderId The sender to remove.
     */
    void removeSender (rav::Id senderId) const;

    /**
     * Updates the configuration of a sender.
     * @param senderId The id of the sender to update.
     * @param config The configuration changes to apply.
     */
    void updateSenderConfiguration (rav::Id senderId, rav::RavennaSender::Configuration config) const;

    /**
     * Adds a subscriber to the audio mixer.
     * @param subscriber The subscriber to add.
     * @return true if the subscriber was added, or false if it was already in the list.
     */
    [[nodiscard]] bool subscribe (Subscriber* subscriber);

    /**
     * Removes a subscriber from the audio mixer.
     * @param subscriber The subscriber to remove.
     * @return true if the subscriber was removed, or false if it was not in the list.
     */
    [[nodiscard]] bool unsubscribe (Subscriber* subscriber);

    // rav::RavennaNode::Subscriber overrides
    void ravenna_sender_added (const rav::RavennaSender& sender) override;
    void ravenna_sender_removed (rav::Id sender_id) override;

    // juce::AudioIODeviceCallback overrides
    void audioDeviceIOCallbackWithContext (
        const float* const* inputChannelData,
        int numInputChannels,
        float* const* outputChannelData,
        int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError (const juce::String& errorMessage) override;

private:
    class Sender : public rav::RavennaSender::Subscriber
    {
    public:
        Sender (AudioSendersModel& owner, rav::Id senderId);
        ~Sender() override;

        [[nodiscard]] rav::Id getSenderId() const;
        [[nodiscard]] const SenderState& getState() const;

        void ravenna_sender_configuration_updated (
            rav::Id sender_id,
            const rav::RavennaSender::Configuration& configuration) override;

        void prepareInput (rav::AudioFormat inputFormat, uint32_t max_num_frames);
        void processBlock (const rav::AudioBufferView<const float>& inputBuffer, uint32_t timestamp) const;

    private:
        [[maybe_unused]] AudioSendersModel& owner_;
        rav::Id senderId_;
        SenderState state_;
        MessageThreadExecutor executor_; // Keep last so that it's destroyed first
    };

    struct RealtimeSharedContext
    {
        std::vector<Sender*> senders;
        std::optional<uint32_t> rtp_ts {};
        rav::AudioFormat deviceFormat;
    };

    rav::RavennaNode& node_;
    rav::AudioFormat deviceFormat_;
    std::vector<std::unique_ptr<Sender>> senders_;
    rav::SubscriberList<Subscriber> subscribers_;
    rav::RealtimeSharedObject<RealtimeSharedContext> realtimeSharedContext_;
    MessageThreadExecutor executor_; // Keep last so that it's destroyed first

    [[nodiscard]] Sender* findSender (rav::Id senderId) const;
    void updateRealtimeSharedContext();
};
