/**
 * @file Logger.cpp
 * @brief Implementation of the Logger class.
 */

#include "Logger.hpp"

#include <chrono>

namespace GmLog {

Logger::Logger(LoggerConfig config, std::unique_ptr<ILogDispatcher> dispatcher)
    : config_(std::move(config))
    , dispatcher_(std::move(dispatcher))
{}

Logger::~Logger()
{
    if (dispatcher_)
        dispatcher_->flush();
}

const std::string& Logger::name() const
{
    return config_.name;
}

LogLevel Logger::minLevel() const
{
    return config_.minLevel;
}

void Logger::setLevel(LogLevel level)
{
    config_.minLevel = level;
}

bool Logger::isEnabled(LogLevel level) const
{
    return level >= config_.minLevel;
}

void Logger::log(LogLevel           level,
                 const std::string& message,
                 const char*        file,
                 int                line,
                 const char*        function)
{
    if (!isEnabled(level))
        return;

    LogRecord record;
    record.level      = level;
    record.loggerName = config_.name;
    record.message    = message;
    record.timestamp  = std::chrono::system_clock::now();

    if (config_.enableSourceLocation) {
        record.file     = file;
        record.line     = line;
        record.function = function;
    }

    dispatcher_->dispatch(record);
}

void Logger::debug(const std::string& message)
{
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message)
{
    log(LogLevel::Info, message);
}

void Logger::warning(const std::string& message)
{
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message)
{
    log(LogLevel::Error, message);
}

void Logger::critical(const std::string& message)
{
    log(LogLevel::Critical, message);
    flush();
}

void Logger::flush()
{
    if (dispatcher_)
        dispatcher_->flush();
}

} // namespace GmLog
