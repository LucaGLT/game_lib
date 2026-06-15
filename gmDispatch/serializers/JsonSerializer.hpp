#ifndef GMDISPATCH_JSONSERIALIZER_HPP
#define GMDISPATCH_JSONSERIALIZER_HPP

/**
 * @file serializers/JsonSerializer.hpp
 * @brief JSON Lines serializer for gmDispatch envelopes.
 */

#include "ISerializer.hpp"

#include <string>

namespace gmDispatch {

/**
 * @brief Serialises an @ref Envelope to a single-line JSON object (JSON Lines).
 *
 * Each call to @ref serialize() produces one JSON object with no trailing
 * newline.  The channel (e.g. @ref StdoutChannel) is responsible for adding
 * the line terminator.
 *
 * ### Output format
 * @code
 *   {"time":"2026-06-11T10:30:00.123","source":"CoreEngine","typeId":"engine.tick",
 *    "messageId":"","targets":[],"payload":"TickData"}
 * @endcode
 *
 * | Field | Type | Notes |
 * |---|---|---|
 * | @c time | string (ISO-8601 UTC, ms) | From @c Envelope::timestamp |
 * | @c source | string | @c Envelope::source |
 * | @c typeId | string | @c Envelope::typeId |
 * | @c messageId | string | @c Envelope::messageId (empty when not set) |
 * | @c targets | JSON array of strings | @c Envelope::targets (empty array = broadcast) |
 * | @c payload | string | @c std::any::type().name() or custom (Phase 3) |
 *
 * ### Relationship with GmLog::JsonFormatter
 * Applies the same escaping rules as @c GmLog::JsonFormatter::escape_json_string.
 * The two formatters are independent (no shared code) to keep the libraries
 * decoupled.
 */
class JsonSerializer : public ISerializer {
public:
	JsonSerializer() = default;

	/**
	 * @brief Converts @p envelope to a single-line JSON object.
	 *
	 * @param envelope The dispatch event to serialise.
	 * @return JSON string without trailing newline.
	 */
	std::string serialize(const Envelope& envelope) override;

	/**
	 * @brief Escapes a string value for safe embedding inside a JSON string.
	 *
	 * Escapes: @c \\ → @c \\\\, @c " → @c \\", newline → @c \\n,
	 * CR → @c \\r, tab → @c \\t, control chars @c < 0x20 → @c \\uXXXX.
	 *
	 * @param value Raw string to escape.
	 * @return Escaped string suitable for use between JSON double-quotes.
	 */
	static std::string escape_json_string(const std::string& value);
};

} // namespace gmDispatch

#endif // GMDISPATCH_JSONSERIALIZER_HPP
