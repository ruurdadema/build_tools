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
#include "ravennakit/core/containers/detail/stl_helpers.hpp"

#include <bitset>

SendersContainer::SendersContainer (ApplicationContext& context) : context_ (context)
{
    addButton.onClick = [this] {
        std::ignore = context_.getAudioSenders().createSender();
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

    txPortLabel_.setText ("TX Port:", juce::dontSendNotification);
    txPortLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (txPortLabel_);

    txPortComboBox_.addItem ("Primary", 0b01);   // Interface 1
    txPortComboBox_.addItem ("SPS", 0b11);       // Interface 1 & 2
    txPortComboBox_.addItem ("Secondary", 0b10); // Interface 2
    txPortComboBox_.onChange = [this] {
        const auto selectedId = txPortComboBox_.getSelectedId();
        if (selectedId <= 0)
            return;

        auto destinations = senderState_.senderConfiguration.destinations;

        RAV_ASSERT (destinations.size() == 2, "Expected two destinations");

        std::bitset<2> ports (static_cast<size_t> (selectedId));

        for (auto& dst : destinations)
        {
            RAV_ASSERT (dst.interface_by_rank.value() < 2, "Invalid interface rank");
            dst.enabled = ports[dst.interface_by_rank.value()];
        }

        rav::RavennaSender::ConfigurationUpdate update {};
        update.destinations = std::move (destinations);
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
    };
    addAndMakeVisible (txPortComboBox_);

    primaryAddressLabel_.setText ("Primary address:", juce::dontSendNotification);
    primaryAddressLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (primaryAddressLabel_);

    primaryAddressEditor_.setIndents (8, 8);
    primaryAddressEditor_.onReturnKey = [this] {
        auto destinations = senderState_.senderConfiguration.destinations;

        const auto it = std::find_if (
            destinations.begin(),
            destinations.end(),
            [] (const rav::RavennaSender::Destination& dst) {
                return dst.interface_by_rank == rav::Rank::primary();
            });

        if (it == destinations.end())
        {
            RAV_ERROR ("Primary destination not found");
            return;
        }

        if (primaryAddressEditor_.isEmpty())
        {
            it->endpoint.address (asio::ip::address_v4());
        }
        else
        {
            const auto addr = asio::ip::make_address_v4 (primaryAddressEditor_.getText().toRawUTF8());
            it->endpoint.address (addr);
        }

        rav::RavennaSender::ConfigurationUpdate update;
        update.destinations = std::move (destinations);
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
        juce::TextEditor::unfocusAllComponents();
    };
    primaryAddressEditor_.onEscapeKey = [this] {
        primaryAddressEditor_.setText (
            getDestinationAddress (rav::Rank::primary()).to_string(),
            juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    primaryAddressEditor_.onFocusLost = [this] {
        primaryAddressEditor_.setText (
            getDestinationAddress (rav::Rank::primary()).to_string(),
            juce::dontSendNotification);
    };
    addAndMakeVisible (primaryAddressEditor_);

    secondaryAddressLabel_.setText ("Secondary address:", juce::dontSendNotification);
    secondaryAddressLabel_.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (secondaryAddressLabel_);

    secondaryAddressEditor_.setIndents (8, 8);
    secondaryAddressEditor_.onReturnKey = [this] {
        auto destinations = senderState_.senderConfiguration.destinations;

        const auto it = std::find_if (
            destinations.begin(),
            destinations.end(),
            [] (const rav::RavennaSender::Destination& dst) {
                return dst.interface_by_rank == rav::Rank::secondary();
            });

        if (it == destinations.end())
        {
            RAV_ERROR ("Primary destination not found");
            return;
        }

        if (secondaryAddressEditor_.isEmpty())
        {
            it->endpoint.address (asio::ip::address_v4());
        }
        else
        {
            const auto addr = asio::ip::make_address_v4 (secondaryAddressEditor_.getText().toRawUTF8());
            it->endpoint.address (addr);
        }

        rav::RavennaSender::ConfigurationUpdate update;
        update.destinations = std::move (destinations);
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
        juce::TextEditor::unfocusAllComponents();
    };
    secondaryAddressEditor_.onEscapeKey = [this] {
        secondaryAddressEditor_.setText (
            getDestinationAddress (rav::Rank::secondary()).to_string(),
            juce::dontSendNotification);
        juce::TextEditor::unfocusAllComponents();
    };
    secondaryAddressEditor_.onFocusLost = [this] {
        secondaryAddressEditor_.setText (
            getDestinationAddress (rav::Rank::secondary()).to_string(),
            juce::dontSendNotification);
    };
    addAndMakeVisible (secondaryAddressEditor_);

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

    statusMessage_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (statusMessage_);

    onOffButton_.setClickingTogglesState (true);
    onOffButton_.setColour (juce::TextButton::ColourIds::buttonColourId, Constants::Colours::grey);
    onOffButton_.setColour (juce::TextButton::ColourIds::buttonOnColourId, Constants::Colours::green);
    onOffButton_.onClick = [this] {
        rav::RavennaSender::ConfigurationUpdate update;
        update.enabled = onOffButton_.getToggleState();
        audioSenders_.updateSenderConfiguration (senderId_, std::move (update));
    };
    addAndMakeVisible (onOffButton_);

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

    std::bitset<2> ports;

    for (auto& dst : state.senderConfiguration.destinations)
    {
        if (dst.interface_by_rank.value() >= ports.size())
            continue;
        ports[dst.interface_by_rank.value()] = dst.enabled;
    }

    txPortComboBox_.setSelectedId (static_cast<int> (ports.to_ulong()), juce::dontSendNotification);

    // Primary address editor
    primaryAddressEditor_.setText (
        getDestinationAddress (rav::Rank::primary()).to_string(),
        juce::dontSendNotification);
    primaryAddressEditor_.setEnabled (ports[0]);
    primaryAddressLabel_.setEnabled (ports[0]);

    // Secondary address editor
    secondaryAddressEditor_.setText (
        getDestinationAddress (rav::Rank::secondary()).to_string(),
        juce::dontSendNotification);
    secondaryAddressEditor_.setEnabled (ports[1]);
    secondaryAddressLabel_.setEnabled (ports[1]);

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
    onOffButton_.setToggleState (state.senderConfiguration.enabled, juce::dontSendNotification);
    onOffButton_.setButtonText (state.senderConfiguration.enabled ? "On" : "Off");

    // Update status message and colours:

    sampleRateComboBox_.setColour (
        juce::ComboBox::ColourIds::outlineColourId,
        findColour (juce::ComboBox::ColourIds::outlineColourId));

    numChannelsEditor_.setColour (
        juce::TextEditor::ColourIds::outlineColourId,
        findColour (juce::TextEditor::ColourIds::outlineColourId));

    if (!state.statusMessage.empty())
    {
        statusMessage_.setText (rav::string_to_upper (state.statusMessage, 1), juce::dontSendNotification);
        statusMessage_.setColour (juce::Label::ColourIds::textColourId, Constants::Colours::warning);
    }
    else if (state.senderConfiguration.audio_format.sample_rate != state.inputFormat.sample_rate)
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
    onOffButton_.setBounds (buttons.removeFromRight (65));

    auto row1 = b.removeFromTop (24);
    b.removeFromTop (kMargin / 2);
    auto row2 = b.removeFromTop (31);
    b.removeFromTop (kMargin);

    sessionNameLabel_.setBounds (row1.removeFromLeft (200));
    sessionNameEditor_.setBounds (row2.removeFromLeft (200));

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    numChannelsLabel_.setBounds (row1.removeFromLeft (80));
    numChannelsEditor_.setBounds (row2.removeFromLeft (80));

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    sampleRateLabel_.setBounds (row1.removeFromLeft (100));
    sampleRateComboBox_.setBounds (row2.removeFromLeft (100));

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    encodingLabel_.setBounds (row1.removeFromLeft (80));
    encodingComboBox_.setBounds (row2.removeFromLeft (80));

    // Next row

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    txPortLabel_.setBounds (row1.removeFromLeft (110));
    txPortComboBox_.setBounds (row2.removeFromLeft (110));

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    primaryAddressLabel_.setBounds (row1.removeFromLeft (130));
    primaryAddressEditor_.setBounds (row2.removeFromLeft (130));

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    secondaryAddressLabel_.setBounds (row1.removeFromLeft (130));
    secondaryAddressEditor_.setBounds (row2.removeFromLeft (130));

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    ttlLabel_.setBounds (row1.removeFromLeft (50));
    ttlEditor_.setBounds (row2.removeFromLeft (50));

    row1.removeFromLeft (kMargin);
    row2.removeFromLeft (kMargin);

    payloadTypeLabel_.setBounds (row1.removeFromLeft (100));
    payloadTypeEditor_.setBounds (row2.removeFromLeft (100));

    statusMessage_.setBounds (b);
}

asio::ip::address_v4 SendersContainer::Row::getDestinationAddress (const rav::Rank rank) const
{
    for (auto& dst : senderState_.senderConfiguration.destinations)
        if (dst.interface_by_rank == rank)
            return dst.endpoint.address().to_v4();
    return {};
}
