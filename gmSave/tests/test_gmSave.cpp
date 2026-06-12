/**
 * @file tests/test_gmSave.cpp
 * @brief Unit tests for the GmSave library — Phase 11.
 *
 * Runs all 16 test cases defined in gmSave/PLAN.md Phase 11.
 * Results are logged to @c test_gmSave_out.log via gmLog.
 *
 * ### Build (from workspace root)
 * @code
 *   g++ -std=c++17 -I. \
 *       gmSave/tests/test_gmSave.cpp \
 *       gmSave/gmSave.cpp \
 *       gmLog/LogLevel.cpp gmLog/Logger.cpp gmLog/LoggerFactory.cpp \
 *       gmLog/sinks/StdoutSink.cpp gmLog/sinks/FileSink.cpp \
 *       gmLog/formatters/JsonFormatter.cpp \
 *       gmLog/dispatchers/SyncDispatcher.cpp \
 *       -o test_gmSave
 * @endcode
 */

#include "../gmSave.hpp"
#include "../../gmLog/LoggerFactory.hpp"
#include "../../gmLog/macros/LogMacros.hpp"

#include <cstdio>       // std::remove
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

// =============================================================================
// Test structs + ADL hooks
// =============================================================================

// --- Flat struct ---

struct FlatItem {
    std::string name;
    int         value;
    int         count;
};

inline void to_json(nlohmann::json& j, const FlatItem& f)
{
    j = {{"name", f.name}, {"value", f.value}, {"count", f.count}};
}

inline void from_json(const nlohmann::json& j, FlatItem& f)
{
    j.at("name") .get_to(f.name);
    j.at("value").get_to(f.value);
    j.at("count").get_to(f.count);
}

inline bool operator==(const FlatItem& a, const FlatItem& b)
{
    return a.name == b.name && a.value == b.value && a.count == b.count;
}

// --- Inner struct (for nesting) ---

struct Tag {
    std::string key;
    std::string label;
};

inline void to_json(nlohmann::json& j, const Tag& t)
{
    j = {{"key", t.key}, {"label", t.label}};
}

inline void from_json(const nlohmann::json& j, Tag& t)
{
    j.at("key")  .get_to(t.key);
    j.at("label").get_to(t.label);
}

inline bool operator==(const Tag& a, const Tag& b)
{
    return a.key == b.key && a.label == b.label;
}

// --- Nested struct ---

struct NestedItem {
    std::string name;
    Tag         tag;
};

inline void to_json(nlohmann::json& j, const NestedItem& n)
{
    j = {{"name", n.name}, {"tag", n.tag}};
}

inline void from_json(const nlohmann::json& j, NestedItem& n)
{
    j.at("name").get_to(n.name);
    j.at("tag") .get_to(n.tag);
}

inline bool operator==(const NestedItem& a, const NestedItem& b)
{
    return a.name == b.name && a.tag == b.tag;
}

// --- Struct with std::vector<int> ---

struct ListItem {
    std::string      title;
    std::vector<int> scores;
};

inline void to_json(nlohmann::json& j, const ListItem& l)
{
    j = {{"title", l.title}, {"scores", l.scores}};
}

inline void from_json(const nlohmann::json& j, ListItem& l)
{
    j.at("title") .get_to(l.title);
    j.at("scores").get_to(l.scores);
}

inline bool operator==(const ListItem& a, const ListItem& b)
{
    return a.title == b.title && a.scores == b.scores;
}

// --- Struct with std::optional<int> ---

struct OptItem {
    std::string        name;
    std::optional<int> bonus;
};

inline void to_json(nlohmann::json& j, const OptItem& o)
{
    j["name"]  = o.name;
    j["bonus"] = o.bonus.has_value()
                     ? nlohmann::json(*o.bonus)
                     : nlohmann::json(nullptr);
}

inline void from_json(const nlohmann::json& j, OptItem& o)
{
    j.at("name").get_to(o.name);
    const nlohmann::json& b = j.at("bonus");
    if (b.is_null())
        o.bonus = std::nullopt;
    else
        o.bonus = b.get<int>();
}

