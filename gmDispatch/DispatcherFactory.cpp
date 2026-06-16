#include "DispatcherFactory.hpp"
#include "DispatcherConfig.hpp"
#include "dispatchers/SyncDispatcher.hpp"
#include "dispatchers/AsyncDispatcher.hpp"
#include "routers/SyncRouter.hpp"
#include "routers/PatternRouter.hpp"
#include "channels/StdoutChannel.hpp"

#include <memory>

namespace gmDispatch {

GmDispatcher DispatcherFactory::create_sync_dispatcher(const std::string& name,
												   bool               auto_timestamp)
{
	DispatcherConfig cfg;
	cfg.name          = name;
	cfg.auto_timestamp = auto_timestamp;

	return GmDispatcher(
		std::move(cfg),
		std::make_unique<SyncDispatcher>(
			std::make_unique<SyncRouter>()));
}

GmDispatcher DispatcherFactory::create_debug_dispatcher(const std::string& name)
{
	DispatcherConfig cfg;
	cfg.name          = name;
	cfg.auto_timestamp = true;

	std::unique_ptr<SyncDispatcher> impl =
		std::make_unique<SyncDispatcher>(std::make_unique<SyncRouter>());

	impl->subscribe("*", std::make_shared<StdoutChannel>());

	return GmDispatcher(std::move(cfg), std::move(impl));
}

GmDispatcher DispatcherFactory::create_async_dispatcher(const std::string& name,
													bool               auto_timestamp)
{
	DispatcherConfig cfg;
	cfg.name          = name;
	cfg.auto_timestamp = auto_timestamp;

	return GmDispatcher(
		std::move(cfg),
		std::make_unique<AsyncDispatcher>(
			std::make_unique<SyncRouter>()));
}

GmDispatcher DispatcherFactory::create_pattern_dispatcher(const std::string& name,
													  bool               auto_timestamp)
{
	DispatcherConfig cfg;
	cfg.name          = name;
	cfg.auto_timestamp = auto_timestamp;

	return GmDispatcher(
		std::move(cfg),
		std::make_unique<SyncDispatcher>(
			std::make_unique<PatternRouter>()));
}

} // namespace gmDispatch
