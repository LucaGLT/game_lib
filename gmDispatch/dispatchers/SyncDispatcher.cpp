#include "SyncDispatcher.hpp"

namespace gmDispatch {

SyncDispatcher::SyncDispatcher(std::unique_ptr<IRouter> router)
	: _router(std::move(router))
{}

void SyncDispatcher::dispatch(const Envelope& envelope)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	_router->route(envelope);
}

void SyncDispatcher::subscribe(const std::string&        typeId,
							   std::shared_ptr<IChannel> channel)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	_router->subscribe(typeId, std::move(channel));
}

void SyncDispatcher::unsubscribe(const std::string&        typeId,
								 std::shared_ptr<IChannel> channel)
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	_router->unsubscribe(typeId, channel);
}

void SyncDispatcher::flush()
{
	std::lock_guard<std::recursive_mutex> lock(_mutex);
	_router->flush();
}

} // namespace gmDispatch