inline bool operator==(const OptItem& a, const OptItem& b)
{
    return a.name == b.name && a.bonus == b.bonus;
}

// =============================================================================
// RAII temporary-file guard
// =============================================================================

/**
 * @brief Deletes the file at @p path when the guard goes out of scope.
 */
struct TempFile {
    std::string path;
    explicit TempFile(std::string p) : path(std::move(p)) {}
    ~TempFile() { std::remove(path.c_str()); }
};

// =============================================================================
// Minimal test runner
// =============================================================================

/**
 * @brief Lightweight test-result tracker.
 *
 * Holds a reference to a gmLog Logger and records pass/fail counts.
 * Use @ref check, @ref checkThrows, and @ref checkNoThrow to register results.
 */
struct TestRunner {
    gmLog::GmLogger& log;
    int            passed = 0;
    int            failed = 0;

    // --- check a plain boolean condition ------------------------------------

    void check(bool condition, const std::string& testName)
    {
        if (condition) {
            ++passed;
            logInfo(log, "[PASS] " + testName);
        } else {
            ++failed;
            logErr(log, "[FAIL] " + testName);
        }
    }

    // --- check that a specific exception type is thrown ---------------------

    template <typename ExcType, typename Fn>
    void checkThrows(const std::string& testName, Fn&& fn)
    {
        try {
            fn();
            ++failed;
            logErr(log, "[FAIL] " + testName + " — expected exception not thrown");
        }
        catch (const ExcType&) {
            ++passed;
            logInfo(log, "[PASS] " + testName);
        }
        catch (const std::exception& e) {
            ++failed;
            logErr(log, "[FAIL] " + testName
                      + " — wrong exception type: " + e.what());
        }
    }

    // --- check that no exception is thrown ----------------------------------

    template <typename Fn>
    void checkNoThrow(const std::string& testName, Fn&& fn)
    {
        try {
            fn();
            ++passed;
            logInfo(log, "[PASS] " + testName);
        }
        catch (const std::exception& e) {
            ++failed;
            logErr(log, "[FAIL] " + testName
                      + " — unexpected exception: " + e.what());
        }
    }

    // --- print summary -------------------------------------------------------

    void summary()
    {
        const int total = passed + failed;
        std::ostringstream oss;
        oss << "=== gmSave test summary: "
            << passed << "/" << total << " passed";
        if (failed > 0)
            oss << "  *** " << failed << " FAILED ***";

        if (failed == 0)
            logInfo(log, oss.str());
        else
            logErr(log, oss.str());
    }
};

// =============================================================================
// Individual test groups
// =============================================================================

// T01 — save + load round-trip: flat struct
static void test_roundtrip_flat(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_flat.json");

    const FlatItem original{"Sword", 150, 3};

    GmSave::save(tmp.path, original);
    const FlatItem loaded = GmSave::load<FlatItem>(tmp.path);

    tr.check(loaded == original, "T01 round-trip flat struct");
}

// T02 — save + load round-trip: nested struct
static void test_roundtrip_nested(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_nested.json");

    const NestedItem original{"Hero", Tag{"class", "warrior"}};

    GmSave::save(tmp.path, original);
    const NestedItem loaded = GmSave::load<NestedItem>(tmp.path);

    tr.check(loaded == original, "T02 round-trip nested struct");
}

// T03 — save + load round-trip: std::vector<int> field
static void test_roundtrip_vector(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_vector.json");

    const ListItem original{"High Scores", {980, 750, 620, 500}};

    GmSave::save(tmp.path, original);
    const ListItem loaded = GmSave::load<ListItem>(tmp.path);

    tr.check(loaded == original, "T03 round-trip std::vector field");
}

// T04 — save + load round-trip: std::optional<int> present
static void test_roundtrip_optional_present(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_opt_present.json");

    const OptItem original{"Wizard", 40};

    GmSave::save(tmp.path, original);
    const OptItem loaded = GmSave::load<OptItem>(tmp.path);

    tr.check(loaded == original, "T04 round-trip std::optional (present)");
}

