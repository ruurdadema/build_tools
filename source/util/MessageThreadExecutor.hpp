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

#include <juce_events/juce_events.h>

/**
 * Simple class which executes work using the JUCE message manager. The scheduled work will be tied to the lifetime of
 * this object, which avoids dealing with weak pointers etc.
 */
class MessageThreadExecutor : juce::AsyncUpdater
{
public:
    ~MessageThreadExecutor() override
    {
        cancelPendingUpdate();
    }

    /**
     * Schedules given function to be called from the message thread using the JUCE message manager. The function will
     * return immediately and the work will be executed some time in the future. It's safe to call this function from
     * any thread.
     * @param work The work to execute.
     */
    void callAsync (std::function<void()>&& work)
    {
        const std::lock_guard lock (mutex_);
        work_.push_back (std::move (work));
        triggerAsyncUpdate();
    }

    /**
     * Calls given work on the message thread using the JUCE message manager and waits for it to finish.
     * It's safe to call this function from any thread.
     * @param work The work to execute on the message thread.
     * @param timeOutMs The Maximum time to wait for the work to finish.
     * @return True if the work was finished, or false if not (possibly due to the timeout).
     */
    static bool callOnMessageThread (const std::function<void()>& work, double const timeOutMs = -1)
    {
        if (work == nullptr)
            return false;

        if (const auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        {
            if (mm->isThisTheMessageThread())
            {
                work(); // Execute the work directly since we're already on the message thread.
                return true;
            }
        }
        else
        {
            jassertfalse; // MessageManager is not running!
            return false;
        }

        const juce::WaitableEvent event;

        juce::MessageManager::callAsync ([&work, &event] {
            work();
            event.signal();
        });

        return event.wait (timeOutMs);
    }

private:
    std::mutex mutex_;
    std::vector<std::function<void()>> work_;

    void handleAsyncUpdate() override
    {
        const std::lock_guard lock (mutex_);
        for (auto& work : work_)
            if (work)
                work();
        work_.clear();
    }
};
