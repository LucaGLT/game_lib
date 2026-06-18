/**
 * @file log/GameLog.cpp
 * @brief Stub implementation of GameLog.
 *
 * Real gmLog integration will be introduced in FASE B.
 */

#include "log/GameLog.hpp"

#include <iostream>

namespace gmDungeonBasic
{

GameLog::GameLog()
{
	// ToBeImplemented //
}

void GameLog::log_session_start(const std::string& session_id,
                                const std::string& map_file)
{
	(void)session_id;
	(void)map_file;
	// ToBeImplemented //
	std::cout << "[GameLog] session_start stub.\n";
}

void GameLog::log_action(const std::string& actor_id,
                         const std::string& action,
                         const std::string& detail)
{
	(void)actor_id;
	(void)action;
	(void)detail;
	// ToBeImplemented //
}

void GameLog::log_rejection(const std::string& actor_id,
                            const std::string& command,
                            const std::string& reason)
{
	(void)actor_id;
	(void)command;
	(void)reason;
	// ToBeImplemented //
}

void GameLog::log_session_end(const std::string& outcome)
{
	(void)outcome;
	// ToBeImplemented //
}

void GameLog::log_info(const std::string& message)
{
	(void)message;
	// ToBeImplemented //
}

} // namespace gmDungeonBasic
