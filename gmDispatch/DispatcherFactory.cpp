#include "DispatcherFactory.hpp"
#include "DispatcherConfig.hpp"
#include "dispatchers/SyncDispatcher.hpp"
#include "dispatchers/AsyncDispatcher.hpp"
#include "routers/SyncRouter.hpp"
#include "routers/PatternRouter.hpp"
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

    impl->subscribe("*", std::make_shared<StdoutChannel>());

    return Dispatcher(std::move(cfg), std::move(impl));
}

Dispatcher DispatcherFactory::createAsyncDispatcher(const std::string& name,
                                                    bool               autoTimestamp)
{
    DispatcherConfig cfg;
    cfg.name          = name;
    cfg.autoTimestamp = autoTimestamp;

    return Dispatcher(
        std::move(cfg),
        std::make_unique<AsyncDispatcher>(
            std::make_unique<SyncRouter>()));
}

Dispatcher DispatcherFactory::createPatternDispatcher(const std::string& name,
                                                      bool               autoTimestamp)
{
    DispatcherConfig cfg;
    cfg.name          = name;
    cfg.autoTimestamp = autoTimestamp;

    return Dispatcher(
        std::move(cfg),
        std::make_unique<SyncDispatcher>(
            std::make_unique<PatternRouter>()));
}

} // namespace GmDispatch
