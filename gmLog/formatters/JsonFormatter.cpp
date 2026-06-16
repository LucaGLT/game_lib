/**
 * @file formatters/JsonFormatter.cpp
 * @brief Implementation of JsonFormatter.
 */

#include "JsonFormatter.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <sstream>

#include "LogLevel.hpp"

namespace gmLog {

std::string JsonFormatter::format(const LogRecord& record)
{
	std::ostringstream oss;

	oss << '{'
		<< "\"time\":"    << '"' << format_timestamp(record.timestamp)        << '"'
		<< ",\"logger\":" << '"' << escape_json_string(record.logger_name)      << '"'
		<< ",\"level\":"  << '"' << level_to_string(record.level)              << '"';

	if (record.file && record.line > 0)
	{
		oss << ",\"file\":"     << '"' << escape_json_string(record.file) << '"'
			<< ",\"line\":"     << record.line;
	}

	if (record.function)
	{
		oss << ",\"function\":" << '"' << escape_json_string(record.function) << '"';
	}

	oss << ",\"message\":" << '"' << escape_json_string(record.message) << '"'
		<< '}';

	return oss.str();
}

std::string JsonFormatter::escape_json_string(const std::string& value)
{
	std::string result;
	result.reserve(value.size() + 8);

	for (unsigned char c : value)
	{
		switch (c)
		{
			case '"':  result += "\\\""; break;
			case '\\': result += "\\\\"; break;
			case '\n': result += "\\n";  break;
			case '\r': result += "\\r";  break;
			case '\t': result += "\\t";  break;
			default:
				if (c < 0x20)
				{
					char buf[7];
					std::snprintf(buf, sizeof(buf), "\\u%04x", c);
					result += buf;
				}
				else
				{
					result += static_cast<char>(c);
				}
				break;
		}
	}

	return result;
}

std::string JsonFormatter::format_timestamp(
	const std::chrono::system_clock::time_point& tp)
{
	using namespace std::chrono;

	const std::time_t tt = system_clock::to_time_t(tp);

	std::tm tm_utc{};
	#if defined(_WIN32)
	// On Windows use the thread-safe API to avoid deprecation warnings.
	gmtime_s(&tm_utc, &tt);
	#else
	// On POSIX use the thread-safe API.
	gmtime_r(&tt, &tm_utc);
	#endif

	const auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

	char date_buf[24];
	std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%dT%H:%M:%S", &tm_utc);

	char result[32];
	std::snprintf(result, sizeof(result), "%s.%03d",
				  date_buf, static_cast<int>(ms.count()));

	return result;
}

} // namespace gmLog
