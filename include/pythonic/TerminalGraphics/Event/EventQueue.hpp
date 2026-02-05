/**
 * @file EventQueue.hpp
 * @brief Thread-safe event queue for collecting and processing events
 */

#pragma once

#include "../Config.hpp"
#include "Event.hpp"
#include <queue>
#include <mutex>

TG_NAMESPACE_BEGIN

/**
 * @brief Thread-safe queue for events
 *
 * Collects events from the input thread and allows polling from the main thread.
 */
class EventQueue
{
public:
    /**
     * @brief Push an event onto the queue
     * @param event The event to add
     *
     * Thread-safe. Can be called from any thread.
     */
    void push(const Event &event)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(event);
    }

    /**
     * @brief Pop and return an event
     * @param event Output parameter for the event
     * @return true if an event was available, false if queue was empty
     */
    bool poll(Event &event)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;

        event = m_queue.front();
        m_queue.pop();
        return true;
    }

    /**
     * @brief Peek at the front event without removing it
     * @param event Output parameter for the event
     * @return true if an event was available
     */
    bool peek(Event &event) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty())
            return false;

        event = m_queue.front();
        return true;
    }

    /**
     * @brief Check if the queue is empty
     */
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
     * @brief Get the number of pending events
     */
    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    /**
     * @brief Clear all pending events
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        while (!m_queue.empty())
            m_queue.pop();
    }

private:
    std::queue<Event> m_queue;
    mutable std::mutex m_mutex;
};

TG_NAMESPACE_END
