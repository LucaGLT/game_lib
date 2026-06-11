# gmDispatch – Message & Event Dispatch Library

**Version:** 1.0
**Status:** Phase 1 – Interfaces & Stubs
**Language:** C++17 Standard
**Namespace:** `GmDispatch`
**Headers:** see [File Structure](#file-structure)

---

## Table of Contents

- [Overview](#overview)
- [Design Philosophy](#design-philosophy)
- [Requirements & Setup](#requirements--setup)
- [File Structure](#file-structure)
- [Architecture](#architecture)
- [API Reference](#api-reference)
  - [Envelope](#envelope)
  - [IChannel](#ichannel)
  - [ISerializer](#iserializer)
  - [IRouter](#irouter)
  - [IDispatcher](#idispatcher)
  - [DispatcherConfig](#dispatcherconfig)
  - [Dispatcher](#dispatcher)
  - [DispatcherFactory](#dispatcherfactory)
  - [EventBusChannel](#eventbuschannel)
  - [StdoutChannel](#stdoutchannel)
  - [JsonSerializer](#jsonserializer)
  - [SyncRouter](#syncrouter)
  - [SyncDispatcher](#syncdispatcher)
- [JSON Output Format](#json-output-format)
- [Usage Examples](#usage-examples)
- [Thread Safety](#thread-safety)
- [Payload Conventions](#payload-conventions)
- [Relationship with gmLog](#relationship-with-gmlog)
- [Future Extensions](#future-extensions)

---

## Overview

**gmDispatch** is a lightweight, subscription-based C++17 message/event dispatch
library designed for game engine and application component communication.

It lets producers (CoreEngine, InputSystem, …) send typed messages (`Envelope`)
without knowing who listens.  Consumers register channels at runtime and receive
envelopes via callbacks, stdout, files, or network channels.

### Key Features

| Feature | Detail |
|---|---|
| **1:N routing** | One dispatch call fans out to all subscribed channels |
| **Runtime subscription** | `subscribe()` / `unsubscribe()` at any time |
| **Generic payload** | `std::any` payload — no template explosion |
| **In-process bus** | `EventBusChannel` delivers raw `Envelope` to callbacks |
| **Serializable** | `JsonSerializer` converts to JSON Lines for file/network channels |
| **Thread safe** | `SyncDispatcher` serialises all operations with `std::mutex` |
| **Extensible** | New channels, serializers, routers via interfaces |
| **Standard C++17** | No OS-specific APIs in the core |
| **Independent** | No dependency on gmLog or any other game_lib module |

---

## Design Philosophy

```
Application code
      ↓
bus.dispatch(envelope)
      ↓
Dispatcher          ← facade: auto-timestamp, delegates to IDispatcher
      ↓
IDispatcher         ← when and how to route
      ↓
IRouter             ← which channels receive this typeId
      ↓
IChannel ×N         ← where to deliver
   ├── EventBusChannel   ← in-process: calls std::function callbacks
   └── StdoutChannel     ← debug: ISerializer → std::cout
               ↓
          ISerializer
          └── JsonSerializer
```

- **Dispatcher does not route directly.** It delegates to `IDispatcher`.
- **IDispatcher owns the router.** Swapping sync ↔ async requires only replacing
  the object at construction — no `Dispatcher` API changes.
- **IRouter owns subscriptions.** Routing logic is independently replaceable
  (exact-match V1 → pattern-matching Phase 4).
- **IChannel decides serialisation.** In-process channels skip it entirely;
  network channels call `ISerializer` internally.
- **Configuration is separate from logic.** `DispatcherConfig` collects all
  tunable parameters.

---

## Requirements & Setup

- C++17-compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard library headers: `<any>`, `<chrono>`, `<functional>`, `<map>`,
  `<memory>`, `<mutex>`, `<string>`, `<vector>`
- No external dependencies

Include the desired headers in your source files:

```cpp
#include "Dispatcher.hpp"
#include "DispatcherFactory.hpp"
#include "channels/EventBusChannel.hpp"
```

---

## File Structure

```
gmDispatch/
├── Envelope.hpp
├── IChannel.hpp
├── ISerializer.hpp
├── IRouter.hpp
├── IDispatcher.hpp
├── DispatcherConfig.hpp
├── Dispatcher.hpp / .cpp
├── DispatcherFactory.hpp / .cpp
├── channels/
│   ├── EventBusChannel.hpp / .cpp
│   └── StdoutChannel.hpp / .cpp
├── serializers/
│   └── JsonSerializer.hpp / .cpp
├── routers/
│   └── SyncRouter.hpp / .cpp
└── dispatchers/
    └── SyncDispatcher.hpp / .cpp
```

---

## Architecture

```
┌────────────────────────────────────────────────────────────┐
│  Application code                                          │
│                                                            │
│  bus.dispatch(env)    bus.subscribe("engine.tick", ch)    │
└──────────────────────────┬─────────────────────────────────┘
                           │
                    ┌──────▼──────┐
                    │  Dispatcher │  facade: config, auto-timestamp
                    └──────┬──────┘
                           │ IDispatcher::dispatch / subscribe / unsubscribe
                    ┌──────▼──────────┐
                    │ IDispatcher     │
                    │ └─SyncDispatcher│  mutex + IRouter
                    └──────┬──────────┘
                           │ IRouter::route / subscribe / unsubscribe
                    ┌──────▼──────┐
                    │  IRouter    │
                    │  └SyncRouter│  routes_ map + wildcard "*"
                    └──────┬──────┘
             ┌─────────────┼─────────────┐
             ▼             ▼             ▼
         IChannel      IChannel      IChannel
    EventBusChannel  StdoutChannel   (future)
     callbacks only   ISerializer
                      └JsonSerializer
```

---

## API Reference

### Envelope

```cpp
struct Envelope {
    std::string                               typeId;
    std::string                               source;
    std::vector<std::string>                  targets;
    std::string                               messageId;
    std::any                                  payload;
    std::chrono::system_clock::time_point     timestamp;
};
```

An immutable-by-convention snapshot of one dispatch event.  Created by the
caller and passed through the pipeline.

| Field | Description |
|---|---|
| `typeId` | Message type for routing (e.g. `"engine.tick"`, `"input.key_pressed"`). Use `"*"` in `subscribe()` to match all. |
| `source` | Sender identity (e.g. `"CoreEngine"`). |
| `targets` | Named recipients; **empty = broadcast** to all subscribers of `typeId`. Targeted delivery is a Phase 4 feature. |
| `messageId` | Optional unique ID; empty if not required. |
| `payload` | Variable data; cast with `std::any_cast<T>(env.payload)`. |
| `timestamp` | Captured at dispatch time if `DispatcherConfig::autoTimestamp` is `true`. |

---

### IChannel

```cpp
class IChannel {
public:
    virtual ~IChannel() = default;
    virtual void send(const Envelope& envelope) = 0;
    virtual void flush() = 0;
};
```

Abstract output channel.  Receives an `Envelope` and delivers it to the
underlying medium.  Each concrete channel decides independently whether to
serialize the envelope.

---

### ISerializer

```cpp
class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual std::string serialize(const Envelope& envelope) = 0;
};
```

Abstract serializer.  Converts an `Envelope` to a string **without** a
trailing newline.  Concrete implementation: `JsonSerializer`.

---

### IRouter

```cpp
class IRouter {
public:
    virtual ~IRouter() = default;
    virtual void subscribe(const std::string& typeId,
                           std::shared_ptr<IChannel> channel) = 0;
    virtual void unsubscribe(const std::string& typeId,
                             std::shared_ptr<IChannel> channel) = 0;
    virtual void route(const Envelope& envelope) = 0;
};
```

Abstract router — the extension point for routing strategies.  V1 concrete
implementation: `SyncRouter` (exact-match + `"*"` wildcard).

> **Note:** `IRouter` has no internal mutex.  Thread safety is provided
> entirely by the owning `SyncDispatcher`.

---

### IDispatcher

```cpp
class IDispatcher {
public:
    virtual ~IDispatcher() = default;
    virtual void dispatch(const Envelope& envelope) = 0;
    virtual void subscribe(const std::string& typeId,
                           std::shared_ptr<IChannel> channel) = 0;
    virtual void unsubscribe(const std::string& typeId,
                             std::shared_ptr<IChannel> channel) = 0;
    virtual void flush() = 0;
};
```

Abstract dispatcher — the key extension point for sync ↔ async switching.
The `Dispatcher` facade holds a `std::unique_ptr<IDispatcher>`, so replacing
`SyncDispatcher` with a future `AsyncDispatcher` requires only a factory
change.

---

### DispatcherConfig

```cpp
struct DispatcherConfig {
    std::string name;
    bool        autoTimestamp = true;
};
```

| Field | Default | Description |
|---|---|---|
| `name` | `""` | Dispatcher identity — appears in debug output and log bridges. |
| `autoTimestamp` | `true` | If `true`, `Dispatcher::dispatch()` sets `envelope.timestamp` to `now()` when the caller leaves it at the epoch. |

---

### Dispatcher

#### Constructor

```cpp
Dispatcher(DispatcherConfig config, std::unique_ptr<IDispatcher> dispatcher);
```

Non-copyable and move-constructible.  The destructor calls `flush()`.

#### Identity

```cpp
const std::string& name() const;
```

#### Core dispatch

```cpp
void dispatch(const Envelope& envelope);
```

Auto-stamps `envelope.timestamp` if `autoTimestamp` is enabled and the field
is at the epoch.  Delegates to `IDispatcher::dispatch()`.

#### Subscription management

```cpp
void subscribe(const std::string& typeId, std::shared_ptr<IChannel> channel);
void unsubscribe(const std::string& typeId, std::shared_ptr<IChannel> channel);
```

Delegate to `IDispatcher::subscribe()` / `unsubscribe()`.

#### Flush

```cpp
void flush();
```

Forwards to `IDispatcher::flush()`, which in turn flushes all registered
channels.

---

### DispatcherFactory

```cpp
static Dispatcher createSyncDispatcher(const std::string& name,
                                       bool autoTimestamp = true);

static Dispatcher createDebugDispatcher(const std::string& name);
```

| Method | Assembles |
|---|---|
| `createSyncDispatcher` | `SyncDispatcher → SyncRouter` (empty, no channels) |
| `createDebugDispatcher` | `SyncDispatcher → SyncRouter + StdoutChannel("*")` |

---

### EventBusChannel

```cpp
using Handler = std::function<void(const Envelope&)>;

EventBusChannel();
void addHandler(Handler handler);
void send(const Envelope& envelope) override;   // invokes all handlers
void flush() override;                          // no-op
```

In-process channel.  Handlers are invoked synchronously in the dispatcher's
thread.  No serialization occurs — the raw `Envelope` is passed directly.

**Use-case:** connecting engine events to UI callbacks, AI systems, or unit
tests without any I/O overhead.

---

### StdoutChannel

```cpp
explicit StdoutChannel(std::unique_ptr<ISerializer> serializer = nullptr);

void send(const Envelope& envelope) override;   // serialize → std::cout
void flush() override;                          // std::cout.flush()
```

When `serializer` is `nullptr` at construction, a default `JsonSerializer` is
used (Phase 2).

---

### JsonSerializer

```cpp
std::string serialize(const Envelope& envelope) override;
static std::string escapeJsonString(const std::string& value);
```

#### `serialize()`

Produces a single-line JSON object.  The `payload` field contains a
best-effort string representation of `std::any` (Phase 2 will define the
exact strategy).

#### `escapeJsonString()`

Same escaping rules as `GmLog::JsonFormatter::escapeJsonString`:
`\`, `"`, newline, carriage return, tab, and control characters.

---

### SyncRouter

```cpp
SyncRouter();
void subscribe(const std::string& typeId,
               std::shared_ptr<IChannel> channel) override;
void unsubscribe(const std::string& typeId,
                 std::shared_ptr<IChannel> channel) override;
void route(const Envelope& envelope) override;
```

V1 routing rules:
1. Channels subscribed to `envelope.typeId` (exact match) receive the call.
2. Channels subscribed to `"*"` always receive the call.
3. If `Envelope::targets` is non-empty, only channels whose name matches a
   target are dispatched to (Phase 4 feature).

> **No internal mutex** — locking is the caller's responsibility
> (`SyncDispatcher` acquires the lock before calling any `IRouter` method).

---

### SyncDispatcher

```cpp
explicit SyncDispatcher(std::unique_ptr<IRouter> router);

void dispatch   (const Envelope& envelope) override;
void subscribe  (const std::string& typeId,
                 std::shared_ptr<IChannel> channel) override;
void unsubscribe(const std::string& typeId,
                 std::shared_ptr<IChannel> channel) override;
void flush      () override;
```

All four methods acquire the internal `std::mutex` before delegating to the
router or channels.

---

## JSON Output Format

```json
{"time":"2026-06-11T10:30:00.123","source":"CoreEngine","typeId":"engine.tick","messageId":"","targets":[],"payload":"TickData"}
```

| Field | Type | Always present |
|---|---|---|
| `time` | string (ISO-8601 UTC, ms precision) | Yes |
| `source` | string | Yes |
| `typeId` | string | Yes |
| `messageId` | string | Yes (empty if not set) |
| `targets` | array of strings | Yes (empty array if broadcast) |
| `payload` | string | Yes (type name or serialized value) |

---

## Usage Examples

### Quick start — in-process event bus

```cpp
#include "DispatcherFactory.hpp"
#include "channels/EventBusChannel.hpp"

GmDispatch::Dispatcher bus =
    GmDispatch::DispatcherFactory::createSyncDispatcher("GameBus");

std::shared_ptr<GmDispatch::EventBusChannel> ch =
    std::make_shared<GmDispatch::EventBusChannel>();

ch->addHandler([](const GmDispatch::Envelope& env) {
    // handle engine.tick
});

bus.subscribe("engine.tick", ch);

GmDispatch::Envelope env;
env.typeId  = "engine.tick";
env.source  = "CoreEngine";
// env.payload = TickData{frameId, dt};  // any serializable type
bus.dispatch(env);
```

---

### Debug dispatcher (stdout)

```cpp
GmDispatch::Dispatcher bus =
    GmDispatch::DispatcherFactory::createDebugDispatcher("GameBus");

// All dispatched envelopes will be printed as JSON to stdout
GmDispatch::Envelope env;
env.typeId = "input.key_pressed";
env.source = "InputSystem";
bus.dispatch(env);
```

---

### Direct construction

```cpp
#include "Dispatcher.hpp"
#include "DispatcherConfig.hpp"
#include "dispatchers/SyncDispatcher.hpp"
#include "routers/SyncRouter.hpp"
#include "channels/EventBusChannel.hpp"

GmDispatch::DispatcherConfig cfg;
cfg.name          = "GameBus";
cfg.autoTimestamp = true;

GmDispatch::Dispatcher bus(
    cfg,
    std::make_unique<GmDispatch::SyncDispatcher>(
        std::make_unique<GmDispatch::SyncRouter>()
    )
);
```

---

### Broadcast subscription

```cpp
// Subscribe to ALL message types
bus.subscribe("*", myDiagnosticChannel);
```

---

### Runtime subscribe / unsubscribe

```cpp
std::shared_ptr<GmDispatch::EventBusChannel> ch =
    std::make_shared<GmDispatch::EventBusChannel>();

bus.subscribe("input.key_pressed", ch);

// ... later ...
bus.unsubscribe("input.key_pressed", ch);
```

---

## Thread Safety

`SyncDispatcher` serialises **all** operations (`dispatch`, `subscribe`,
`unsubscribe`, `flush`) under a single `std::mutex`.

- `SyncRouter` has no internal mutex — the dispatcher's lock covers it.
- `EventBusChannel` callbacks are invoked within the dispatcher's lock.
  Callbacks must not call `subscribe`/`unsubscribe`/`dispatch` on the same
  `Dispatcher` (deadlock).  Use a separate bus or post to a queue.
- Share a `Dispatcher` across threads via `std::shared_ptr<Dispatcher>` or
  by passing references.  `Dispatcher` is non-copyable.

---

## Payload Conventions

The payload field is `std::any`.  Recommended usage:

```cpp
// Sender: store a typed value
struct TickData { int frame; float dt; };
env.payload = TickData{42, 0.016f};

// Receiver: extract the value
if (env.payload.type() == typeid(TickData)) {
    TickData td = std::any_cast<TickData>(env.payload);
}
```

For envelopes without data, leave `payload` default-constructed (`std::any{}`).

---

## Relationship with gmLog

`gmDispatch` is **fully independent** of `gmLog`.  Both libraries follow the
same three-layer architecture (envelope → dispatcher → channel/sink) but serve
different purposes:

| | gmLog | gmDispatch |
|---|---|---|
| Purpose | Application logging | Component communication |
| Message | Fixed `LogRecord` fields | Generic `Envelope` + `std::any` |
| Routing | None (fixed 1:1) | Dynamic 1:N subscription |
| Serialization | Always (JSON Lines) | Per-channel, optional |
| Extension | `ILogDispatcher` | `IDispatcher` + `IRouter` |

A future `LogDispatchBridge` (Phase 4) can forward `LogRecord` events to the
dispatch bus without modifying either library.

---

## Future Extensions

| Extension | Phase | Description |
|---|---|---|
| `AsyncDispatcher` | 4 | Queue + worker thread + `condition_variable` |
| `FileChannel` | 3 | Append serialized envelopes to a file |
| `IpSocketChannel` | 3 | TCP send; platform-agnostic interface |
| `WebSocketChannel` | 3+ | Via external library |
| Pattern-matching router | 4 | `"engine.*"` via wildcard / regex |
| Targeted delivery | 4 | Route only to channels in `Envelope::targets` |
| `LogDispatchBridge` | 4 | `LogRecord → Envelope` adapter |
