/**
 * @file Logger.cpp
 * @brief Implementation of the GmLogger class.
 */

#include "Logger.hpp"

#include <chrono>

namespace gmLog {

GmLogger::GmLogger(LoggerConfig config, std::unique_ptr<ILogDispatcher> dispatcher)
	: _config(std::move(config))
	, _dispatcher(std::move(dispatcher))
{}

GmLogger::~GmLogger()
{
	if (_dispatcher)
	{
		_dispatcher->flush();
	}
}

const std::string& GmLogger::name() const
{
	return _config.name;
}

LogLevel GmLogger::min_level() const
{
	return _config.min_level;
}

void GmLogger::set_level(LogLevel level)
{
	_config.min_level = level;
}

bool GmLogger::is_enabled(LogLevel level) const
{
	return level >= _config.min_level;
}

void GmLogger::log(LogLevel           level,
				   const std::string& message,
				   const char*        file,
				   int                line,
				   const char*        function
)
{
	if (!is_enabled(level))
		return;

	LogRecord record;
	record.level = level;
	record.logger_name = _config.name;
	record.message = message;
	record.timestamp = std::chrono::system_clock::now();

	if (_config.enable_source_location)
	{
		record.file = file;
		record.line = line;
		record.function = function;
	}

	_dispatcher->dispatch(record);
}

void GmLogger::debug(const std::string& message)
{
	log(LogLevel::DEBUG, message);
}

void GmLogger::info(const std::string& message)
{
	log(LogLevel::INFO, message);
}

void GmLogger::warning(const std::string& message)
{
	log(LogLevel::WARNING, message);
}

void GmLogger::error(const std::string& message)
{
	log(LogLevel::ERROR, message);
}

void GmLogger::critical(const std::string& message)
{
	log(LogLevel::CRITICAL, message);
	flush();
}

void GmLogger::flush()
{
	if (_dispatcher)
	{
		_dispatcher->flush();
	}
}

} // namespace gmLog
