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

## Libraries at a Glance (Updated)

| Library | Namespace | Maturity | CMake root integration | API doc |
| ------- | --------- | -------- | ---------------------- | ------- |
| `gmAlea` | `gmAlea` | Production | Enabled | `gmAlea/gmAlea_API.md` |
| `gmDispatch` | `GmDispatch` | Active | Enabled | `gmDispatch/gmDispatch_API.md` |
| `gmFlow` | `gmFlow` | Core ready, ongoing | Enabled | `gmFlow/gmFlow_API.md` |
| `gmLog` | `GmLog` | Production | Enabled | `gmLog/gmLog_API.md` |
| `gmMap` | `gmMap` | In development | Disabled in root CMake | `gmMap/gmMap_API.md` |
| `gmRules` | `gmRules` | Active | Enabled | `gmRules/gmRules_API.md` |
| `gmSave` | `gmSave` | Production | Disabled in root CMake | `gmSave/gmSave_API.md` |
| `gmActor` | `gmActor` | Early phases | Not integrated in root CMake | `gmActor/gmActor_API.md` |

---

## Library Summaries

### gmAlea

Deterministic deck and dice toolkit.

- Core components: `GmDeck`, `GmCompDeck`, `GmDice`, `StdDice`.
- Supports seeded reproducibility, duplicate-token decks, and multi-zone card
  lifecycle orchestration.

### gmDispatch

Message/event dispatch layer for decoupled engine components.

- Components include dispatcher, routers, channels, serializers, and bridges.
- `tests/` and `examples/` are present in the module.

### gmFlow

Flow controller framework (turns/phases/rounds/actions/event lifecycle).

- Namespace: `gmFlow`.
- API reports core infrastructure complete with `TimelineFlowController`.

### gmLog

Structured logging library with configurable sinks/formatters/dispatchers.

- JSON Lines output.
- Runtime and compile-time level filtering.

### gmMap

Topology-agnostic map model (`gmMap<ItemT>`) with locations, tiles, adjacency,
items, and metadata.

- Module contains implementation, tests, PLAN and REVIEW.
- Currently not added at root CMake level.

### gmRules

Rules toolkit for targets, conditions, effects, and status lifecycle.

- Public facade: `gmRules::gmRulesEngine`.
- Includes `specs/` and dedicated tests.

### gmSave

Generic JSON save/load layer for C++ structs.

- Versioned save envelope and `peek_version()`.
- Uses vendored `nlohmann/json` (`json.hpp`).
- Module is present but currently commented in root CMake.

### gmActor

Actor-state library (stats, statuses, modifiers, inventory, adapters).

- API currently reports phase-based progress and pending implementations.
- Not currently integrated in root CMake.

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

## Root CMake Status (Current)

According to `CMakeLists.txt` at repository root:

- Enabled: `gmAlea`, `gmLog`, `gmDispatch`, `gmFlow`, `gmRules`
- Disabled/commented: `gmMap`, `gmSave`
- Not yet added: `gmActor`

---

## What Was Missing or Outdated (Now Updated)

The previous README had these gaps now corrected:

- Missing libraries in the overview table:
  - `gmFlow`
  - `gmRules`
  - `gmActor`
- Missing tooling section for `grs` CLI (`tools/grs`), now added.
- Incorrect global statement claiming all libs are dependency-free:
  - corrected with explicit `gmSave/json.hpp` note.
- Build section examples referenced old paths (`gmDeck/...`) not aligned with
  current module layout (`gmAlea/...`), replaced with current high-level
  root CMake integration status.
- Status/integration visibility was incomplete:
  - now explicitly split by maturity and root CMake integration state.

---

## Suggested Validation Commands

```powershell
# build configured targets from root
cmake --build build --config Debug

# run GRS tests
cd tools
python -m pytest grs/tests/ --tb=no -q
```
