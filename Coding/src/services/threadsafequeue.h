/* -------------------------------------------------------------------------
//  文件名      :  threadsafequeue.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Mutex-protected deque for cross-thread producer/consumer queues.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_THREADSAFEQUEUE_H__
#define __CLASSEXAM_THREADSAFEQUEUE_H__

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

template<typename T>
class KCTThreadSafeQueue
{
public:
    KCTThreadSafeQueue() = default;
    KCTThreadSafeQueue(const KCTThreadSafeQueue&) = delete;
    KCTThreadSafeQueue& operator=(const KCTThreadSafeQueue&) = delete;

    void push(T value)
    {
        std::scoped_lock lock(m_mutex);
        if (m_shutdown) {
            return;
        }
        m_queue.push_back(std::move(value));
        m_cv.notify_one();
    }

    // Blocks until items exist, shutdown, or timeout; pops all currently queued items into out.
    template<typename Rep, typename Period>
    void popAllWaitFor(std::vector<T>& out, const std::chrono::duration<Rep, Period>& timeout)
    {
        out.clear();
        std::unique_lock lock(m_mutex);
        m_cv.wait_for(lock, timeout, [this] {
            return !m_queue.empty() || m_shutdown;
        });
        while (!m_queue.empty()) {
            out.push_back(std::move(m_queue.front()));
            m_queue.pop_front();
        }
    }

    void drainAll(std::vector<T>& out)
    {
        out.clear();
        std::scoped_lock lock(m_mutex);
        while (!m_queue.empty()) {
            out.push_back(std::move(m_queue.front()));
            m_queue.pop_front();
        }
    }

    bool empty() const
    {
        std::scoped_lock lock(m_mutex);
        return m_queue.empty();
    }

    void shutdown()
    {
        {
            std::scoped_lock lock(m_mutex);
            m_shutdown = true;
        }
        m_cv.notify_all();
    }

    bool isShutdown() const
    {
        std::scoped_lock lock(m_mutex);
        return m_shutdown;
    }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<T> m_queue;
    bool m_shutdown = false;
};

#endif // __CLASSEXAM_THREADSAFEQUEUE_H__
