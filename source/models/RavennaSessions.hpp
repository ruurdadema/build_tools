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

class RavennaSessions : public rav::RavennaNode::Subscriber
{
public:
    struct SessionState
    {
        std::string name;
        std::string host;
    };

    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;
        virtual void onSessionUpdated ([[maybe_unused]] const std::string& sessionName, [[maybe_unused]] const SessionState* state) {}
    };

    explicit RavennaSessions (rav::RavennaNode& ravennaNode);
    ~RavennaSessions() override;

    bool addSubscriber (Subscriber* subscriber);
    bool removeSubscriber (Subscriber* subscriber);

    // rav::ravenna_node::subscriber overrides
    void ravenna_session_discovered (const rav::dnssd::Browser::ServiceResolved& event) override;
    void ravenna_session_removed (const rav::dnssd::Browser::ServiceRemoved& event) override;

private:
    rav::RavennaNode& node_;
    rav::SubscriberList<Subscriber> subscribers_;
    std::vector<SessionState> sessions_;
    MessageThreadExecutor executor_;

    SessionState* findSession (const std::string& sessionName);
};
