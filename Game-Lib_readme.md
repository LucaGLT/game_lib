# game_lib

Collection of C++17 libraries for building tabletop game applications.  
All libraries are self-contained, dependency-free (C++17 stdlib only), and designed to compose cleanly with each other.

---

## Libraries at a Glance

| Library | Namespace | Status | Purpose |
| ------- | --------- | ------ | ------- |
| `gmLog` | `GmLog` | Production | Structured JSON Lines logging |
| `gmSave` | `GmSave` | Production | JSON persistence with versioning |
| `gmMap` | `GameMap` | In Development | Generic topology-agnostic game map |
| `gmDispatch` | `GmDispatch` | Production | Subscription-based event/message routing |
| `gmAlea` | `gmAlea` | Production | Token deck management — `GmDeck`, `GmCompDeck`, `GmDice` |

---

## gmLog — Logging Library

**Namespace:** `GmLog` | **API:** `gmLog/gmLog_API.md`

Lightweight multi-logger library with JSON Lines output, runtime and compile-time level filtering, and thread-safe dispatch.

**Key features:**

- Multiple independent `Logger` instances, each with its own `LogLevel` threshold
- JSON Lines output format — machine-readable and human-readable at once
- Compile-time level stripping via `LOG_COMPILED_LEVEL` macro (zero overhead in production builds)
- Thread-safe `SyncDispatcher` with `std::mutex`
- Extensible via `ILogSink`, `ILogFormatter`, `ILogDispatcher` interfaces
- Factory helpers: `LoggerFactory::createFileLogger()`, `createStdoutLogger()`, etc.

**Main classes:** `Logger`, `LoggerConfig`, `LogRecord`, `LogLevel`, `LoggerFactory`, `SyncDispatcher`, `StdoutSink`, `FileSink`, `JsonFormatter`

---

## gmSave — JSON Persistence Library

**Namespace:** `GmSave` | **API:** `gmSave/gmSave_API.md`

Generic JSON persistence layer for any C++ struct, requiring only `to_json` / `from_json` free functions on the target type.

**Key features:**

- Fully generic: works with any struct that implements the two-function contract
- Versioned saves with built-in `_version` envelope and `peek_version()` for upgrade paths
- Non-throwing `try_load()` variant — safe for startup loading without try/catch boilerplate
- Supports vectors, `std::optional`, nested structs, heterogeneous field types
- Configurable indentation; compact mode available
- Comprehensive exception hierarchy: `EFileWriteError`, `EFileReadError`, `EJsonParseError`, `EVersionMismatchError`

**Main functions:** `save()`, `load()`, `try_load()`, `save_versioned()`, `load_versioned()`, `peek_version()`

---

## gmMap — Tabletop Game Map Library

**Namespace:** `GameMap` | **API:** `gmMap/gmMap_API.md`

Topology-agnostic, graph-based game map engine. Models board state as a graph of *locations* connected by *adjacency edges*, grouped into named *tiles* (zones, floors, regions), with typed *items* and *metadata* on every node.

**Key features:**

- No forced grid or coordinate system — topology is expressed purely via adjacency
- Generic `ItemT` template parameter; no constraints on item domain
- Named tiles group locations into zones, regions or floors
- Directed and bidirectional adjacency — supports irregular maps and portal connections
- Per-node and per-tile `Metadata` (`unordered_map<string, MetadataValue>`)
- Safe by default: dedicated exception hierarchy for all invalid operations

**Main types:** `gmMap<ItemT>`, `LocationId`, `TileId`, `EntityUid`, `UidRef`, `MetadataValue`

---

## gmDispatch — Message / Event Dispatch Library

**Namespace:** `GmDispatch` | **API:** `gmDispatch/gmDispatch_API.md`

Lightweight 1:N subscription-based event bus for decoupling game engine components (e.g. CoreEngine ↔ UI ↔ AI ↔ Logger) without compile-time coupling.

**Key features:**

- 1:N routing: one `dispatch()` call fans out to all subscribed channels
- Runtime subscribe/unsubscribe with no rebuild required
- Generic `std::any` payload — no template explosion; receiver casts to its expected type
- `EventBusChannel` for in-process callback delivery (main usage)
- `StdoutChannel` + `JsonSerializer` for debug/file output
- Thread-safe `SyncDispatcher` with `std::recursive_mutex` (supports re-entrant request/response patterns)
- `PatternRouter` supports wildcard subscriptions (e.g. `"eng.*"`)

