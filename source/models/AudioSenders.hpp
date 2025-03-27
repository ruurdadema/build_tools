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

class AudioSenders : public rav::RavennaNode::Subscriber, public juce::AudioIODeviceCallback
{
public:
    struct SenderState
    {
        rav::RavennaSender::Configuration configuration;
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

    explicit AudioSenders (rav::RavennaNode& node);

    ~AudioSenders() override;

    /**
     * Creates a sender.
     * @param sessionName The session name to create the sender for. May be empty.
     * @return A valid id of the newly created sender, or an invalid id on failure.
     */
    [[nodiscard]] rav::Id createSender (const std::string& sessionName) const;

    /**
     * Removes a sender.
     * @param senderId The sender to remove.
     */
    void removeSender (rav::Id senderId) const;

    /**
     * Updates the configuration of a sender.
     * @param senderId The id of the sender to update.
     * @param update The configuration changes to apply.
     */
    void updateSenderConfiguration (rav::Id senderId, rav::RavennaSender::ConfigurationUpdate update) const;

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
        Sender (AudioSenders& owner, rav::Id senderId);
        ~Sender() override;

        [[nodiscard]] rav::Id getSenderId() const;
        [[nodiscard]] const SenderState& getState() const;

        void ravenna_sender_configuration_updated (
            rav::Id sender_id,
            const rav::RavennaSender::Configuration& configuration) override;

    private:
        [[maybe_unused]] AudioSenders& owner_;
        rav::Id senderId_;
        SenderState state_;
        MessageThreadExecutor executor_; // Keep last so that it's destroyed first to prevent dangling pointers
    };

    struct RealtimeSharedContext
    {
        std::vector<Sender*> senders;
    };

    rav::RavennaNode& node_;
    rav::AudioFormat deviceFormat_;
    std::vector<std::unique_ptr<Sender>> senders_;
    rav::SubscriberList<Subscriber> subscribers_;
    rav::RealtimeSharedObject<RealtimeSharedContext> realtimeSharedContext_;
    MessageThreadExecutor executor_; // Keep last so that it's destroyed first to prevent dangling pointers

    [[nodiscard]] Sender* findSender (rav::Id senderId) const;
    void updateRealtimeSharedContext();
};
