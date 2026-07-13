# game_lib

Collection of C++17 libraries for building tabletop game applications.

The workspace includes engine libraries and one Python support tool (`grs`) for
the `gmRules` DSL.

## Scope and Dependencies

- C++ baseline: C++17.
- Core design goal: independent libraries with clear boundaries.
- External dependency note:
  - Most C++ libraries rely on the standard library only.
  - `gmSave` vendors `nlohmann/json` (`gmSave/json.hpp`) by design.

---

## Libraries at a Glance

| Library | API Doc Link | Details |
| ------- | ------------ | ------- |
| `gmAlea` | [gmAlea_API.md](gmAlea/gmAlea_API.md) | v3.0 + F1 · Deterministic deck, dice, card sequences |
| `gmDispatch` | [gmDispatch_API.md](gmDispatch/gmDispatch_API.md) | v1.0 · Message/event dispatch layer |
| `gmFlow` | [gmFlow_API.md](gmFlow/gmFlow_API.md) | v2.0 + F2 · Flow control + timeline milestones |
| `gmLog` | [gmLog_API.md](gmLog/gmLog_API.md) | v1.0 · Structured logging |
| `gmMap` | [gmMap_API.md](gmMap/gmMap_API.md) | v1.0 (stub) · Topology-agnostic map model |
| `gmRules` | [gmRules_API.md](gmRules/gmRules_API.md) | Rules toolkit (target/condition/effect/status) |
| `gmSave` | [gmSave_API.md](gmSave/gmSave_API.md) | v1.0 · Generic JSON persistence |
| `gmActor` | [gmActor_API.md](gmActor/gmActor_API.md) | v0.2.0 + F3, F4 · Actor state, formations, behavior |
| `gmGui` | [gmGui_API.md](pyLib/gmGui/gmGui_API.md) | v0.1.0 · PySide6 generic game widgets |

> Each API doc contains the authoritative namespace, version, status, and detailed API reference.

---

## Library Details

Each library has its own namespace, version, and detailed API reference.
For complete information on any library, refer to its API document linked above.

### Quick Reference

| Library | Key Components | Main Use Case |
| ------- | -------------- | ------------- |
| **gmAlea** | `GmDeck`, `GmCompDeck`, `CardType`, `SequenceEngine`, `GmDice`, `StdDice` | Token shuffling, sequences, card zones, dice rolls |
| **gmDispatch** | `Dispatcher`, `EventBusChannel`, `SyncDispatcher` | Event routing between engine components |
| **gmFlow** | `GameSession`, `IFlowController`, `TimelineMilestoneSystem` | Turn/phase/action lifecycle, temporal triggers |
| **gmLog** | `Logger`, `LoggerConfig`, `SyncDispatcher` | Structured JSON Lines logging |
| **gmMap** | `gmMap<ItemT>`, `LocationId`, `ZoneId`, `RegionId` | Board state without enforced coordinates |
| **gmRules** | `gmRulesEngine`, `TargetResolver`, `ConditionEvaluator` | Target selection, condition checks, effects, status |
| **gmSave** | `save()`, `load()`, `try_load()` | Generic JSON persistence with versioning |
| **gmActor** | `ActorStore`, `FormationValidator`, `BehaviorCardProcessor`, `StatBlock` | Actor state, formations, behavior, modifiers |
| **gmGui** | `TimelineWidget`, `FormationWidget`, `SequenceStateWidget`, `BehaviorCardWidget` | Generic game UI components (PySide6) |

---

## Tooling: GRS CLI Tool (`tools/grs`)

`grs` is a Python CLI for the GRS DSL used by `gmRules/specs`.

### Features

- `lint`: structural checks (L-001 ... L-008)
- `validate`: semantic checks (V-001 ... V-010)
- `check`: combined lint + validate
- `yaml`: AST to canonical YAML export
- `grapho`: Mermaid graph generation (all rules or single rule)

### Quick usage

```powershell
# from repo root
grs check "gmRules\specs\turn-card-dungeon.example.grs"

# export yaml
grs yaml "gmRules\specs\turn-card-dungeon.example.grs" -o "gmRules\specs\turn-card-dungeon.example.yaml"

# export all diagrams
grs grapho "gmRules\specs\turn-card-dungeon.example.grs" -o "gmRules\specs\turn-card-dungeon.example_Diagram.md"
```

### Installation

```powershell
# from game_lib/tools
pip install -e .

# or from game_lib root
pip install -e tools/
```

### Current test status

- Test package: `tools/grs/tests/`
- Latest full run in workspace: `168 passed`.

### Detailed user manual

- See `tools/GRS_MANUAL.md`.

---

## Root CMake Integration Status

According to `CMakeLists.txt` at repository root:

- **Enabled (built by default):**
  - `gmAlea`
  - `gmLog`
  - `gmDispatch`
  - `gmFlow`
  - `gmRules`

- **Disabled (commented, not built):**
  - `gmMap` — in development, excluded to avoid build issues
  - `gmSave` — production-ready, but excluded to limit root dependencies

- **Not yet integrated:**
  - `gmActor` — early-phase development, awaiting full API stabilization

---

## Updates in This README

**Design change:** This README is now primarily a **navigation hub** and **CMake integration status** document.
All authoritative information (namespace, version, API details, status, features) lives in each library's dedicated `*_API.md` file.

### Why This Structure

- **Single source of truth:** Library maintainers update only the API doc; README links remain valid.
- **No duplication:** Namespace, version, and feature lists are *authored once* in the API doc.
- **Easy discovery:** Readers know exactly where to find complete documentation for each library.

### What Changed

| Aspect | Before | After |
| ------ | ------- | ----- |
| Namespace/version in README | Duplicated from API docs | Link + version in quick reference |
| Library summaries | Full feature lists | Quick reference table with main use case |
| Single source of truth | Fragmented | Centralized in each `*_API.md` |
| CMake integration state | Not visible | New dedicated section with clear status |

### Validation

To ensure consistency:
- Read the API doc link for complete namespace, version, and status.
- Refer to the [Root CMake Status](#root-cmake-status-current) section to see which libraries are built.

---

## Suggested Validation Commands

```powershell
# build configured targets from root
cmake --build build --config Debug

# run GRS tests
cd tools
python -m pytest grs/tests/ --tb=no -q
```
