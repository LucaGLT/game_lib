/**
 * @file log/GameLog.cpp
 * @brief GameLog implementation using gmLog.
 */

#include "log/GameLog.hpp"

#include "gmLog/Logger.hpp"
#include "gmLog/LoggerFactory.hpp"
#include "gmLog/LogLevel.hpp"

#include <memory>

namespace gmDungeonBasic
{

static std::unique_ptr<gmLog::GmLogger> g_logger;

static gmLog::GmLogger& logger()
{
	if (!g_logger)
	{
		g_logger = std::make_unique<gmLog::GmLogger>(
			gmLog::LoggerFactory::create_stdout_logger(
				"DungeonEngine",
				gmLog::LogLevel::INFO,
				false));
	}
	return *g_logger;
}

GameLog::GameLog()
{
	// Logger is initialised lazily on first use.
}

void GameLog::log_session_start(const std::string& session_id,
                                const std::string& map_file)
{
	const std::string _msg = "[session_start] id=" + session_id + " map=" + map_file;
	logger().log(gmLog::LogLevel::INFO, _msg);
}

void GameLog::log_action(const std::string& actor_id,
                         const std::string& action,
                         const std::string& detail)
{
	std::string msg = "[action] actor=" + actor_id + " action=" + action;
	if (!detail.empty())
	{
		msg += " detail=" + detail;
	}
	logger().log(gmLog::LogLevel::INFO, msg);
}

void GameLog::log_rejection(const std::string& actor_id,
                            const std::string& command,
                            const std::string& reason)
{
	const std::string reason_str = "[rejected] actor=" + actor_id +
	                               " cmd=" + command +
	                               " reason=" + reason;
	logger().log(gmLog::LogLevel::WARNING, reason_str);
}

void GameLog::log_session_end(const std::string& outcome)
{
	const std::string _msg_end = "[session_end] outcome=" + outcome;
	logger().log(gmLog::LogLevel::INFO, _msg_end);
}

void GameLog::log_info(const std::string& message)
{
	logger().log(gmLog::LogLevel::INFO, message);
}

} // namespace gmDungeonBasic
