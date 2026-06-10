# gmLog – Logging Library

**Version:** 1.0
**Status:** Production (V1)
**Language:** C++17 Standard
**Namespace:** `GmLog`
**Headers:** see [File Structure](#file-structure)

---

## Table of Contents

- [Overview](#overview)
- [Design Philosophy](#design-philosophy)
- [Requirements & Setup](#requirements--setup)
- [File Structure](#file-structure)
- [Architecture](#architecture)
- [API Reference](#api-reference)
  - [LogLevel](#loglevel)
  - [LogRecord](#logrecord)
  - [LoggerConfig](#loggerconfig)
  - [ILogSink](#ilogsink)
  - [ILogFormatter](#ilogformatter)
  - [ILogDispatcher](#ilogdispatcher)
  - [SyncDispatcher](#syncdispatcher)
  - [StdoutSink](#stdoutsink)
  - [FileSink](#filesink)
  - [JsonFormatter](#jsonformatter)
  - [Logger](#logger)
  - [LoggerFactory](#loggerfactory)
  - [LogMacros](#logmacros)
- [JSON Output Format](#json-output-format)
- [Usage Examples](#usage-examples)
- [Thread Safety](#thread-safety)
- [Compile-Time Level Filtering](#compile-time-level-filtering)
- [Future Extensions](#future-extensions)

---

## Overview

**gmLog** is a lightweight, multi-logger C++17 logging library designed for
desktop game and application development.  It supports multiple independent
loggers, two built-in output channels (stdout and file), JSON Lines output
format, runtime and compile-time level filtering, and thread-safe synchronous
dispatch.

### Key Features

| Feature | Detail |
|---|---|
| **Multiple loggers** | Each logger has its own name, level, and output channel |
| **JSON Lines format** | One JSON object per line; machine- and human-readable |
| **Runtime level filter** | `setLevel()` at any time; O(1) check per call |
| **Compile-time stripping** | `LOG_COMPILED_LEVEL` removes call sites from binary |
| **Lazy evaluation** | Template overloads avoid string construction when level is off |
| **Source location** | `__FILE__` / `__LINE__` / `__func__` injected via macros |
| **Thread safe** | `SyncDispatcher` serialises writes with `std::mutex` |
| **Extensible** | New sinks, formatters, and dispatchers via interfaces |
| **Standard C++17** | No OS-specific APIs in the core |

---

## Design Philosophy

```
Codice applicativo
      ↓
logInfo(db, "Messaggio")
      ↓
Logger          ← filtra livello, crea LogRecord
      ↓
ILogDispatcher  ← quando e come scrivere
      ↓
ILogFormatter   ← come formattare
      ↓
ILogSink        ← dove scrivere
```

- **The Logger does not write directly.**  It creates a `LogRecord` and hands
  it to the dispatcher.
- **The dispatcher owns the formatter and the sink.**  Swapping dispatcher type
  (sync → async) requires no Logger API changes.
- **Sinks are ignorant of formatting.**  They receive a ready-to-write string.
- **Configuration is separate from logic.**  `LoggerConfig` collects all
  tunable parameters; the Logger constructor accepts it as a value.

---

## Requirements & Setup

- C++17-compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard library headers: `<iostream>`, `<fstream>`, `<string>`, `<memory>`,
  `<mutex>`, `<chrono>`, `<sstream>`, `<iomanip>`, `<ctime>`, `<algorithm>`
- No external dependencies

Include the desired headers in your source files:

```cpp
#include "Logger.hpp"
#include "LoggerFactory.hpp"
#include "macros/LogMacros.hpp"
```

---

## File Structure

```
gmLog/
├── LogLevel.hpp / .cpp
├── LogRecord.hpp
├── LoggerConfig.hpp
├── ILogSink.hpp
├── ILogFormatter.hpp
├── ILogDispatcher.hpp
├── Logger.hpp / .cpp
├── LoggerFactory.hpp / .cpp
├── sinks/
│   ├── StdoutSink.hpp / .cpp
│   └── FileSink.hpp / .cpp
├── formatters/
│   └── JsonFormatter.hpp / .cpp
├── dispatchers/
│   └── SyncDispatcher.hpp / .cpp
└── macros/
    └── LogMacros.hpp
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│  Application code                                       │
│                                                         │
│  logInfo(db, "msg")   db.info("msg")                   │
└────────────────────┬────────────────────────────────────┘
                     │ log(level, msg, file, line, func)
                     ▼
              ┌──────────────┐
              │   Logger     │  filters level, creates LogRecord
              └──────┬───────┘
                     │ dispatch(LogRecord)
                     ▼
           ┌──────────────────────┐
           │  ILogDispatcher      │
           │  └─ SyncDispatcher   │  mutex + format + write
           └───┬──────────────────┘
               │
       ┌───────┴───────┐
       │               │
       ▼               ▼
 ILogFormatter    ILogSink
 └─JsonFormatter  ├─ StdoutSink
                  └─ FileSink
```

---

## API Reference

### LogLevel

```cpp
enum class LogLevel : int {
    Debug    = 0,
    Info     = 1,
    Warning  = 2,
    Error    = 3,
    Critical = 4,
    Off      = 5
};
```

`Off` disables all logging when set as the minimum level.

#### `levelToString()`

```cpp
const char* levelToString(LogLevel level);
```

Returns `"DEBUG"`, `"INFO"`, `"WARNING"`, `"ERROR"`, `"CRITICAL"`, `"OFF"`, or
`"UNKNOWN"`.

#### `levelFromString()`

```cpp
bool levelFromString(const std::string& str, LogLevel& out);
```

Parses a string (case-insensitive) into a `LogLevel`.  Returns `false` if the
string is not recognised; `out` is not modified on failure.

---

### LogRecord

```cpp
struct LogRecord {
    LogLevel    level       = LogLevel::Debug;
    std::string loggerName;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
    const char* file     = nullptr;
    int         line     = 0;
    const char* function = nullptr;
};
```

An immutable snapshot of one log event.  Created by `Logger::log()` and passed
through the pipeline.  Source-location fields are `nullptr` / `0` when
`LoggerConfig::enableSourceLocation` is `false` or when the call does not
originate from a macro.

---

### LoggerConfig

```cpp
struct LoggerConfig {
    std::string name;
    LogLevel    minLevel             = LogLevel::Debug;
    bool        enableSourceLocation = true;
};
```

Configuration bundle for a single logger.  Pass to the `Logger` constructor
or to a `LoggerFactory` helper.

| Field | Default | Description |
|---|---|---|
| `name` | `""` | Logger identity — appears in every log line. |
| `minLevel` | `Debug` | Events below this level are silently dropped. |
| `enableSourceLocation` | `true` | Attach `__FILE__` / `__LINE__` / `__func__` to records. |

---

### ILogSink

```cpp
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(const std::string& message) = 0;
    virtual void flush() = 0;
};
```

Abstract output channel.  Receives a fully-formatted string and writes it to
the underlying channel.  Concrete implementations: `StdoutSink`, `FileSink`.

---

### ILogFormatter

```cpp
class ILogFormatter {
public:
    virtual ~ILogFormatter() = default;
    virtual std::string format(const LogRecord& record) = 0;
};
```

Abstract formatter.  Converts a `LogRecord` into a string **without** a
trailing newline (the sink appends the line terminator).  Concrete
implementation: `JsonFormatter`.

---

### ILogDispatcher

```cpp
class ILogDispatcher {
public:
    virtual ~ILogDispatcher() = default;
    virtual void dispatch(const LogRecord& record) = 0;
    virtual void flush() = 0;
};
```

Abstract dispatcher — the key extension point for future async logging.
Concrete implementation: `SyncDispatcher`.

---

### SyncDispatcher

```cpp
SyncDispatcher(std::unique_ptr<ILogSink>      sink,
               std::unique_ptr<ILogFormatter> formatter);
void dispatch(const LogRecord& record) override;
void flush() override;
```

Thread-safe synchronous dispatcher.  Acquires an internal `std::mutex`, calls
`formatter_->format(record)`, then calls `sink_->write(result)`.

| Parameter | Description |
|---|---|
| `sink` | Ownership-transferred sink. |
| `formatter` | Ownership-transferred formatter. |

---

### StdoutSink

```cpp
void write(const std::string& message) override;  // std::cout << message << std::endl
void flush() override;                             // std::cout.flush()
```

Writes one line to `std::cout` followed by `std::endl` (implicit flush).

---

### FileSink

```cpp
explicit FileSink(const std::string& filePath);   // opens in append mode
void write(const std::string& message) override;  // appends line + '\n'
void flush() override;                             // flushes file buffer
```

Appends one line per call to the log file.  The file is created if it does not
exist and appended to if it does.

**Throws:** `std::runtime_error` at construction if the file cannot be opened.

---

### JsonFormatter

```cpp
std::string format(const LogRecord& record) override;
static std::string escapeJsonString(const std::string& value);
```

#### `format()`

Produces a single-line JSON object.  Source-location fields are included only
when `record.file != nullptr`.

#### `escapeJsonString()`

Escapes the following characters for safe JSON embedding:

| Character | Escaped |
|---|---|
| `\` | `\\` |
| `"` | `\"` |
| newline | `\n` |
| carriage return | `\r` |
| tab | `\t` |
| other `< 0x20` | `\uXXXX` |

---

### Logger

#### Constructor

```cpp
Logger(LoggerConfig config, std::unique_ptr<ILogDispatcher> dispatcher);
```

Logger is **non-copyable** and **move-constructible**.  The destructor calls
`flush()` automatically.

#### Configuration

```cpp
const std::string& name()    const;
LogLevel           minLevel() const;
void               setLevel(LogLevel level);
bool               isEnabled(LogLevel level) const;
```

#### Core log method

```cpp
void log(LogLevel           level,
         const std::string& message,
         const char*        file     = nullptr,
         int                line     = 0,
         const char*        function = nullptr);
```

Creates a `LogRecord` and dispatches it.  Returns immediately if
`level < minLevel()`.

#### Convenience methods (string overloads)

```cpp
void debug   (const std::string& message);
void info    (const std::string& message);
void warning (const std::string& message);
void error   (const std::string& message);
void critical(const std::string& message);  // also calls flush()
```

#### Lazy-evaluation overloads

```cpp
template <typename F> void debug   (F&& factory);
template <typename F> void info    (F&& factory);
template <typename F> void warning (F&& factory);
template <typename F> void error   (F&& factory);
template <typename F> void critical(F&& factory);  // also calls flush()
```

`F` must be callable with signature `std::string()`.  The factory is invoked
only when the level is enabled.

#### Flush

```cpp
void flush();
```

Forwards the flush call to the dispatcher and its sink.

---

### LoggerFactory

```cpp
static Logger createStdoutLogger(
    const std::string& name,
    LogLevel           level                = LogLevel::Debug,
    bool               enableSourceLocation = true);

static Logger createFileLogger(
    const std::string& name,
    const std::string& filePath,
    LogLevel           level                = LogLevel::Debug,
    bool               enableSourceLocation = true);
```

Both methods assemble `SyncDispatcher → {StdoutSink | FileSink} + JsonFormatter`
and return a ready-to-use `Logger`.

**Throws:** `std::runtime_error` from `createFileLogger` if the file cannot be
opened.

---

### LogMacros

Include `macros/LogMacros.hpp` to use the macros.

```cpp
logDebug   (logger, expr)
logInfo    (logger, expr)
logWarn (logger, expr)
logErr   (logger, expr)
logCritic(logger, expr)
```

- `logger` — a `GmLog::Logger` lvalue.
- `expr` — any expression returning `std::string`; evaluated only when the
  level is active at runtime.
- `__FILE__`, `__LINE__`, `__func__` are injected automatically.
- `logCritic` calls `logger.flush()` after writing.

#### Compile-time constants

```cpp
#define LOG_LEVEL_DEBUG    0
#define LOG_LEVEL_INFO     1
#define LOG_LEVEL_WARNING  2
#define LOG_LEVEL_ERROR    3
#define LOG_LEVEL_CRITICAL 4
#define LOG_LEVEL_OFF      5
```

---

## JSON Output Format

### Minimal (source location absent)

```json
{"time":"2026-06-10T17:42:11.235","logger":"Database","level":"INFO","message":"System ready"}
```

### Extended (source location present)

```json
{"time":"2026-06-10T17:42:11.235","logger":"Database","level":"ERROR","file":"Database.cpp","line":87,"function":"connect","message":"Connection failed"}
```

| Field | Type | Always present |
|---|---|---|
| `time` | string (ISO-8601 UTC, ms precision) | Yes |
| `logger` | string | Yes |
| `level` | string | Yes |
| `file` | string | Only when source location is enabled |
| `line` | number | Only when source location is enabled |
| `function` | string | Only when source location is enabled |
| `message` | string | Yes |

---

## Usage Examples

### Quick start — single stdout logger

```cpp
#include "LoggerFactory.hpp"
#include "macros/LogMacros.hpp"

int main() {
    auto log = GmLog::LoggerFactory::createStdoutLogger("App");

    logInfo (log, "Application started");
    logDebug(log, "Initialising subsystems");
    logErr(log, "Config file not found");
    logCritic(log, "Fatal: cannot allocate memory");
}
```

---

### Multiple independent loggers

```cpp
#include "LoggerFactory.hpp"
#include "macros/LogMacros.hpp"

auto db  = GmLog::LoggerFactory::createFileLogger(
    "Database", "db.log",      GmLog::LogLevel::Debug);

auto net = GmLog::LoggerFactory::createFileLogger(
    "Network",  "network.log", GmLog::LogLevel::Info);

auto ui  = GmLog::LoggerFactory::createStdoutLogger(
    "UI",                      GmLog::LogLevel::Warning);

logDebug  (db,  "Opening connection");   // written to db.log
logInfo   (net, "Packet received");      // written to network.log
logWarn(ui,  "Render lag detected");  // written to stdout
logDebug  (ui,  "Frame rendered");       // suppressed — UI level is Warning
```

---

### Direct construction (without factory)

```cpp
#include "Logger.hpp"
#include "LoggerConfig.hpp"
#include "dispatchers/SyncDispatcher.hpp"
#include "formatters/JsonFormatter.hpp"
#include "sinks/FileSink.hpp"

GmLog::LoggerConfig cfg;
cfg.name                 = "GameEngine";
cfg.minLevel             = GmLog::LogLevel::Info;
cfg.enableSourceLocation = true;

GmLog::Logger engine(
    cfg,
    std::make_unique<GmLog::SyncDispatcher>(
        std::make_unique<GmLog::FileSink>("engine.log"),
        std::make_unique<GmLog::JsonFormatter>()
    )
);

engine.info("Engine initialised");
engine.warning("Asset cache miss");
```

---

### Runtime level change

```cpp
auto log = GmLog::LoggerFactory::createStdoutLogger("App");

log.setLevel(GmLog::LogLevel::Warning);   // Debug and Info are now suppressed
log.info("This will not appear");
log.warning("This will appear");

log.setLevel(GmLog::LogLevel::Debug);     // Restore full verbosity
log.debug("Debug output re-enabled");
```

---

### Lazy evaluation (expensive string construction)

```cpp
// The lambda is NOT called when Debug is disabled at runtime.
log.debug([&]{
    return "Entity positions: " + serializeAllEntities(world);
});

// Macro version (also lazy + injects source location)
logDebug(log, "Entity positions: " + serializeAllEntities(world));
```

---

### Escaping special characters

`JsonFormatter::escapeJsonString` is called automatically on all fields.  The
following message is serialised correctly:

```cpp
log.error("Field \"name\" contains illegal char: \t<tab>");
// → {"message":"Field \"name\" contains illegal char: \t<tab>"}
```

---

### Manual flush before exit

```cpp
auto log = GmLog::LoggerFactory::createFileLogger("App", "app.log");
// ... log events ...
log.flush();  // ensure all buffered data is written before the process exits
// Also called automatically in Logger's destructor
```

---

### `try_load` pattern with gmSave + gmLog

```cpp
Config cfg;
if (!GmSave::try_load("config.json", cfg)) {
    logWarn(log, "Config not found — using defaults");
    cfg = Config{"default_map", 2};
}
```

---

## Thread Safety

`SyncDispatcher` serialises all calls to `format()` + `write()` under a single
`std::mutex`.  Two threads writing to the same logger will never interleave
their output lines.

Each logger must be accessed through a reference or pointer shared between
threads.  The logger itself does not need external locking.

**Do not share a Logger instance across threads via copies** — Logger is
non-copyable; sharing is done via `std::shared_ptr<Logger>` or by passing
references.

---

## Compile-Time Level Filtering

Define `LOG_COMPILED_LEVEL` before including `LogMacros.hpp` or in your build
system to strip specific levels from the binary at compile time:

```cmake
# CMakeLists.txt — strip Debug and Info from Release builds
target_compile_definitions(myapp PRIVATE
    $<$<CONFIG:Release>:LOG_COMPILED_LEVEL=LOG_LEVEL_WARNING>
)
```

Result: every `logDebug` and `logInfo` call is replaced by
`do{}while(0)` — zero runtime overhead, zero binary size impact.

| `LOG_COMPILED_LEVEL` | Compiled in |
|---|---|
| `LOG_LEVEL_DEBUG` (default) | All levels |
| `LOG_LEVEL_INFO` | Info, Warning, Error, Critical |
| `LOG_LEVEL_WARNING` | Warning, Error, Critical |
| `LOG_LEVEL_ERROR` | Error, Critical |
| `LOG_LEVEL_CRITICAL` | Critical only |
| `LOG_LEVEL_OFF` | Nothing |

---

## Future Extensions

| Extension | Description |
|---|---|
| `AsyncDispatcher` | Queue + worker thread + `condition_variable`; drop-in replacement for `SyncDispatcher` |
| `RotatingFileSink` | Rotate by file size; keep last N backups |
| `DailyFileSink` | Rotate at midnight; date-stamped filenames |
| `PlainTextFormatter` | Human-friendly single-line format |
| `LoggerRegistry` | Central named-logger store for global access |
| `LoggerConfigLoader` | Load logger configuration from a JSON file (via gmSave) |
| `SerialSink` | OS-specific serial output; kept outside the core as a separate plugin |
