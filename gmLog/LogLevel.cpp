/**
 * @file LogLevel.cpp
 * @brief Implementation of LogLevel utility functions.
 */

#include "LogLevel.hpp"

#include <algorithm>
#include <cctype>

namespace gmLog {

const char* level_to_string(LogLevel level)
{
	switch (level)
	{
		case LogLevel::DEBUG:    return "DEBUG";
		case LogLevel::INFO:     return "INFO";
		case LogLevel::WARNING:  return "WARNING";
		case LogLevel::ERROR:    return "ERROR";
		case LogLevel::CRITICAL: return "CRITICAL";
		case LogLevel::OFF:      return "OFF";
		default:                 return "UNKNOWN";
	}
}

bool level_from_string(const std::string& str, LogLevel& out)
{
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(),
				   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	if (lower == "debug")
	{
		out = LogLevel::DEBUG;
		return true;
	}
	if (lower == "info")
	{
		out = LogLevel::INFO;
		return true;
	}
	if (lower == "warning")
	{
		out = LogLevel::WARNING;
		return true;
	}
	if (lower == "error")
	{
		out = LogLevel::ERROR;
		return true;
	}
	if (lower == "critical")
	{
		out = LogLevel::CRITICAL;
		return true;
	}
	if (lower == "off")
	{
		out = LogLevel::OFF;
		return true;
	}
	return false;
}

} // namespace gmLog
