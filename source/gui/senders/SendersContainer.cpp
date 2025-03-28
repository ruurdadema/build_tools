/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "SendersContainer.hpp"

#include "gui/lookandfeel/Constants.hpp"

SendersContainer::SendersContainer (ApplicationContext& context) : context_ (context)
{
    addButton.onClick = [this] {
        std::ignore = context_.getAudioSenders().createSender ();
    };
    addAndMakeVisible (addButton);

    if (!context_.getAudioSenders().subscribe (this))
    {
        RAV_ERROR ("Failed to add subscriber");
    }

    resizeToFitContent();
}

SendersContainer::~SendersContainer()
{
    if (!context_.getAudioSenders().unsubscribe (this))
    {
        RAV_ERROR ("Failed to remove subscriber");
    }
}

void SendersContainer::resizeToFitContent()
{
    setSize (getWidth(), rows_.size() * kRowHeight + kMargin + kMargin * rows_.size() + kMargin * 2 + kButtonHeight);
}

void SendersContainer::onAudioSenderUpdated (rav::Id senderId, const AudioSenders::SenderState* state)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (state != nullptr)
    {
        for (const auto& row : rows_)
        {
            if (row->getId() == senderId)
            {
                row->update (*state);
                return;
            }
        }

        auto* row = rows_.add (std::make_unique<Row> (context_.getAudioSenders(), senderId));
        RAV_ASSERT (row != nullptr, "Failed to create row");
        row->update (*state);
        addAndMakeVisible (row);
        resizeToFitContent();
    }
    else
    {
        for (auto i = 0; i < rows_.size(); ++i)
        {
            if (rows_.getUnchecked (i)->getId() == senderId)
            {
                rows_.remove (i);
                resizeToFitContent();
                return;
            }
        }
    }
}

void SendersContainer::resized()
{
    auto b = getLocalBounds().reduced (kMargin);
    for (auto i = 0; i < rows_.size(); ++i)
    {
        rows_.getUnchecked (i)->setBounds (b.removeFromTop (kRowHeight));
        b.removeFromTop (kMargin);
    }

    b.removeFromTop (kMargin * 2);
    addButton.setBounds (b.withSizeKeepingCentre (200, kButtonHeight));
}

