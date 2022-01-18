#ifndef POCKETCOIN_EVENTLOOP_H
#define POCKETCOIN_EVENTLOOP_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <thread>

template<class T>
class Queue
{
public:
    bool GetNext(T& out)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_fRunning && m_queue.empty()) {
            m_cv.wait(lock);
        }
        if (!m_fRunning || m_queue.empty()) {
            return false;
        }
        out = std::forward<T>(m_queue.front());
        m_queue.pop();
        return true;
    }

    void Add(T entry)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_queue.push(std::forward<T>(entry));
        }
        m_cv.notify_one();
    }
    void Interrupt()
    {
        m_fRunning = false;
        m_cv.notify_all();
    }
private:
    std::atomic_bool m_fRunning{true};
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

template<class T>
class IQueueProcessor {
public:
    virtual void Process(T entry) = 0;
};

template<class T>
class QueueEventLoopThread
{
public:
    QueueEventLoopThread(std::shared_ptr<Queue<T>> queue, std::shared_ptr<IQueueProcessor<T>> queueProcessor) {
        m_queue = std::move(queue);
        m_queueProcessor = std::move(queueProcessor);
    }

    void Start()
    {
        m_fRunning = true;
        m_thread = std::thread([&fRunning = m_fRunning, queue = m_queue, queueProcessor = m_queueProcessor](){
            while (fRunning) {
                T entry;
                auto res = queue->GetNext(entry);
                if (res) {
                    queueProcessor->Process(std::forward<T>(entry));
                }
            }
        });
    }

    void Stop()
    {
        m_fRunning = false;
        m_queue->Interrupt();
        m_thread.join();
    }

private:
    std::thread m_thread;
    std::shared_ptr<Queue<T>> m_queue;
    std::atomic_bool m_fRunning = true;
    std::shared_ptr<IQueueProcessor<T>> m_queueProcessor;
};

#endif // POCKETCOIN_EVENTLOOP_H