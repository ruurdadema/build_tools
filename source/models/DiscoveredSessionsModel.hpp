/*
 * Project: RAVENNAKIT demo application
 * Copyright (c) 2024-2025 Sound on Digital
 *
 * This file is part of the RAVENNAKIT demo application.
 *
 * The RAVENNAKIT demo application is licensed on a proprietary,
 * source-available basis. You are allowed to view, modify, and compile
 * this code for personal or internal evaluation only.
 *
 * You may NOT redistribute this file, any modified versions, or any
 * compiled binaries to third parties, and you may NOT use it for any
 * commercial purpose, except under a separate written agreement with
 * Sound on Digital.
 *
 * For the full license terms, see:
 *   LICENSE.md
 * or visit:
 *   https://ravennakit.com
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
