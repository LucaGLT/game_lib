# Dungeon Crawler Basic — Implementation Notes

**Version:** 0.1.0
**Status:** FASE A in progress 🔧

This document tracks implementation decisions, known issues, and integration
notes for the Dungeon Crawler Basic CoreEngine and GUI.

---

## Integration decisions (FASE A)

### gmMap usage

- `DungeonMap` uses `gmMap::gmMap<std::string>` internally.
- Room ids (strings) are mapped to `gmMap::LocationId` (uint32_t) via an
  internal `std::unordered_map<std::string, gmMap::LocationId>`.
- Adjacency is set via `gmMap` adjacency API after room creation.
- Tags are stored as gmMap location metadata (key = tag name, value = `true`).
- `DungeonMapLoader` uses `gmSave/json.hpp` (nlohmann::json) for file parsing.

### gmActor usage

- `ActorRoster` wraps `gmActor::ActorStore`.
- Hero maps to `ActorKind::HERO`.
- Monster and Monster Elite map to `ActorKind::ALLY_NPC` (repurposed as enemy;
  gmActor has no ENEMY kind — faction is tracked via tags instead).
- BossMonster maps to `ActorKind::HERO` with a special `boss_monster` tag to
  distinguish it from the player hero.
- HP is tracked via `Health` stat block. Tags via `StatusContainer` tags.
- Statuses (defended, poisoned, stunned) use `StatusContainer`.

### gmFlow usage

- `TurnFlow` uses `gmFlow::GameSession` + `gmFlow::ActorRegistry`.
- Actor order per round is set at session start from the `ActorRoster` data.
- One `Turn` per actor per round.

### gmRules usage

- `DungeonRuleAdapter` implements the `gmRules::RuleContext` interface.
- Conditions and effects from the GRS specs are expressed as
  `ConditionSpec` / `EffectSpec` objects fed to `gmRulesEngine::evaluate_condition`.
- Only Move, Heal, Equip rules are wired in v1.

### gmLog usage

- `GameLog` uses `gmLog::LoggerFactory` to create a logger with console + file sinks.
- Log level: INFO for normal events, WARNING for rejections, ERROR for failures.

### gmDispatch usage

- `GuiBridge` uses `gmDispatch::IpSocketChannel` (TCP client, lazy connect).
- `CmdServer` uses raw Winsock2/POSIX sockets, same framing as IpSocketChannel.
- Wire format: 4-byte big-endian length prefix + UTF-8 JSON.

---

## Known limitations in FASE A

- All CoreEngine method bodies are stubs (`// ToBeImplemented //`).
- GUI widgets are shells without real rendering.
- DungeonBridge connects to EngineReceiver/EngineSender but does not start them yet.
- The smoke test verifies only compilation and GUI startup, not gameplay.

---

## Specification references

- GRS rules: `gmRules/specs/dungeon-crawler-basic.example.grs`
- Machine-readable spec: `gmRules/specs/dungeon-crawler-basic.example.yaml`
- Rule flow diagrams: `gmRules/specs/dungeon-crawler-basic.example_Diagram.md`
- Wire contract: `GAME/Dungeon-Crawler-Basic/info/wire-contract-v1.md`

---

## FASE B checklist (to be filled during implementation)

- [ ] DungeonMap / DungeonMapLoader bodies with real gmMap calls.
- [ ] ActorRoster bodies with real gmActor calls.
- [ ] TurnFlow bodies with real gmFlow calls.
- [ ] DungeonRuleAdapter bodies with real gmRules RuleContext calls.
- [ ] ActionV1 bodies: Move, Heal, Equip effects applied via roster + map.
- [ ] GuiBridge body: IpSocketChannel lazy connect + envelope build.
- [ ] CmdServer body: Winsock2 accept/receive loop with framing.
- [ ] GUI widgets: real rendering and event-driven updates.
- [ ] Smoke test: CoreEngine compiles, GUI starts headless, handshake verified.