// T05 — save + load round-trip: std::optional<int> absent (nullopt)
static void test_roundtrip_optional_absent(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_opt_absent.json");

    const OptItem original{"Rogue", std::nullopt};

    GmSave::save(tmp.path, original);
    const OptItem loaded = GmSave::load<OptItem>(tmp.path);

    tr.check(loaded == original, "T05 round-trip std::optional (absent / nullopt)");
}

// T06 — try_load returns false on missing file (must not throw)
static void test_tryload_missing_file(TestRunner& tr)
{
    FlatItem out{"default", 0, 0};
    const bool result = GmSave::try_load("_nonexistent_file_xyz_12345.json", out);

    tr.check(!result, "T06 try_load returns false on missing file");
    tr.check(out.name == "default", "T06 try_load leaves out unchanged on failure");
}

// T07 — try_load returns false on malformed JSON (must not throw)
static void test_tryload_malformed_json(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_corrupt.json");

    // Write invalid JSON manually
    {
        std::ofstream f(tmp.path);
        f << "{ this is not valid json !!!";
    }

    FlatItem out{"default", 0, 0};
    const bool result = GmSave::try_load(tmp.path, out);

    tr.check(!result, "T07 try_load returns false on malformed JSON");
    tr.check(out.name == "default", "T07 try_load leaves out unchanged on malformed JSON");
}

// T08 — save_versioned + load_versioned round-trip (correct version)
static void test_versioned_roundtrip(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_versioned.json");

    const FlatItem original{"Shield", 80, 1};

    GmSave::save_versioned(tmp.path, original, /*version=*/2);
    tr.checkNoThrow("T08 save_versioned completes without exception", [&]{
        GmSave::save_versioned(tmp.path, original, 2);
    });

    const FlatItem loaded = GmSave::load_versioned<FlatItem>(tmp.path, 2);
    tr.check(loaded == original, "T08 load_versioned round-trip (correct version)");
}

// T09 — load_versioned throws VersionMismatchError on wrong version
static void test_versioned_mismatch(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_vmismatch.json");

    const FlatItem original{"Axe", 200, 1};
    GmSave::save_versioned(tmp.path, original, /*version=*/1);

    tr.checkThrows<GmSave::VersionMismatchError>(
        "T09 load_versioned throws VersionMismatchError on wrong version",
        [&]{ GmSave::load_versioned<FlatItem>(tmp.path, /*expected=*/3); }
    );
}

// T09b — VersionMismatchError carries the correct version numbers
static void test_versioned_mismatch_fields(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_vmismatch_fields.json");

    const FlatItem original{"Spear", 120, 2};
    GmSave::save_versioned(tmp.path, original, /*version=*/1);

    try {
        GmSave::load_versioned<FlatItem>(tmp.path, /*expected=*/5);
        tr.check(false, "T09b VersionMismatchError fields — exception not thrown");
    }
    catch (const GmSave::VersionMismatchError& e) {
        tr.check(e.expected_version == 5 && e.found_version == 1,
                 "T09b VersionMismatchError carries correct expected/found versions");
    }
    catch (...) {
        tr.check(false, "T09b VersionMismatchError fields — wrong exception type");
    }
}

// T10 — load_versioned throws JsonParseError when _version field is missing
static void test_versioned_missing_version_field(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_nover.json");

    // Write a plain (non-versioned) JSON file
    const FlatItem original{"Mace", 90, 2};
    GmSave::save(tmp.path, original);   // no _version envelope

    tr.checkThrows<GmSave::JsonParseError>(
        "T10 load_versioned throws JsonParseError on missing _version field",
        [&]{ GmSave::load_versioned<FlatItem>(tmp.path, 1); }
    );
}

// T11 — peek_version returns correct version
static void test_peek_version_present(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_peek.json");

    const FlatItem original{"Staff", 60, 1};
    GmSave::save_versioned(tmp.path, original, /*version=*/7);

    const std::optional<uint32_t> ver = GmSave::peek_version(tmp.path);

    tr.check(ver.has_value(),   "T11 peek_version returns a value");
    tr.check(*ver == 7u,        "T11 peek_version returns correct version number (7)");
}