SendersContainer::Row::Row (AudioSenders& audioSenders, const rav::Id senderId) :
    audioSenders_ (audioSenders),
    senderId_ (senderId)
{
    sessionNameLabel_.setText ("Session name:", juce::dontSendNotification);
    sessionNameLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (sessionNameLabel_);

    sessionNameEditor_.setIndents (8, 8);
    sessionNameEditor_.onReturnKey = [this] {
        rav::RavennaSender::ConfigurationUpdate update;
        update.session_name = sessionNameEditor_.getText().toStdString();
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
        juce::TextEditor::unfocusAllComponents();
    };
    sessionNameEditor_.onEscapeKey = [this] {
        sessionNameEditor_.setText (senderState_.senderConfiguration.session_name, juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    sessionNameEditor_.onFocusLost = [this] {
        sessionNameEditor_.setText (senderState_.senderConfiguration.session_name, juce::dontSendNotification);
    };
    addAndMakeVisible (sessionNameEditor_);

    addressLabel_.setText ("Address:", juce::dontSendNotification);
    addressLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (addressLabel_);

    addressEditor_.setIndents (8, 8);
    addressEditor_.onReturnKey = [this] {
        rav::RavennaSender::ConfigurationUpdate update;
        update.destination_address = asio::ip::make_address_v4 (addressEditor_.getText().toRawUTF8());
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
        juce::TextEditor::unfocusAllComponents();
    };
    addressEditor_.onEscapeKey = [this] {
        addressEditor_.setText (
            senderState_.senderConfiguration.destination_address.to_string(),
            juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    addressEditor_.onFocusLost = [this] {
        addressEditor_.setText (
            senderState_.senderConfiguration.destination_address.to_string(),
            juce::dontSendNotification);
    };
    addAndMakeVisible (addressEditor_);

    ttlLabel_.setText ("TTL:", juce::dontSendNotification);
    ttlLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (ttlLabel_);

    ttlEditor_.setIndents (8, 8);
    ttlEditor_.setInputRestrictions (3, "0123456789");
    ttlEditor_.onReturnKey = [this] {
        rav::RavennaSender::ConfigurationUpdate update;
        update.ttl = ttlEditor_.getText().getIntValue();
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
        juce::TextEditor::unfocusAllComponents();
    };
    ttlEditor_.onEscapeKey = [this] {
        ttlEditor_.setText (juce::String (senderState_.senderConfiguration.ttl), juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    ttlEditor_.onFocusLost = [this] {
        ttlEditor_.setText (juce::String (senderState_.senderConfiguration.ttl), juce::dontSendNotification);
    };
    addAndMakeVisible (ttlEditor_);

    payloadTypeLabel_.setText ("Payload type:", juce::dontSendNotification);
    payloadTypeLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (payloadTypeLabel_);

    payloadTypeEditor_.setIndents (8, 8);
    payloadTypeEditor_.setInputRestrictions (3, "0123456789");
    payloadTypeEditor_.onReturnKey = [this] {
        rav::RavennaSender::ConfigurationUpdate update;
        update.payload_type = static_cast<uint8_t> (payloadTypeEditor_.getText().getIntValue());
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
        juce::TextEditor::unfocusAllComponents();
    };
    payloadTypeEditor_.onEscapeKey = [this] {
        payloadTypeEditor_.setText (
            juce::String (senderState_.senderConfiguration.payload_type),
            juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    payloadTypeEditor_.onFocusLost = [this] {
        payloadTypeEditor_.setText (
            juce::String (senderState_.senderConfiguration.payload_type),
            juce::dontSendNotification);
    };
    addAndMakeVisible (payloadTypeEditor_);

    numChannelsLabel_.setText ("Channels:", juce::dontSendNotification);
    numChannelsLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (numChannelsLabel_);

    numChannelsEditor_.setIndents (8, 8);
    numChannelsEditor_.setInputRestrictions (3, "0123456789");
    numChannelsEditor_.onReturnKey = [this] {
        rav::RavennaSender::ConfigurationUpdate update;
        update.audio_format = senderState_.senderConfiguration.audio_format;
        update.audio_format->num_channels = static_cast<uint32_t> (numChannelsEditor_.getText().getIntValue());
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
        juce::TextEditor::unfocusAllComponents();
    };
    numChannelsEditor_.onEscapeKey = [this] {
        numChannelsEditor_.setText (
            juce::String (senderState_.senderConfiguration.audio_format.num_channels),
            juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    numChannelsEditor_.onFocusLost = [this] {
        numChannelsEditor_.setText (
            juce::String (senderState_.senderConfiguration.audio_format.num_channels),
            juce::dontSendNotification);
    };
    addAndMakeVisible (numChannelsEditor_);

    sampleRateLabel_.setText ("Sample rate:", juce::dontSendNotification);
    sampleRateLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (sampleRateLabel_);

    sampleRateComboBox_.addItem ("44.1 kHz", 44100);
    sampleRateComboBox_.addItem ("48 kHz", 48000);
    sampleRateComboBox_.addItem ("96 kHz", 96000);
    sampleRateComboBox_.onChange = [this] {
        const auto selectedId = sampleRateComboBox_.getSelectedId();
        if (selectedId <= 0)
            return;
        rav::RavennaSender::ConfigurationUpdate update {};
        update.audio_format = senderState_.senderConfiguration.audio_format;
        update.audio_format->sample_rate = static_cast<uint32_t> (selectedId);
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
    };
    addAndMakeVisible (sampleRateComboBox_);

    encodingLabel_.setText ("Encoding:", juce::dontSendNotification);
    encodingLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (encodingLabel_);

    encodingComboBox_.addItem ("L16", static_cast<int> (rav::AudioEncoding::pcm_s16));
    encodingComboBox_.addItem ("L24", static_cast<int> (rav::AudioEncoding::pcm_s24));

    encodingComboBox_.onChange = [this] {
        const auto selectedId = encodingComboBox_.getSelectedId();
        if (selectedId <= 0)
            return;
        rav::RavennaSender::ConfigurationUpdate update {};
        update.audio_format = senderState_.senderConfiguration.audio_format;
        update.audio_format->encoding = static_cast<rav::AudioEncoding> (selectedId);
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
    };
    addAndMakeVisible (encodingComboBox_);

    statusMessage_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (statusMessage_);

    startStopButton_.setClickingTogglesState (true);
    startStopButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::green);
    startStopButton_.setColour (juce::TextButton::ColourIds::buttonOnColourId, Constants::Colours::yellow);
    startStopButton_.onClick = [this] {
        rav::RavennaSender::ConfigurationUpdate update;
        update.enabled = startStopButton_.getToggleState();
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
    };
    addAndMakeVisible (startStopButton_);

    deleteButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::red);
    deleteButton_.onClick = [this] {
        audioSenders_.removeSender (senderId_);
    };
    addAndMakeVisible (deleteButton_);
}

rav::Id SendersContainer::Row::getId() const
{
    return senderId_;
}

void SendersContainer::Row::update (const AudioSenders::SenderState& state)
{
    senderState_ = state;
    sessionNameEditor_.setText (state.senderConfiguration.session_name, juce::dontSendNotification);
    addressEditor_.setText (state.senderConfiguration.destination_address.to_string(), juce::dontSendNotification);
    ttlEditor_.setText (juce::String (state.senderConfiguration.ttl), juce::dontSendNotification);
    payloadTypeEditor_.setText (juce::String (state.senderConfiguration.payload_type), juce::dontSendNotification);
    numChannelsEditor_.setText (
        juce::String (state.senderConfiguration.audio_format.num_channels),
        juce::dontSendNotification);
    sampleRateComboBox_.setSelectedId (
        static_cast<int> (state.senderConfiguration.audio_format.sample_rate),
        juce::dontSendNotification);
    encodingComboBox_.setSelectedId (
        static_cast<int> (state.senderConfiguration.audio_format.encoding),
        juce::dontSendNotification);
    startStopButton_.setToggleState (state.senderConfiguration.enabled, juce::dontSendNotification);
    startStopButton_.setButtonText (state.senderConfiguration.enabled ? "Stop" : "Start");

    // Update status message and colours:

    sampleRateComboBox_.setColour (
        juce::ComboBox::ColourIds::outlineColourId,
        findColour (juce::ComboBox::ColourIds::outlineColourId));

    numChannelsEditor_.setColour (
        juce::TextEditor::ColourIds::outlineColourId,
        findColour (juce::TextEditor::ColourIds::outlineColourId));

    if (state.senderConfiguration.audio_format.sample_rate != state.inputFormat.sample_rate)
    {
        statusMessage_.setText (
            fmt::format ("Sample rate mismatch ({})", state.inputFormat.sample_rate),
            juce::dontSendNotification);
        statusMessage_.setColour (juce::Label::ColourIds::textColourId, Constants::Colours::warning);
        sampleRateComboBox_.setColour (juce::ComboBox::ColourIds::outlineColourId, Constants::Colours::warning);
    }
    else if (state.senderConfiguration.audio_format.num_channels != state.inputFormat.num_channels)
    {
        statusMessage_.setText (
            fmt::format ("Channel count mismatch ({})", state.inputFormat.num_channels),
            juce::dontSendNotification);
        statusMessage_.setColour (juce::Label::ColourIds::textColourId, Constants::Colours::warning);
        numChannelsEditor_.setColour (juce::TextEditor::ColourIds::outlineColourId, Constants::Colours::warning);
    }
    else
    {
        statusMessage_.setText ({}, juce::dontSendNotification);
    }
}

void SendersContainer::Row::paint (juce::Graphics& g)
{
    g.setColour (Constants::Colours::rowBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 5.0f);
}

void SendersContainer::Row::resized()
{
    auto b = getLocalBounds().reduced (kMargin);

    auto buttons = b.withTrimmedTop (b.getHeight() - kButtonHeight);
    deleteButton_.setBounds (buttons.removeFromRight (65));
    buttons.removeFromRight (kMargin);
    startStopButton_.setBounds (buttons.removeFromRight (65));

    auto topRow = b.removeFromTop (24);
    b.removeFromTop (kMargin / 2);
    auto bottomRow = b;

    sessionNameLabel_.setBounds (topRow.removeFromLeft (200));
    sessionNameEditor_.setBounds (bottomRow.removeFromLeft (200));

    topRow.removeFromLeft (kMargin);
    bottomRow.removeFromLeft (kMargin);

    addressLabel_.setBounds (topRow.removeFromLeft (160));
    addressEditor_.setBounds (bottomRow.removeFromLeft (160));

    topRow.removeFromLeft (kMargin);
    bottomRow.removeFromLeft (kMargin);

    ttlLabel_.setBounds (topRow.removeFromLeft (50));
    ttlEditor_.setBounds (bottomRow.removeFromLeft (50));

    topRow.removeFromLeft (kMargin);
    bottomRow.removeFromLeft (kMargin);

    payloadTypeLabel_.setBounds (topRow.removeFromLeft (100));
    payloadTypeEditor_.setBounds (bottomRow.removeFromLeft (100));

    topRow.removeFromLeft (kMargin);
    bottomRow.removeFromLeft (kMargin);

    numChannelsLabel_.setBounds (topRow.removeFromLeft (100));
    numChannelsEditor_.setBounds (bottomRow.removeFromLeft (100));

    topRow.removeFromLeft (kMargin);
    bottomRow.removeFromLeft (kMargin);

    sampleRateLabel_.setBounds (topRow.removeFromLeft (100));
    sampleRateComboBox_.setBounds (bottomRow.removeFromLeft (100));

    topRow.removeFromLeft (kMargin);
    bottomRow.removeFromLeft (kMargin);

    encodingLabel_.setBounds (topRow.removeFromLeft (100));
    encodingComboBox_.setBounds (bottomRow.removeFromLeft (100));

    statusMessage_.setBounds (topRow);
}
