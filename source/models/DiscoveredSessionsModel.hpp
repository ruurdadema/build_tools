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

class DiscoveredSessionsModel : public rav::RavennaNode::Subscriber
{
public:
    struct NodeState
    {
        std::string name;
        std::string host;
    };

    struct SessionState
    {
        std::string name;
        std::string host;
    };

    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;
        virtual void onSessionUpdated (const std::string& sessionName, const SessionState* state)
        {
            std::ignore = sessionName;
            std::ignore = state;
        }
        virtual void onNodeUpdated (const std::string& nodeName, const NodeState* state)
        {
            std::ignore = nodeName;
            std::ignore = state;
        }
    };

    explicit DiscoveredSessionsModel (rav::RavennaNode& ravennaNode);
    ~DiscoveredSessionsModel() override;

    bool addSubscriber (Subscriber* subscriber);
    bool removeSubscriber (const Subscriber* subscriber);

    // rav::ravenna_node::subscriber overrides
    void ravenna_session_discovered (const rav::dnssd::ServiceDescription& desc) override;
    void ravenna_session_removed (const rav::dnssd::ServiceDescription& desc) override;
    void ravenna_node_discovered (const rav::dnssd::ServiceDescription& desc) override;
    void ravenna_node_removed (const rav::dnssd::ServiceDescription& desc) override;

private:
    rav::RavennaNode& node_;
    rav::SubscriberList<Subscriber> subscribers_;
    std::vector<SessionState> sessions_;
    std::vector<NodeState> nodes_;
    MessageThreadExecutor executor_;

    SessionState* findSession (const std::string& sessionName);
    NodeState* findNode (const std::string& nodeName);
};
