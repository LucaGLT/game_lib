# gmDispatch – Development Plan

**Version:** 1.0
**Status:** Phase 4 – Complete ✅
**Language:** C++17 Standard
**Namespace:** `GmDispatch`

---

## Goal

Generic message/event dispatch library for C++17 game engine components.
Decouples producers (e.g. CoreEngine) from consumers (e.g. UI, AI, logging bridge)
via a subscription-based, multi-channel routing system.

Key differences from **gmLog** (which inspired the architecture):

| Aspect          | gmLog                     | gmDispatch                            |
|-----------------|---------------------------|---------------------------------------|
| Message         | `LogRecord` (fixed fields)| `Envelope` (generic `std::any` payload)|
| Destinations    | 1 sink per logger         | N channels, runtime subscription      |
| Routing         | none (fixed at build)     | `IRouter` — typeId-based, dynamic     |
| Message ID      | absent                    | optional `messageId` field             |
| Source field    | `loggerName`              | explicit `source` field                |
| Serialization   | always (JsonFormatter)    | optional per-channel                   |

---

## Architecture

```
Application code
      │ dispatch(Envelope)
      ▼
Dispatcher  (facade — analogous to Logger)
      │ IDispatcher::dispatch()
      ▼
SyncDispatcher  (owns IRouter + mutex)
      │ IRouter::route()
      ▼
SyncRouter  (subscription map: typeId → list<IChannel>)
      │ IChannel::send() ×N
      ├─► EventBusChannel   — in-process std::function callbacks
      └─► StdoutChannel     — serializes to std::cout  (debug)
              │ ISerializer::serialize()
              ▼
          JsonSerializer   — JSON Lines format
```

---

## File Structure

```
gmDispatch/
├── PLAN.md
├── gmDispatch_API.md
├── ai-instructions.md
├── Envelope.hpp              ← message container       (LogRecord analog)
├── IChannel.hpp              ← output interface        (ILogSink analog)
├── ISerializer.hpp           ← serialization interface (ILogFormatter analog)
├── IRouter.hpp               ← subscription & routing  (NEW — no gmLog analog)
├── IDispatcher.hpp           ← top-level dispatch iface(ILogDispatcher analog)
├── DispatcherConfig.hpp      ← config struct           (LoggerConfig analog)
├── Dispatcher.hpp / .cpp     ← user-facing facade      (Logger analog)
├── DispatcherFactory.hpp/.cpp← factory                 (LoggerFactory analog)
├── channels/
│   ├── EventBusChannel.hpp/.cpp   ← in-process pub/sub (V1)
│   └── StdoutChannel.hpp/.cpp     ← debug stdout       (V1)
├── serializers/
│   └── JsonSerializer.hpp/.cpp    ← JSON Lines          (V1)
├── routers/
│   └── SyncRouter.hpp/.cpp        ← synchronous 1:N    (V1)
└── dispatchers/
    └── SyncDispatcher.hpp/.cpp    ← sync + mutex       (V1)
```

---

## Development Phases

### Phase 1 — Interfaces & Stubs (current)

- [x] PLAN.md, gmDispatch_API.md, ai-instructions.md
- [x] `Envelope.hpp`
- [x] `IChannel.hpp`, `ISerializer.hpp`, `IRouter.hpp`, `IDispatcher.hpp`
- [x] `DispatcherConfig.hpp`
- [x] `Dispatcher.hpp` / `.cpp` (stub)
- [x] `DispatcherFactory.hpp` / `.cpp` (stub)
- [x] `channels/EventBusChannel.hpp` / `.cpp` (stub)
- [x] `channels/StdoutChannel.hpp` / `.cpp` (stub)
- [x] `serializers/JsonSerializer.hpp` / `.cpp` (stub)
- [x] `routers/SyncRouter.hpp` / `.cpp` (stub)
- [x] `dispatchers/SyncDispatcher.hpp` / `.cpp` (stub)

