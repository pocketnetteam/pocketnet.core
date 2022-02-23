#ifndef POCKETCOIN_EVENTLOOP_H
#define POCKETCOIN_EVENTLOOP_H

#include <util.h>
#include <logging.h>

#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <thread>
#include <optional>
#include <exception>


// TODO (losty-fur): interface for queue
/**
 * Thread safe queue class
 * 
 * @tparam T - queue entry
 */
template<class T>
class Queue
{
public:
    /**
    * Pop the next object from queue.
    * If queue is empty - blocks current thread until new value comes to queue
    * 
    * @param out - poped element
    * @return true if element was filled
    * @return false if element was not filled
    */
    bool GetNext(T& out)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!GetPostConditionCheck()) {
            return false;
        }
        if (m_queue.empty()) {
            m_cv.wait(lock);
        }
        if (m_queue.empty()) {
            // Just return false because if we are here - queue waiting was interrupted and wi need to unblock waiting threads.
            // False indicates that there is no out value and thread can call GetNext() again if it was not expected to interrupt.
            return false;
        }
        if (!GetPostConditionCheck()) {
            return false;
        }
        out = std::forward<T>(m_queue.front());
        m_queue.pop();
        return true;
    }

    bool Add(T entry)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!AddConditionCheck()) {
                return false;
            }
            m_queue.push(std::forward<T>(entry));
        }
        m_cv.notify_one();
        return true;
    }
    void Interrupt()
    {
        // This just simply unblocks all threads that are waiting for value.
        // If there are multiple threads working with a single queue this will have the following workflow:
        // 1) All threads that are waiting in GetNext() will be unblocked.
        // 2) Threads that are not going to be stopped will again call GetNext() and continue waiting for value
        // 3) Thread that calls this method and want to stop will be unblocked and able to end queue processing
        //    by not call GetNext() again.
        m_cv.notify_all();
    }

    size_t Size()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return _Size();
    }
protected:
    // Override following methods to define special queue restrictions, e.x. max queue length.
    virtual bool AddConditionCheck() {
        return true;
    }
    virtual bool GetPreconditionCheck() {
        return true;
    }
    virtual bool GetPostConditionCheck() {
        return true;
    }
    size_t _Size()
    {
        return m_queue.size();
    }
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};

template<class T>
class QueueLimited : public Queue<T>
{
public:
    QueueLimited(size_t _maxDepth)
        : m_maxDepth(std::move(_maxDepth))
    {}
protected:
    bool AddConditionCheck() override
    {
        return this->_Size() < m_maxDepth;
    }   
private:
    size_t m_maxDepth;
};

/**
 * Queue processor that will be called on every queue element
 * in an event loop. Concrete processor should inherit this interface and DI into
 * QueueEventLoopThread so event loop will use it ti process new queue entries.
 * 
 * @tparam T 
 */
template<class T>
class IQueueProcessor {
public:
    virtual void Process(T entry) = 0;
};

/**
 * Functional based queue processor that accepts functor in ctor and simply delegates all
 * entry procession to it
 * 
 * @tparam T 
 */
template<class T>
class FunctionalBaseQueueProcessor : public IQueueProcessor<T>
{
public:
    explicit FunctionalBaseQueueProcessor(std::function<void(T)> func)
        : m_func(std::move(func))
    {}

    void Process(T entry) override
    {
        m_func(std::forward<T>(entry));
    }
private:
    std::function<void(T)> m_func;
};

template<class T>
class QueueEventLoopThread
{
public:
    QueueEventLoopThread(std::shared_ptr<Queue<T>> queue, std::shared_ptr<IQueueProcessor<T>> queueProcessor) {
        m_queue = std::move(queue);
        m_queueProcessor = std::move(queueProcessor);
    }

    void Start(std::optional<std::string> name = std::nullopt)
    {
        m_fRunning = true;
        m_thread = std::thread([name, &fRunning = m_fRunning, queue = m_queue, queueProcessor = m_queueProcessor](){
            if (name) {
                RenameThread(name->c_str());
            }
            while (fRunning) {
                try {
                    T entry;
                    auto res = queue->GetNext(entry);
                    // If res is false - someone else interrupts queue and if current thread still wants to run just call GetNext() again
                    if (res && fRunning) {
                        queueProcessor->Process(std::forward<T>(entry));
                    }
                } catch (const std::exception& e) {
                    fRunning = false;
                    LogPrintf("Shutting down %s event loop thread because of exception: %s", name.value_or(""), e.what());
                }
            }
        });
    }

    /**
     * Stopping and joining current thread.
     * Affects only current thread and still allows other threads process queue
     */
    void Stop()
    {
        if (m_fRunning) {
            m_fRunning = false;
            m_queue->Interrupt();
        }
        // Try join anyway because otherwise there could be a situation when thread is stopped but not joined
        // that is a bad practise.
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    ~QueueEventLoopThread()
    {
        Stop();
    }

private:
    std::thread m_thread;
    std::shared_ptr<Queue<T>> m_queue;
    std::atomic_bool m_fRunning = true;
    std::shared_ptr<IQueueProcessor<T>> m_queueProcessor;
};

#endif // POCKETCOIN_EVENTLOOP_H