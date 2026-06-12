/**
 * @file LoggerFactory.cpp
 * @brief Implementation of the LoggerFactory static helpers.
 */

#include "LoggerFactory.hpp"

#include <memory>

#include "dispatchers/SyncDispatcher.hpp"
#include "formatters/JsonFormatter.hpp"
#include "sinks/FileSink.hpp"
#include "sinks/StdoutSink.hpp"

namespace gmLog {

GmLogger LoggerFactory::create_stdout_logger(const std::string& name,
											 LogLevel           level,
											 bool               enable_source_location
)
{
	LoggerConfig cfg;
	cfg.name = name;
	cfg.min_level = level;
	cfg.enable_source_location = enable_source_location;

	return GmLogger(
		cfg,
		std::make_unique<SyncDispatcher>(
			std::make_unique<StdoutSink>(),
			std::make_unique<JsonFormatter>()
		)
	);
}

GmLogger LoggerFactory::create_file_logger(const std::string& name,
										   const std::string& filePath,
										   LogLevel           level,
										   bool               enable_source_location
)
{
	LoggerConfig cfg;
	cfg.name = name;
	cfg.min_level = level;
	cfg.enable_source_location = enable_source_location;

	return GmLogger(
		cfg,
		std::make_unique<SyncDispatcher>(
			std::make_unique<FileSink>(filePath),
			std::make_unique<JsonFormatter>()
		)
	);
}

} // namespace gmLog