### Phase 2 — Core Implementation ✅

- [x] `SyncRouter` — exact-match + wildcard `"*"` routing
- [x] `SyncDispatcher` — mutex lock, route, flush
- [x] `EventBusChannel` — invoke registered `std::function` callbacks
- [x] `StdoutChannel` — serialize + write to `std::cout`
- [x] `JsonSerializer` — JSON Lines: timestamp, source, typeId, messageId, targets, payload
- [x] `Dispatcher` facade — auto-timestamp, delegate all calls
- [x] `DispatcherFactory` — assemble ready-to-use Dispatcher instances
- [x] Smoke test (6/6 PASS) — `tests/smoke_test.cpp`

**Notes:**
- `gmtime_s` (Windows) / `gmtime_r` (POSIX) used in `JsonSerializer` for safety.
- `IRouter::flush()` added to interface to allow `SyncDispatcher` to flush all
  channels without exposing the internal subscription map.
- `payload` field in JSON output shows the compiler-mangled type name (RTTI).
  Phase 3 will add per-type payload serializer registration.

### Phase 3 — Additional Channels ✅

- [x] `FileChannel` — append serialized envelopes to a file
- [x] `IpSocketChannel` — TCP client; length-prefixed frames; `_WIN32`/POSIX

### Phase 4 — Advanced Features ✅

- [x] `IChannel::name()` — virtual, default `""` (backward compatible)
- [x] `PatternRouter` — exact-match + `"engine.*"` prefix wildcard + `"*"` broadcast
- [x] Targeted delivery — `Envelope::targets` + `IChannel::name()` in `PatternRouter`
- [x] `AsyncDispatcher` — `std::queue` + worker thread + `condition_variable`; `flush()` blocks until drained
- [x] `LogDispatchBridge` — `GmLog::ILogDispatcher` adapter; `LogRecord → Envelope`; no modification to gmLog
- [x] `DispatcherFactory::createAsyncDispatcher()`
- [x] `DispatcherFactory::createPatternDispatcher()`
- [x] Smoke test Phase 3+4 (9/9 PASS) — `tests/smoke_test_phase3_4.cpp`

**Notes:**
- `IChannel::name()` has a non-pure default `""` — all existing channels remain compilable without changes.
- Anonymous channels (name == `""`) bypass targeted delivery — they always receive.
- `AsyncDispatcher` uses two mutexes: `queueMutex_` (queue + drain CV) and `routeMutex_` (router ops) to avoid deadlock between dispatch and subscribe.
- `LogDispatchBridge` lives in `gmDispatch/bridges/` — it depends on both libs but neither lib depends on the other.
- `IpSocketChannel` is fully implemented (Winsock2 / BSD sockets, length-prefixed TCP frames).  Link with `-lws2_32` on Windows.

---

## Key Design Decisions

1. **`Envelope::payload` is `std::any`** — zero template explosion; cast with
   `std::any_cast<T>()` at the receiving end.  No heap allocation for small types
   thanks to SBO in most STL implementations.

2. **`IChannel::send()` receives `const Envelope&`** — each channel decides
   independently whether to serialize or to use the raw struct.  No forced coupling
   to `ISerializer`.

3. **`IRouter` is a separate interface** — future routers (pattern-matching,
   priority-based) are drop-in replacements inside `SyncDispatcher` without any
   API changes.

4. **`SyncRouter` has NO internal mutex** — locking is provided entirely by the
   owning `SyncDispatcher`, avoiding double-locking.

5. **`subscribe`/`unsubscribe` on `IDispatcher`** — the `Dispatcher` facade
   exposes them transparently; no need to down-cast to `SyncDispatcher`.

6. **`"*"` typeId in `subscribe()`** means "receive all messages" (broadcast
   subscription).  V1 supports only exact-match and `"*"`.

7. **No dependency on gmLog** — fully standalone.  A future `LogDispatchBridge`
   adapter can connect the two libraries without modifying either.
