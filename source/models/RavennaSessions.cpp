/*
 * Owllab License Agreement
 *
 * This software is provided by Owllab and may not be used, copied, modified,
 * merged, published, distributed, sublicensed, or sold without a valid and
 * explicit agreement with Owllab.
 *
 * Copyright (c) 2025 Owllab. All rights reserved.
 */

#include "RavennaSessions.hpp"

RavennaSessions::RavennaSessions (rav::ravenna_node& ravennaNode) : node_ (ravennaNode)
{
    node_.add_subscriber (this).wait();
}

RavennaSessions::~RavennaSessions()
{
    node_.remove_subscriber (this).wait();
}

bool RavennaSessions::addSubscriber (Subscriber* subscriber)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    if (subscribers_.add (subscriber))
    {
        for (const auto& session : sessions_)
            subscriber->onSessionUpdated (session.name, &session);
        return true;
    }
    return false;
}

bool RavennaSessions::removeSubscriber (Subscriber* subscriber)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    return subscribers_.remove (subscriber);
}

void RavennaSessions::ravenna_session_discovered (const rav::dnssd::dnssd_browser::service_resolved& event)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    executor_.callAsync ([this, desc = event.description] {
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

void RavennaSessions::ravenna_session_removed (const rav::dnssd::dnssd_browser::service_removed& event)
{
    RAV_ASSERT_NODE_MAINTENANCE_THREAD (node_);
    executor_.callAsync ([this, desc = event.description] {
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

RavennaSessions::SessionState* RavennaSessions::findSession (const std::string& sessionName)
{
    JUCE_ASSERT_MESSAGE_THREAD;
    for (auto& session : sessions_)
    {
        if (session.name == sessionName)
            return &session;
    }
    return nullptr;
}
