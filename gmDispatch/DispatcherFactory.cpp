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
    // TODO: Phase 2 — implement:
    //   auto router     = std::make_unique<SyncRouter>();
    //   auto dispatcher = std::make_unique<SyncDispatcher>(std::move(router));
    //   DispatcherConfig cfg; cfg.name = name; cfg.autoTimestamp = autoTimestamp;
    //   return Dispatcher(std::move(cfg), std::move(dispatcher));

    DispatcherConfig cfg;
    cfg.name          = name;
    cfg.autoTimestamp = autoTimestamp;
    return Dispatcher(std::move(cfg), nullptr);
}

Dispatcher DispatcherFactory::createDebugDispatcher(const std::string& name)
{
    // TODO: Phase 2 — implement:
    //   create SyncDispatcher + SyncRouter, then subscribe a StdoutChannel to "*"

    DispatcherConfig cfg;
    cfg.name          = name;
    cfg.autoTimestamp = true;
    return Dispatcher(std::move(cfg), nullptr);
}

} // namespace GmDispatch
