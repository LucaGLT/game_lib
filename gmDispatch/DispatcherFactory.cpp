#include "DispatcherFactory.hpp"
#include "DispatcherConfig.hpp"
#include "dispatchers/SyncDispatcher.hpp"
#include "routers/SyncRouter.hpp"
#include "channels/StdoutChannel.hpp"

#include <memory>

namespace GmDispatch {

Dispatcher DispatcherFactory::createSyncDispatcher(const std::string& name,
                                                   bool               autoTimestamp)
{
    DispatcherConfig cfg;
    cfg.name          = name;
    cfg.autoTimestamp = autoTimestamp;

    return Dispatcher(
        std::move(cfg),
        std::make_unique<SyncDispatcher>(
            std::make_unique<SyncRouter>()));
}

Dispatcher DispatcherFactory::createDebugDispatcher(const std::string& name)
{
    DispatcherConfig cfg;
    cfg.name          = name;
    cfg.autoTimestamp = true;

    std::unique_ptr<SyncDispatcher> impl =
        std::make_unique<SyncDispatcher>(std::make_unique<SyncRouter>());

    // Subscribe a StdoutChannel to "*" so every envelope is printed.
    impl->subscribe("*", std::make_shared<StdoutChannel>());

    return Dispatcher(std::move(cfg), std::move(impl));
}

} // namespace GmDispatch
