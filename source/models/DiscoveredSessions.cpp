/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "DiscoveredSessions.hpp"

DiscoveredSessions::DiscoveredSessions (rav::RavennaNode& ravennaNode) : node_ (ravennaNode)
{
    node_.subscribe (this).wait();
}

DiscoveredSessions::~DiscoveredSessions()
{
    node_.unsubscribe (this).wait();
}

bool DiscoveredSessions::addSubscriber (Subscriber* subscriber)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (subscribers_.add (subscriber))
    {
        for (const auto& session : sessions_)
            subscriber->onSessionUpdated (session.name, &session);
        for (const auto& node : nodes_)
            subscriber->onNodeUpdated (node.name, &node);
        return true;
    }
    return false;
}

bool DiscoveredSessions::removeSubscriber (Subscriber* subscriber)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return subscribers_.remove (subscriber);
}

void DiscoveredSessions::ravenna_session_discovered (const rav::dnssd::ServiceDescription& desc)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    executor_.callAsync ([this, desc] {
        if (auto* session = findSession (desc.name))
        {
            session->host = desc.host_target;
            for (auto* subscriber : subscribers_)
                subscriber->onSessionUpdated (desc.name, session);
            return;
        }

        const auto& it = sessions_.emplace_back (SessionState { desc.name, desc.host_target });
        for (auto* subscriber : subscribers_)
            subscriber->onSessionUpdated (desc.name, &it);
    });
}

void DiscoveredSessions::ravenna_session_removed (const rav::dnssd::ServiceDescription& desc)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    executor_.callAsync ([this, desc] {
        for (auto it = sessions_.begin(); it != sessions_.end(); ++it)
        {
            if (it->name == desc.name)
            {
                sessions_.erase (it);
                for (auto* subscriber : subscribers_)
                    subscriber->onSessionUpdated (desc.name, nullptr);
                return;
            }
        }
    });
}

void DiscoveredSessions::ravenna_node_discovered (const rav::dnssd::ServiceDescription& desc)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    executor_.callAsync ([this, desc] {
        if (auto* node = findNode (desc.name))
        {
            node->host = desc.host_target;
            for (auto* subscriber : subscribers_)
                subscriber->onNodeUpdated (desc.name, node);
            return;
        }

        const auto& it = nodes_.emplace_back (NodeState { desc.name, desc.host_target });
        for (auto* subscriber : subscribers_)
            subscriber->onNodeUpdated (desc.name, &it);
    });
}

void DiscoveredSessions::ravenna_node_removed (const rav::dnssd::ServiceDescription& desc)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    executor_.callAsync ([this, desc] {
        for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
        {
            if (it->name == desc.name)
            {
                nodes_.erase (it);
                for (auto* subscriber : subscribers_)
                    subscriber->onNodeUpdated (desc.name, nullptr);
                return;
            }
        }
    });
}

DiscoveredSessions::SessionState* DiscoveredSessions::findSession (const std::string& sessionName)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (auto& session : sessions_)
        if (session.name == sessionName)
            return &session;
    return nullptr;
}

DiscoveredSessions::NodeState* DiscoveredSessions::findNode (const std::string& nodeName)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (auto& node : nodes_)
        if (node.name == nodeName)
            return &node;
    return nullptr;
}
