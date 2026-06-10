/**
 * @file LoggerFactory.cpp
 * @brief Implementation of the LoggerFactory static helpers.
 */

#include "LoggerFactory.hpp"

#include "dispatchers/SyncDispatcher.hpp"
#include "formatters/JsonFormatter.hpp"
#include "sinks/FileSink.hpp"
#include "sinks/StdoutSink.hpp"

#include <memory>

namespace GmLog {

Logger LoggerFactory::createStdoutLogger(const std::string& name,
                                         LogLevel           level,
                                         bool               enableSourceLocation)
{
    LoggerConfig cfg;
    cfg.name                 = name;
    cfg.minLevel             = level;
    cfg.enableSourceLocation = enableSourceLocation;

    return Logger(
        cfg,
        std::make_unique<SyncDispatcher>(
            std::make_unique<StdoutSink>(),
            std::make_unique<JsonFormatter>()
        )
    );
}

Logger LoggerFactory::createFileLogger(const std::string& name,
                                       const std::string& filePath,
                                       LogLevel           level,
                                       bool               enableSourceLocation)
{
    LoggerConfig cfg;
    cfg.name                 = name;
    cfg.minLevel             = level;
    cfg.enableSourceLocation = enableSourceLocation;

    return Logger(
        cfg,
        std::make_unique<SyncDispatcher>(
            std::make_unique<FileSink>(filePath),
            std::make_unique<JsonFormatter>()
        )
    );
}

} // namespace GmLog
