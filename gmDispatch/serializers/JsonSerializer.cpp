/**
 * @file serializers/JsonSerializer.cpp
 * @brief Implementation of JsonSerializer.
 */

#include "JsonSerializer.hpp"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <sstream>

namespace gmDispatch {

// ── Internal helper ───────────────────────────────────────────────────────────

static std::string formatTimestamp(
	const std::chrono::system_clock::time_point& tp)
{
	using namespace std::chrono;

	const std::time_t tt = system_clock::to_time_t(tp);

	// Use gmtime_s (Windows) / gmtime_r (POSIX) to avoid deprecated gmtime().
	std::tm tm_utc{};
#if defined(_WIN32)
	gmtime_s(&tm_utc, &tt);
#else
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

// ── Public interface ──────────────────────────────────────────────────────────

std::string JsonSerializer::serialize(const Envelope& envelope)
{
	std::ostringstream oss;

	oss << '{'
		<< "\"time\":"      << '"' << formatTimestamp(envelope.timestamp)        << '"'
		<< ",\"source\":"   << '"' << escape_json_string(envelope.source)           << '"'
		<< ",\"typeId\":"   << '"' << escape_json_string(envelope.typeId)           << '"'
		<< ",\"messageId\":" << '"' << escape_json_string(envelope.messageId)       << '"';

	// targets JSON array
	oss << ",\"targets\":[";
	for (std::size_t i = 0; i < envelope.targets.size(); ++i) {
		if (i > 0) oss << ',';
		oss << '"' << escape_json_string(envelope.targets[i]) << '"';
	}
	oss << ']';

	// headers JSON object
	oss << ",\"headers\":{";
	std::size_t header_index = 0;
	for (const std::pair<const std::string, std::string>& header : envelope.headers)
	{
		if (header_index > 0)
		{
			oss << ',';
		}

		oss << '"' << escape_json_string(header.first) << '"'
			<< ':'
			<< '"' << escape_json_string(header.second) << '"';
		++header_index;
	}
	oss << '}';

	// payload: type().name() as best-effort string (Phase 3: per-type serializers)
	const std::string payloadStr = envelope.payload.has_value()
									   ? std::string(envelope.payload.type().name())
									   : "";
	oss << ",\"payload\":" << '"' << escape_json_string(payloadStr) << '"'
		<< '}';

	return oss.str();
}

std::string JsonSerializer::escape_json_string(const std::string& value)
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

} // namespace gmDispatch
