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

#include "DiscoveredSessionsModel.hpp"

DiscoveredSessionsModel::DiscoveredSessionsModel (rav::RavennaNode& ravennaNode) : node_ (ravennaNode)
{
    node_.subscribe (this).wait();
}

DiscoveredSessionsModel::~DiscoveredSessionsModel()
{
    node_.unsubscribe (this).wait();
}

bool DiscoveredSessionsModel::addSubscriber (Subscriber* subscriber)
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

bool DiscoveredSessionsModel::removeSubscriber (const Subscriber* subscriber)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return subscribers_.remove (subscriber);
}

void DiscoveredSessionsModel::ravenna_session_discovered (const rav::dnssd::ServiceDescription& desc)
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

void DiscoveredSessionsModel::ravenna_session_removed (const rav::dnssd::ServiceDescription& desc)
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

void DiscoveredSessionsModel::ravenna_node_discovered (const rav::dnssd::ServiceDescription& desc)
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

void DiscoveredSessionsModel::ravenna_node_removed (const rav::dnssd::ServiceDescription& desc)
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

DiscoveredSessionsModel::SessionState* DiscoveredSessionsModel::findSession (const std::string& sessionName)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (auto& session : sessions_)
        if (session.name == sessionName)
            return &session;
    return nullptr;
}

DiscoveredSessionsModel::NodeState* DiscoveredSessionsModel::findNode (const std::string& nodeName)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (auto& node : nodes_)
        if (node.name == nodeName)
            return &node;
    return nullptr;
}