**Main classes:** `Dispatcher`, `DispatcherFactory`, `Envelope`, `EventBusChannel`, `SyncDispatcher`, `SyncRouter`, `JsonSerializer`

---

## gmAlea — Token Deck & Dice Library

**Namespace:** `gmAlea` | **API:** `gmAlea/gmDeck_API.md`

High-performance, deterministic token deck with `uint32_t` IDs. Suitable for any draw-and-shuffle mechanic including probability/fate decks with repeated card types.

**Key features:**

- `uint32_t` IDs: 4 bytes per token vs 50–100+ bytes for strings; excellent cache locality
- Deterministic shuffling via optional seed — reproducible for tests and replays
- `allow_duplicates = true` mode for probability decks (e.g. 8× Success + 2× Failure → 80/20 draw odds)
- `draw_one()`, `draw_many(k)`, `draw_specific(id)` — draw from top or by specific ID
- `push_back()` / `push_front()` — add tokens without reshuffle
- `auto_shuffle` flag for zones that must not shuffle on construction

**Main class:** `GmDeck` — exceptions: `EAleaDeckEmptyError`, `EAleaDuplicateTokenIdError`, `EAleaTokenNotFoundError`, `EAleaInvalidDrawCountError`

---

## gmAlea — Composite Deck Orchestrator

**Namespace:** `gmAlea` | **API:** `gmAlea/gmCompDeck_API.md`

Multi-zone card lifecycle manager built on top of `GmDeck`. Models the complete state of one game entity's cards across six distinct zones, guaranteeing that every token ID lives in exactly one zone at all times.

**Key features:**

- Six zones: **Main Deck** (shufflable) · **Hand** · **Play Area** · **Memory** · **Discard Pile** (order preserved, no shuffle) · **Banish Zone** (insert-only, permanent)
- Uniqueness invariant: every token is in exactly one zone — enforced by the orchestrator
- Compile-time zone safety via `static_assert` in `PolicyBasedDeck<Policy>`: shuffling a discard pile or drawing from a banish zone is a *compile error*, not a runtime bug
- Atomic cross-zone moves: `draw_to_hand()`, `play_card()`, `resolve_card()`, `discard_from_hand()`, `take_from_discard()`, `banish()`, `reshuffle_discard_into_deck()`, and more
- `locate(id)` — find which zone currently holds any token
- Standalone zone types usable without `GmCompDeck`: `MainDeck`, `CardHand`, `PlayArea`, `MemoryZone`, `DiscardPile`, `BanishZone`
- Custom policies supported via user-defined `struct` with three `constexpr bool` flags

**Main class:** `GmCompDeck` — zone aliases: `MainDeck`, `CardHand`, `PlayArea`, `MemoryZone`, `DiscardPile`, `BanishZone`

---

## Build Commands (Windows + clang++)

```powershell
# gmDeck v2 unit tests
clang++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/tests/test_gmDeck_v2.cpp -o test_gmDeck_v2.exe ; ./test_gmDeck_v2.exe

# gmCompDeck integration tests
clang++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/gmCompDeck.cpp gmDeck/tests/test_gmCompDeck.cpp -o test_gmCompDeck.exe ; ./test_gmCompDeck.exe

# gmMap tests (phases 2-4)
clang++ -std=c++17 -I. gmMap/tests/test_phases_2_4.cpp gmLog/*.cpp gmLog/sinks/*.cpp gmLog/formatters/*.cpp gmLog/dispatchers/*.cpp -o test_phases_2_4.exe ; ./test_phases_2_4.exe

# gmSave tests
clang++ -std=c++17 -I. gmSave/tests/test_gmSave.cpp gmSave/gmSave.cpp gmLog/LogLevel.cpp gmLog/Logger.cpp gmLog/LoggerFactory.cpp gmLog/sinks/StdoutSink.cpp gmLog/sinks/FileSink.cpp gmLog/formatters/JsonFormatter.cpp gmLog/dispatchers/SyncDispatcher.cpp -o test_gmSave.exe ; ./test_gmSave.exe
```

---

## Development Status

| Library | Phase | Notes |
| ------- | ----- | ----- |
| `gmLog` | ✅ Complete | Production-ready |
| `gmSave` | ✅ Complete | Production-ready |
| `gmDeck` + `gmCompDeck` | ✅ Complete | All tests passing (12+12) |
| `gmDispatch` | ✅ Complete | Phase 4 done, examples available |
| `gmMap` | 🔄 Phase 10/11 | Core complete, final phases in progress |