// T12 — peek_version returns nullopt on plain (non-versioned) file
static void test_peek_version_absent(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_peek_plain.json");

    const FlatItem original{"Dagger", 40, 5};
    GmSave::save(tmp.path, original);   // no _version envelope

    const std::optional<uint32_t> ver = GmSave::peek_version(tmp.path);

    tr.check(!ver.has_value(),
             "T12 peek_version returns nullopt on non-versioned file");
}

// T13 — FileReadError thrown on non-existent file
static void test_fileread_error(TestRunner& tr)
{
    tr.checkThrows<GmSave::FileReadError>(
        "T13 load throws FileReadError on non-existent file",
        []{ GmSave::load<FlatItem>("_nonexistent_file_xyz_99999.json"); }
    );
}

// T14 — FileWriteError thrown on unwritable path
static void test_filewrite_error(TestRunner& tr)
{
    // A path whose parent directory does not exist will reliably fail.
    tr.checkThrows<GmSave::FileWriteError>(
        "T14 save throws FileWriteError on unwritable path",
        []{
            const FlatItem item{"test", 1, 1};
            GmSave::save("_nonexistent_dir_abc123/output.json", item);
        }
    );
}

// T15 — JsonParseError thrown on invalid JSON content
static void test_jsonparse_error(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_invalid.json");

    {
        std::ofstream f(tmp.path);
        f << "<<<not json at all>>>";
    }

    tr.checkThrows<GmSave::JsonParseError>(
        "T15 load throws JsonParseError on invalid JSON content",
        [&]{ GmSave::load<FlatItem>(tmp.path); }
    );
}

// T16 — Compact output (indent = -1) produces single-line JSON
static void test_compact_output(TestRunner& tr)
{
    TempFile tmp("_gmSave_test_compact.json");

    const FlatItem original{"Bow", 70, 10};
    GmSave::save(tmp.path, original, /*indent=*/-1);

    // Read the file back as raw text and verify it is a single line
    std::ifstream f(tmp.path);
    std::string line1, line2;
    const bool has_line1 = static_cast<bool>(std::getline(f, line1));
    const bool has_line2 = static_cast<bool>(std::getline(f, line2));

    tr.check(has_line1,  "T16 compact output: file is not empty");
    tr.check(!has_line2, "T16 compact output: file contains exactly one line (no indent)");

    // Also verify round-trip still works with compact output
    const FlatItem loaded = GmSave::load<FlatItem>(tmp.path);
    tr.check(loaded == original, "T16 compact output: round-trip still correct");
}

// =============================================================================
// Entry point
// =============================================================================

int main()
{
    // Create a file logger via gmLog — all test results go to this file.
    gmLog::GmLogger log = gmLog::LoggerFactory::create_file_logger(
        "gmSaveTest",
        "test_gmSave_out.log",
        gmLog::LogLevel::DEBUG
    );

    logInfo(log, "======================================================");
    logInfo(log, "  gmSave Unit Tests — Phase 11");
    logInfo(log, "======================================================");

    TestRunner tr{log};

    // --- run all test groups -------------------------------------------------
    test_roundtrip_flat             (tr);
    test_roundtrip_nested           (tr);
    test_roundtrip_vector           (tr);
    test_roundtrip_optional_present (tr);
    test_roundtrip_optional_absent  (tr);
    test_tryload_missing_file       (tr);
    test_tryload_malformed_json     (tr);
    test_versioned_roundtrip        (tr);
    test_versioned_mismatch         (tr);
    test_versioned_mismatch_fields  (tr);
    test_versioned_missing_version_field(tr);
    test_peek_version_present       (tr);
    test_peek_version_absent        (tr);
    test_fileread_error             (tr);
    test_filewrite_error            (tr);
    test_jsonparse_error            (tr);
    test_compact_output             (tr);

    // --- summary -------------------------------------------------------------
    tr.summary();

    logInfo(log, "======================================================");

    return (tr.failed == 0) ? 0 : 1;
}
