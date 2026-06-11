# gmLog – Development Plan

## Phase 1 – Architecture & File Structure
- [x] Define `LogLevel` enum + `levelToString` / `levelFromString`
- [x] Define `LogRecord` struct
- [x] Define `LoggerConfig` struct
- [x] Define `ILogSink` abstract interface
- [x] Define `ILogFormatter` abstract interface
- [x] Define `ILogDispatcher` abstract interface
- [x] Design `Logger` class (non-copyable, move-constructible)
- [x] Design `LoggerFactory` static helpers
- [x] Design `SyncDispatcher` (V1 dispatcher)
- [x] Design `JsonFormatter` (JSON Lines format)
- [x] Design `StdoutSink` and `FileSink`
- [x] Design `LogMacros.hpp` with compile-time level filtering
- [x] Write Doxygen comments on all declarations
- [x] Create all 13 header files
- [x] Create `PLAN.md`

---

## Phase 2 – Source Implementations
- [x] Implement `LogLevel.cpp` — `levelToString`, `levelFromString`
- [x] Implement `Logger.cpp` — constructor, destructor (flush), `log()`, convenience methods
- [x] Implement `LoggerFactory.cpp` — `createStdoutLogger`, `createFileLogger`
- [x] Implement `sinks/StdoutSink.cpp` — `write`, `flush`
- [x] Implement `sinks/FileSink.cpp` — constructor (append mode), destructor, `write`, `flush`
- [x] Implement `formatters/JsonFormatter.cpp` — `format`, `escapeJsonString`, `formatTimestamp`
- [x] Implement `dispatchers/SyncDispatcher.cpp` — constructor, `dispatch` (mutex + format + write), `flush`

---

## Phase 3 – Documentation
- [x] Create `gmLog_API.md` (usage manual)
- [ ] Configure `Doxyfile` for gmLog
- [ ] Generate Doxygen HTML docs

---

## Phase 4 – Unit Tests
- [ ] Level filtering: `setLevel(Warning)` suppresses Debug and Info
- [ ] `isEnabled()` returns correct values per level
- [ ] `levelToString` returns correct labels for all 6 levels
- [ ] `levelFromString` parses correctly (case-insensitive)
- [ ] `levelFromString` returns `false` for unknown strings
- [ ] `StdoutSink::write` outputs line to stdout
- [ ] `FileSink` creates file if it does not exist
- [ ] `FileSink` appends to existing file
- [ ] `FileSink` throws `std::runtime_error` on invalid path
- [ ] `JsonFormatter::escapeJsonString` handles `"`, `\`, `\n`, `\r`, `\t`
- [ ] `JsonFormatter::escapeJsonString` handles control characters (< 0x20)
- [ ] `JsonFormatter::format` produces valid JSON with all fields
- [ ] `JsonFormatter::format` omits `file`/`line`/`function` when absent
- [ ] `SyncDispatcher::dispatch` acquires mutex (no interleaved output under 2 threads)
- [ ] `Logger::log` with level below `minLevel` does not call dispatcher
- [ ] `Logger::log` populates `LogRecord` source location correctly
- [ ] `Logger::critical` flushes dispatcher after writing
- [ ] Lazy-evaluation template methods do not evaluate factory when level is off
- [ ] `logDebug` injects `__FILE__`, `__LINE__`, `__func__`
- [ ] `logDebug` compiled out when `LOG_COMPILED_LEVEL > LOG_LEVEL_DEBUG`
- [ ] `logCritic` calls `flush()` after logging
- [ ] Two independent loggers writing to two different files simultaneously
- [ ] Two threads writing to the same logger produce non-interleaved output

---

## Phase 5 – Examples
- [ ] `examples/basic.cpp` — stdout logger, file logger, multiple loggers, macros

---

## Phase 6 – Future Extensions (not V1)
- [ ] `AsyncDispatcher` — queue + worker thread + condition variable + stop flag
- [ ] `RotatingFileSink` — rotation by file size
- [ ] `DailyFileSink` — rotation by date
- [ ] `PlainTextFormatter`
- [ ] `LoggerRegistry` — central registry for named loggers
- [ ] `LoggerConfigLoader` — load logger config from JSON file
- [ ] `SerialSink` / `IByteStream` — OS-specific serial output as a separate plugin
