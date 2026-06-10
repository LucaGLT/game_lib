/**
 * @file LogLevel.cpp
 * @brief Implementation of LogLevel utility functions.
 */

#include "LogLevel.hpp"

#include <algorithm>
#include <cctype>

namespace GmLog {

const char* levelToString(LogLevel level)
{
    switch (level) {
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warning:  return "WARNING";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        case LogLevel::Off:      return "OFF";
        default:                 return "UNKNOWN";
    }
}

bool levelFromString(const std::string& str, LogLevel& out)
{
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == "debug")    { out = LogLevel::Debug;    return true; }
    if (lower == "info")     { out = LogLevel::Info;     return true; }
    if (lower == "warning")  { out = LogLevel::Warning;  return true; }
    if (lower == "error")    { out = LogLevel::Error;    return true; }
    if (lower == "critical") { out = LogLevel::Critical; return true; }
    if (lower == "off")      { out = LogLevel::Off;      return true; }
    return false;
}

} // namespace GmLog
