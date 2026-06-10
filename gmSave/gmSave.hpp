#ifndef GMSAVE_HPP
#define GMSAVE_HPP

/**
 * @file gmSave.hpp
 * @brief Generic JSON serialization / deserialization library for arbitrary structs.
 *
 * gmSave provides a thin, type-safe wrapper around nlohmann/json that allows
 * any user-defined struct to be saved to and loaded from a JSON file with
 * minimal boilerplate.
 *
 * ### User contract
 * For each struct `T` the caller must define two free functions in the same
 * namespace as `T` (found via ADL):
 * @code
 *   void to_json  (nlohmann::json& j, const T& obj);
 *   void from_json(const nlohmann::json& j, T& obj);
 * @endcode
 * nlohmann/json provides built-in support for all primitive types,
 * `std::string`, `std::vector<T>`, and `std::optional<T>` (>= nlohmann 3.9).
 * Nested structs are handled automatically provided each nested type also
 * exposes its own `to_json`/`from_json` pair.
 *
 * ### Versioned saves
 * `save_versioned` / `load_versioned` wrap the serialized payload inside a
 * JSON envelope:
 * @code{.json}
 * {
 *   "_version": 2,
 *   "payload": { ... }
 * }
 * @endcode
 * Use `peek_version` to inspect the version field without deserializing the
 * full payload.
 *
 * @note All method bodies are defined as inline template functions below the
 *       class declarations (C++ template requirement). They will be completed
 *       in subsequent development phases (see PLAN.md).
 *
 * @par Dependency
 *   Requires nlohmann/json single-header `json.hpp` placed next to this file.
 *   Download from: https://github.com/nlohmann/json/releases (single_include)
 */

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

#include "json.hpp"

static_assert(__cplusplus >= 201703L, "gmSave requires C++17 or later (-std=c++17)");

namespace GmSave {

// --- Exceptions ---------------------------------------------------------------

/**
 * @brief Base exception class for all gmSave errors.
 */
class SaveError : public std::runtime_error {
public:
    explicit SaveError(const std::string& message)
        : std::runtime_error("SaveError: " + message) {}
};

/**
 * @brief Thrown when a file cannot be opened or written to disk.
 */
class FileWriteError : public SaveError {
public:
    explicit FileWriteError(const std::string& message)
        : SaveError(message) {}
};

/**
 * @brief Thrown when a file cannot be found or read from disk.
 */
class FileReadError : public SaveError {
public:
    explicit FileReadError(const std::string& message)
        : SaveError(message) {}
};

/**
 * @brief Thrown when the file content is not valid JSON.
 */
class JsonParseError : public SaveError {
public:
    explicit JsonParseError(const std::string& message)
        : SaveError(message) {}
};

/**
 * @brief Thrown by load_versioned() when the `_version` field in the file
 *        does not match the expected version supplied by the caller.
 */
class VersionMismatchError : public SaveError {
public:
    /**
     * @brief Constructs the error with version detail.
     * @param expected Version number the caller requested.
     * @param found    Version number read from the file.
     */
    VersionMismatchError(uint32_t expected, uint32_t found);

    /// @brief Version number the caller requested.
    uint32_t expected_version;

