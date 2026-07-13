/**
 * @file logging/GameLog.cpp
 * @brief Implementation of the gmLog wrapper.
 */

#include "GameLog.hpp"

#include "gmLog/LoggerFactory.hpp"

namespace gmTris
{

GameLog::GameLog()
    : _logger(gmLog::LoggerFactory::create_stdout_logger("Tris", gmLog::LogLevel::INFO))
{
}

void GameLog::info(const std::string& message)
{
	_logger.info(message);
}

void GameLog::warn(const std::string& message)
{
	_logger.warning(message);
}

} // namespace gmTris
