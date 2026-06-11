/**
 * @file dispatchers/AsyncDispatcher.cpp
 * @brief Implementation of AsyncDispatcher.
 */

#include "AsyncDispatcher.hpp"

namespace GmDispatch {

// ── Constructor / destructor ──────────────────────────────────────────────────

AsyncDispatcher::AsyncDispatcher(std::unique_ptr<IRouter> router)
    : router_(std::move(router))
    , worker_(&AsyncDispatcher::workerLoop, this)
{}

AsyncDispatcher::~AsyncDispatcher()
{
    // Signal the worker to stop after draining the queue.
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    queueCv_.notify_all();

    if (worker_.joinable()) {
        worker_.join();
    }

    // Flush all channels after the worker has stopped.
    std::lock_guard<std::mutex> rlock(routeMutex_);
    router_->flush();
}

// ── IDispatcher interface ─────────────────────────────────────────────────────

void AsyncDispatcher::dispatch(const Envelope& envelope)
{
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.push(envelope);
    }
    queueCv_.notify_one();
}

void AsyncDispatcher::subscribe(const std::string&        typeId,
                                std::shared_ptr<IChannel> channel)
{
    std::lock_guard<std::mutex> lock(routeMutex_);
    router_->subscribe(typeId, std::move(channel));
}

void AsyncDispatcher::unsubscribe(const std::string&        typeId,
                                  std::shared_ptr<IChannel> channel)
{
    std::lock_guard<std::mutex> lock(routeMutex_);
    router_->unsubscribe(typeId, channel);
}

void AsyncDispatcher::flush()
{
    // Wait until the queue is empty AND the worker is not currently routing.
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        drainCv_.wait(lock, [this] {
            return queue_.empty() && !workerBusy_;
        });
    }

    // Flush all channels (under routeMutex_ to avoid races with route()).
    std::lock_guard<std::mutex> rlock(routeMutex_);
    router_->flush();
}

// ── Worker thread ─────────────────────────────────────────────────────────────

void AsyncDispatcher::workerLoop()
{
    while (true) {
        Envelope env;

        // Wait for work or stop signal.
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait(lock, [this] {
                return !queue_.empty() || stop_;
            });

            if (stop_ && queue_.empty()) break;

            env         = std::move(queue_.front());
            queue_.pop();
            workerBusy_ = true;
        }

        // Route outside queueMutex_ but inside routeMutex_.
        {
            std::lock_guard<std::mutex> rlock(routeMutex_);
            router_->route(env);
        }

        // Mark as idle and wake any flush() waiters.
        {
            std::lock_guard<std::mutex> lock(queueMutex_);
            workerBusy_ = false;
        }
        drainCv_.notify_all();
    }
}

} // namespace GmDispatch
