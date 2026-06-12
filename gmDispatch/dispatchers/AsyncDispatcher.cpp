/**
 * @file dispatchers/AsyncDispatcher.cpp
 * @brief Implementation of AsyncDispatcher.
 */

#include "AsyncDispatcher.hpp"

namespace gmDispatch {

// ── Constructor / destructor ──────────────────────────────────────────────────

AsyncDispatcher::AsyncDispatcher(std::unique_ptr<IRouter> router)
	: _router(std::move(router))
	, _worker(&AsyncDispatcher::worker_loop, this)
{}

AsyncDispatcher::~AsyncDispatcher()
{
	// Signal the worker to stop after draining the queue.
	{
		std::lock_guard<std::mutex> lock(_queue_mutex);
		_stop = true;
	}
	_queue_cv.notify_all();

	if (_worker.joinable()) {
		_worker.join();
	}

	// Flush all channels after the worker has stopped.
	std::lock_guard<std::mutex> rlock(_route_mutex);
	_router->flush();
}

// ── IDispatcher interface ─────────────────────────────────────────────────────

void AsyncDispatcher::dispatch(const Envelope& envelope)
{
	{
		std::lock_guard<std::mutex> lock(_queue_mutex);
		_queue.push(envelope);
	}
	_queue_cv.notify_one();
}

void AsyncDispatcher::subscribe(const std::string&        typeId,
								std::shared_ptr<IChannel> channel)
{
	std::lock_guard<std::mutex> lock(_route_mutex);
	_router->subscribe(typeId, std::move(channel));
}

void AsyncDispatcher::unsubscribe(const std::string&        typeId,
								  std::shared_ptr<IChannel> channel)
{
	std::lock_guard<std::mutex> lock(_route_mutex);
	_router->unsubscribe(typeId, channel);
}

void AsyncDispatcher::flush()
{
	// Wait until the queue is empty AND the worker is not currently routing.
	{
		std::unique_lock<std::mutex> lock(_queue_mutex);
		_drain_cv.wait(lock, [this]
		{
			return _queue.empty() && !_worker_busy;
		});
	}

	// Flush all channels (under _route_mutex to avoid races with route()).
	std::lock_guard<std::mutex> rlock(_route_mutex);
	_router->flush();
}

// ── Worker thread ─────────────────────────────────────────────────────────────

void AsyncDispatcher::worker_loop()
{
	while (true)
	{
		Envelope env;

		// Wait for work or stop signal.
		{
			std::unique_lock<std::mutex> lock(_queue_mutex);
			_queue_cv.wait(lock, [this]
			{
				return !_queue.empty() || _stop;
			});

			if (_stop && _queue.empty()) break;

			env         = std::move(_queue.front());
			_queue.pop();
			_worker_busy = true;
		}

		// Route outside _queue_mutex but inside _route_mutex.
		{
			std::lock_guard<std::mutex> rlock(_route_mutex);
			_router->route(env);
		}

		// Mark as idle and wake any flush() waiters.
		{
			std::lock_guard<std::mutex> lock(_queue_mutex);
			_worker_busy = false;
		}
		_drain_cv.notify_all();
	}
}

} // namespace gmDispatch
