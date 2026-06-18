# Dungeon Crawler Basic — CoreEngine and GUI API

**Version:** 0.1
**Status:** FASE A completed (interfaces and stubs)
**Language:** C++17 (CoreEngine) + Python 3 / PySide6 (GUI)
**Namespace:** `gmDungeonBasic`
**Source directory:** `GAME/Dungeon-Crawler-Basic/`

---

## Table of Contents

- [Dungeon Crawler Basic — CoreEngine and GUI API](#dungeon-crawler-basic--coreengine-and-gui-api)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Architecture](#architecture)
  - [Dependencies](#dependencies)
  - [CoreEngine API (C++)](#coreengine-api-c)
    - [engine/](#engine)
      - [DungeonTypes.hpp](#dungeontypeshpp)
      - [DungeonEngine](#dungeonengine)
    - [world/](#world)
      - [DungeonMap](#dungeonmap)
      - [DungeonMapLoader](#dungeonmaploader)
    - [actors/](#actors)
      - [ActorInfo](#actorinfo)
      - [ActorRoster](#actorroster)
    - [flow/](#flow)
      - [TurnFlow](#turnflow)
    - [rules/](#rules)
      - [DungeonRuleAdapter](#dungeonruleadapter)
    - [actions/](#actions)
      - [ActionV1](#actionv1)
    - [log/](#log)
      - [GameLog](#gamelog)
    - [bridge/](#bridge)
      - [GuiBridge](#guibridge)
      - [CmdServer](#cmdserver)
  - [GUI API (Python)](#gui-api-python)
    - [Application layer](#application-layer)
    - [Widget layer](#widget-layer)
  - [Usage Examples](#usage-examples)
    - [C++ Minimal Engine Startup](#c-minimal-engine-startup)
    - [Python GUI Startup](#python-gui-startup)
    - [Command Roundtrip Example](#command-roundtrip-example)
  - [Contract Notes](#contract-notes)
  - [FASE A Notes](#fase-a-notes)

---

## Overview

Dungeon Crawler Basic is implemented as two separate processes:

- **CoreEngine (C++)**: source of truth for game state and rules.
- **GUI (Python/PySide6)**: presentation and user input only.

The v1 gameplay scope is intentionally limited to:

- Move
- Heal
- Equip

Attack and Defend are intentionally outside v1 and planned for v2.

---

## Architecture

```text
GUI (PySide6)                                 CoreEngine (C++)
┌───────────────────────┐                    ┌────────────────────────┐
│ DungeonMainWindow     │   commands 9201    │ CmdServer              │
│ + widgets             │ ──────────────────▶ │ DungeonEngine          │
│ + DungeonBridge       │                     │ + ActionV1             │
│                       │                     │ + DungeonRuleAdapter   │
│ EventRouter           │ ◀────────────────── │ GuiBridge              │
└───────────────────────┘   events 9200      │ + map/actors/flow/log  │
                                              └────────────────────────┘
```

---

## Dependencies

CoreEngine integrates these game libraries:

- `gmDispatch`
- `gmLog`
- `gmAlea`
- `gmFlow`
- `gmRules`
- `gmMap` (header-only)
- `gmSave` (JSON)
- `gmActor` (core sources compiled into target)

GUI reuses:

- `pyLib/gmGui/engine_bridge`

---

## CoreEngine API (C++)

### engine/

#### DungeonTypes.hpp

Defines shared enums and wire IDs.

**Enums**

- `GamePhase`: `BOOTSTRAP`, `HERO_TURN`, `MONSTER_TURN`, `GAME_OVER`
- `DungeonActorKind`: `HERO`, `MONSTER`, `MONSTER_ELITE`, `BOSS_MONSTER`
- `GameOutcome`: `NONE`, `HERO_DEFEATED`, `DUNGEON_CLEARED`

**Ports**

- `ports::EVENTS = 9200`
- `ports::COMMANDS = 9201`

**Command IDs**

- `dungeon.new_game`
- `dungeon.move`
- `dungeon.heal`
- `dungeon.equip`
- `dungeon.end_turn`

**Event IDs**

- `dungeon.session.started`
- `dungeon.map.snapshot`
- `dungeon.actor.snapshot`
- `dungeon.actor.moved`
- `dungeon.actor.healed`
- `dungeon.actor.equipped`
- `dungeon.actor.status_changed`
- `dungeon.actor.hp_changed`
- `dungeon.turn.started`
- `dungeon.turn.ended`
- `dungeon.action.rejected`
- `dungeon.game.over`

#### DungeonEngine

File: `CoreEngine/engine/DungeonEngine.hpp`

Facade/Mediator that coordinates all subsystems.

**Public API**

- `DungeonEngine()`
- `void start_game(const std::string& map_file = "")`
- `void handle_command(const std::string& typeId, const nlohmann::json& data)`
- `void advance_turn()`

**Responsibilities**

- Load map and initialize roster.
- Route commands to handlers.
- Build and emit snapshots/events.
- Keep phase and outcome state.

**Errors**

- `start_game` may throw on map load/runtime failures (full behavior in FASE B).

### world/

#### DungeonMap

File: `CoreEngine/world/DungeonMap.hpp`

Dungeon graph abstraction over gmMap.

**Public API**

- `DungeonMap()`
- `void create_room(const std::string& room_id)`
- `void add_connection(const std::string& from_id, const std::string& to_id, bool bidirectional = true)`
- `void set_room_tag(const std::string& room_id, const std::string& tag)`
- `void remove_room_tag(const std::string& room_id, const std::string& tag)`
- `bool has_room(const std::string& room_id) const`
- `bool is_adjacent(const std::string& from_id, const std::string& to_id) const`
- `bool room_has_tag(const std::string& room_id, const std::string& tag) const`
- `std::vector<std::string> all_rooms() const`
- `std::vector<std::string> rooms_adjacent_to(const std::string& room_id) const`
- `std::vector<std::string> tags_of_room(const std::string& room_id) const`
- `void reset()`

#### DungeonMapLoader

File: `CoreEngine/world/DungeonMapLoader.hpp`

JSON loader for map + initial actors.

**Public API**

- `DungeonMapLoader()`
- `bool load_from_file(const std::string& file_path, DungeonMap& map, ActorRoster& actors)`
- `std::string last_error() const`

**Behavior**

- Returns `true/false` for success/failure.
- Failure reason available via `last_error()`.

### actors/

#### ActorInfo

File: `CoreEngine/actors/ActorRoster.hpp`

Snapshot structure:

- `id`
- `kind`
- `hp`
- `max_hp`
- `location`
- `tags`
- `statuses`

#### ActorRoster

File: `CoreEngine/actors/ActorRoster.hpp`

Domain actor registry over gmActor.

**Public API**

- `ActorRoster()`
- `void add_actor(const ActorInfo& info)`
- `void remove_actor(const std::string& actor_id)`
- `bool has_actor(const std::string& actor_id) const`
- `ActorInfo get_actor(const std::string& actor_id) const`
- `std::vector<std::string> all_actor_ids() const`
- `std::vector<std::string> heroes() const`
- `std::vector<std::string> enemies() const`
- `std::vector<std::string> actors_in_location(const std::string& location_id) const`
- `void set_hp(const std::string& actor_id, int hp)`
- `void add_tag(const std::string& actor_id, const std::string& tag)`
- `void remove_tag(const std::string& actor_id, const std::string& tag)`
- `bool has_tag(const std::string& actor_id, const std::string& tag) const`
- `void add_status(const std::string& actor_id, const std::string& status_id)`
- `void remove_status(const std::string& actor_id, const std::string& status_id)`
- `bool has_status(const std::string& actor_id, const std::string& status_id) const`
- `void move_to(const std::string& actor_id, const std::string& location_id)`
- `void reset()`

### flow/

#### TurnFlow

File: `CoreEngine/flow/TurnFlow.hpp`

Turn/round manager over gmFlow.

**Public API**

- `TurnFlow()`
- `void start_session()`
- `void end_session()`
- `bool is_session_active() const`
- `void start_turn(const std::string& actor_id)`
- `void end_turn()`
- `bool is_turn_active() const`
- `std::string current_actor_id() const`
- `int current_round() const`
- `void set_actor_order(const std::vector<std::string>& actor_order)`
- `std::string next_actor_id() const`
- `void reset()`

### rules/

#### DungeonRuleAdapter

File: `CoreEngine/rules/DungeonRuleAdapter.hpp`

Bridge between game state and gmRules checks.

**Public API**

- `DungeonRuleAdapter(DungeonMap& map, ActorRoster& actors)`
- `bool can_move(const std::string& hero_id, const std::string& destination) const`
- `bool can_heal(const std::string& hero_id, const std::string& target_id) const`
- `bool can_equip(const std::string& hero_id, const std::string& item_tag) const`
- `std::string rejection_reason() const`

### actions/

#### ActionV1

File: `CoreEngine/actions/ActionV1.hpp`

Executor for v1 actions only.

**Public API**

- `ActionV1(DungeonMap& map, ActorRoster& actors, DungeonRuleAdapter& rules)`
- `bool execute_move(const std::string& hero_id, const std::string& destination)`
- `bool execute_heal(const std::string& hero_id, const std::string& target_id)`
- `bool execute_equip(const std::string& hero_id, const std::string& item_tag)`
- `std::string last_rejection_reason() const`

### log/

#### GameLog

File: `CoreEngine/log/GameLog.hpp`

Structured game logger over gmLog.

**Public API**

- `GameLog()`
- `void log_session_start(const std::string& session_id, const std::string& map_file)`
- `void log_action(const std::string& actor_id, const std::string& action, const std::string& detail = "")`
- `void log_rejection(const std::string& actor_id, const std::string& command, const std::string& reason)`
- `void log_session_end(const std::string& outcome)`
- `void log_info(const std::string& message)`

### bridge/

#### GuiBridge

File: `CoreEngine/bridge/GuiBridge.hpp`

Outbound event channel.

**Public API**

- `GuiBridge(const std::string& host = "127.0.0.1", uint16_t port = ports::EVENTS)`
- `void send_event(const std::string& typeId, const nlohmann::json& data)`

#### CmdServer

File: `CoreEngine/bridge/CmdServer.hpp`

Inbound command server.

**Public API**

- `using CommandHandler = std::function<void(const std::string&, const nlohmann::json&)>`
- `CmdServer(uint16_t port, CommandHandler handler)`
- `~CmdServer()`
- `void start()`
- `void stop()`

---

## GUI API (Python)

### Application layer

- `GUI/main.py`: app entry point.
- `GUI/app/dungeon_main_window.py`: window composition and orchestration.
- `GUI/app/dungeon_bridge.py`: adapter over shared engine bridge.
- `GUI/app/event_router.py`: typeId-based dispatch to widgets.

**DungeonMainWindow key methods**

- `_build_layout()`
- `_build_toolbar()`
- `_build_bridge()`
- `_build_router()`
- `_on_envelope(msg)`

**DungeonBridge key methods**

- `send_command(type_id, data)`
- `set_on_event(handler)`
- properties: `receiver`, `sender`

**EventRouter key methods**

- `register(type_id, handler)`
- `dispatch(msg)`

### Widget layer

- `DungeonBoardWidget`: map rendering and move click source.
- `HeroPanelWidget`: actor roster display.
- `ActionPanelWidget`: v1 action controls (heal/equip).
- `LogWidget`: event log stream.
- `ErrorBarWidget`: action rejection and feedback display.

All widgets expose `on_envelope(msg)` as the primary event input.

---

## Usage Examples

### C++ Minimal Engine Startup

```cpp
#include "bridge/CmdServer.hpp"
#include "engine/DungeonEngine.hpp"
#include "engine/DungeonTypes.hpp"

int main()
{
	gmDungeonBasic::DungeonEngine engine;
	gmDungeonBasic::CmdServer server(
	    gmDungeonBasic::ports::COMMANDS,
	    [&engine](const std::string& typeId, const nlohmann::json& data)
	    {
			engine.handle_command(typeId, data);
	    }
	);

	server.start();
	// main loop...
	server.stop();
	return 0;
}
```

### Python GUI Startup

```python
from PySide6.QtWidgets import QApplication
from app.dungeon_main_window import DungeonMainWindow

app = QApplication([])
window = DungeonMainWindow()
window.show()
app.exec()
```

### Command Roundtrip Example

```python
bridge.send_command(
    "dungeon.move",
    {
        "hero_id": "hero",
        "destination": "room_2"
    }
)
```

Expected success event (example):

```json
{
  "typeId": "dungeon.actor.moved",
  "source": "DungeonCore",
  "headers": {
    "data": "{\"actor_id\":\"hero\",\"from\":\"room_1\",\"to\":\"room_2\"}"
  }
}
```

---

## Contract Notes

Wire-level payload details are defined in:

- `GAME/Dungeon-Crawler-Basic/info/wire-contract-v1.md`

This API file documents classes and usage; the wire-contract file remains the
authoritative protocol source.

---

## FASE A Notes

All method bodies are currently stubs by design:

- C++ stubs include marker `// ToBeImplemented //`.
- Python stubs include marker `# ToBeImplemented //`.

Implementation logic is scheduled for FASE B.