    /// @brief Version number actually found in the file.
    uint32_t found_version;
};

// --- Internal helpers (not part of the public API) ----------------------------

/**
 * @namespace GmSave::detail
 * @brief Internal implementation helpers; not part of the public API.
 */
namespace detail {

/**
 * @brief Writes @p content to a file, creating or overwriting it.
 * @param filepath  Destination file path.
 * @param content   String to write.
 * @throws FileWriteError if the file cannot be opened or written.
 */
void write_file(const std::string& filepath, const std::string& content);

/**
 * @brief Reads the entire content of a file into a string.
 * @param filepath  Source file path.
 * @return File content as a string.
 * @throws FileReadError if the file cannot be opened.
 */
std::string read_file(const std::string& filepath);

/**
 * @brief Parses a JSON string, mapping nlohmann parse errors to JsonParseError.
 * @param content   Raw JSON string.
 * @param filepath  File path used in error messages only.
 * @return Parsed `nlohmann::json` object.
 * @throws JsonParseError if @p content is not valid JSON.
 */
nlohmann::json parse_json(const std::string& content, const std::string& filepath);

} // namespace detail

// --- Free functions -----------------------------------------------------------

/**
 * @brief Serializes a value to a JSON file.
 *
 * Calls `to_json(j, data)` via ADL to convert @p data to a
 * `nlohmann::json` object, then writes it to @p filepath.
 *
 * @tparam T   Type of the value to serialize.  Must have a matching
 *             `to_json(nlohmann::json&, const T&)` free function.
 * @param filepath  Destination file path (created or overwritten).
 * @param data      Value to serialize.
 * @param indent    JSON indentation in spaces (default 2; use -1 for compact).
 * @throws FileWriteError if the file cannot be opened or written.
 */
template <typename T>
void save(const std::string& filepath, const T& data, int indent = 2);

/**
 * @brief Deserializes a value from a JSON file.
 *
 * Reads @p filepath, parses it as JSON, then calls
 * `from_json(j, result)` via ADL to populate the returned object.
 *
 * @tparam T   Type of the value to deserialize.  Must have a matching
 *             `from_json(const nlohmann::json&, T&)` free function.
 * @param filepath  Source file path.
 * @return The deserialized value of type @p T.
 * @throws FileReadError   if the file cannot be opened.
 * @throws JsonParseError  if the file content is not valid JSON.
 */
template <typename T>
T load(const std::string& filepath);

/**
 * @brief Non-throwing variant of load().
 *
 * Attempts to load and deserialize the file into @p out.  Returns
 * `false` and leaves @p out unchanged on any error.
 *
 * @tparam T   Type of the value to deserialize.
 * @param filepath  Source file path.
 * @param out       Output variable populated on success.
 * @return `true` on success, `false` on any error (I/O or parse failure).
 */
template <typename T>
bool try_load(const std::string& filepath, T& out) noexcept;

/**
 * @brief Serializes a value to a JSON file with a version envelope.
 *
 * The output JSON has the form:
 * @code{.json}
 * {
 *   "_version": <version>,
 *   "payload": { ... }
 * }
 * @endcode
 *
 * @tparam T   Type of the value to serialize.  Must have a matching
 *             `to_json(nlohmann::json&, const T&)` free function.
 * @param filepath  Destination file path (created or overwritten).
 * @param data      Value to serialize.
 * @param version   Version tag written to the `_version` field.
 * @param indent    JSON indentation in spaces (default 2; use -1 for compact).
 * @throws FileWriteError if the file cannot be opened or written.
 */
template <typename T>
void save_versioned(const std::string& filepath,
                    const T&           data,
                    uint32_t           version,
                    int                indent = 2);

/**
 * @brief Deserializes a value from a versioned JSON file.
 *
 * Reads the file, checks that `_version == expected_version`, then
 * deserializes the `"payload"` object into the returned value.
 *
 * @tparam T   Type of the value to deserialize.  Must have a matching
 *             `from_json(const nlohmann::json&, T&)` free function.
 * @param filepath         Source file path.
 * @param expected_version Version the caller expects to find.
 * @return The deserialized value of type @p T.
 * @throws FileReadError        if the file cannot be opened.
 * @throws JsonParseError       if the file content is not valid JSON or
 *                              the `_version` / `payload` fields are missing.
 * @throws VersionMismatchError if `_version != expected_version`.
 */
template <typename T>
T load_versioned(const std::string& filepath, uint32_t expected_version);

/**
 * @brief Reads only the `_version` field from a versioned JSON file.
 *
 * Useful for detecting save-file version before deciding which
 * struct / migration path to use, without deserializing the payload.
 *
 * @param filepath  Source file path.
 * @return The version number if the field is present, or `std::nullopt`
 *         if the file does not contain a `_version` field (or on any I/O /
 *         parse error).
 */
std::optional<uint32_t> peek_version(const std::string& filepath);

// =============================================================================
// Inline template implementations
// =============================================================================

template <typename T>
void save(const std::string& filepath, const T& data, int indent)
{
    nlohmann::json j = data;
    detail::write_file(filepath, j.dump(indent));
}

template <typename T>
T load(const std::string& filepath)
{
    const std::string    content = detail::read_file(filepath);
    const nlohmann::json j       = detail::parse_json(content, filepath);
    return j.get<T>();
}

template <typename T>
bool try_load(const std::string& filepath, T& out) noexcept
{
    try {
        out = load<T>(filepath);
        return true;
    } catch (...) {
        return false;
    }
}

template <typename T>
void save_versioned(const std::string& filepath,
                    const T&           data,
                    uint32_t           version,
                    int                indent)
{
    nlohmann::json envelope;
    envelope["_version"] = version;
    envelope["payload"]  = data;
    detail::write_file(filepath, envelope.dump(indent));
}

template <typename T>
T load_versioned(const std::string& filepath, uint32_t expected_version)
{
    const std::string    content  = detail::read_file(filepath);
    const nlohmann::json envelope = detail::parse_json(content, filepath);

    if (!envelope.contains("_version") || !envelope.contains("payload")) {
        throw JsonParseError(
            "Missing '_version' or 'payload' field in: " + filepath);
    }

    const uint32_t found = envelope.at("_version").get<uint32_t>();
    if (found != expected_version) {
        throw VersionMismatchError(expected_version, found);
    }

    return envelope.at("payload").get<T>();
}

} // namespace GmSave

#endif // GMSAVE_HPP
