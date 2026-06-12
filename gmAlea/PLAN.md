# gmDeck + gmCompDeck – Development Plan

**Version:** 2.0
**Status:** Phase 3 – In Progress
**Language:** C++17 Standard
**Namespace:** `gmFate`

---

## Goal

Extend the base `gmDeck` token deck library with a full card-lifecycle management
system.  The new `gmCompDeck` class orchestrates multiple specialized zone-decks
(main deck, hand, play area, discard pile, banish zone) within a single entity
(player, faction, event source), guaranteeing the invariant that **every token ID
exists in exactly one zone at any time**.

---

## Architecture

```
External Code
      │ gmCompDeck("Player1", {101...110})
      ▼
gmCompDeck   (orchestrator — guarantees uniqueness invariant)
      │ draw_to_hand / play_card / discard_from_hand / banish / …
      ▼
PolicyBasedDeck<Policy>   (template zone wrapper — enforces per-zone rules)
      │ add / draw / take_specific / shuffle / …
      ▼
gmDeck   (raw token list — shuffle, draw, push_back, draw_specific)
```

### Zone Table

| Zone        | Type alias   | Policy            | Shuffle | Direct access | Insert-only |
|-------------|--------------|-------------------|---------|---------------|-------------|
| Main Deck   | `MainDeck`   | `MainDeckPolicy`  | ✅      | ✅            | ❌          |
| Hand        | `CardHand`   | `HandPolicy`      | ❌      | ✅            | ❌          |
| Play Area   | `PlayArea`   | `PlayAreaPolicy`  | ❌      | ✅            | ❌          |
| Discard     | `DiscardPile`| `DiscardPolicy`   | ❌ (**ORDER SACRED**) | ✅ | ❌    |
| Banish Zone | `BanishZone` | `BanishPolicy`    | ❌      | ❌            | ✅          |

---

## File Structure

```
gmDeck/
├── PLAN.md                     ← this file
├── ai-instructions.md          ← coding conventions for this library
├── gmDeck.hpp                  ← base deck v2 (push_back, push_front, draw_specific, auto_shuffle)
├── gmDeck.cpp
├── gmDeck_API.md               ← base deck API reference
├── CardLocation.hpp            ← enum class ZoneId + inline zone_name()
├── ZonePolicy.hpp              ← compile-time policy structs (5 built-in)
├── PolicyBasedDeck.hpp         ← template<typename Policy> class — bodies inline (template rule)
├── gmCompDeck.hpp              ← CompositeDeck orchestrator — declaration
├── gmCompDeck.cpp              ← CompositeDeck — method bodies
├── gmCompDeck_API.md           ← full API reference
└── tests/
    ├── test_gmDeck_v2.cpp      ← unit tests for gmDeck v2 new methods
    └── test_gmCompDeck.cpp     ← integration tests for gmCompDeck
```

---

## Development Phases

### Phase 1 — gmDeck v2 base enhancements ✅
**Goal:** Add missing mutation operations needed by `PolicyBasedDeck`.

- [x] `push_back(uint32_t)` — append a token without shuffling
- [x] `push_front(uint32_t)` — prepend a token without shuffling
- [x] `draw_specific(uint32_t)` — find, remove, and return a token by ID
- [x] Constructor `auto_shuffle` flag (default `true` — backward compatible)
- [x] `TokenNotFoundError` exception class

### Phase 2 — Policy + Template zone layer ✅
**Goal:** Compile-time zone behaviour constraints.

- [x] `CardLocation.hpp` — `enum class ZoneId` + inline `zone_name(ZoneId)` helper
- [x] `ZonePolicy.hpp` — 5 policy structs with constexpr flags
- [x] `PolicyBasedDeck.hpp` — template wrapper with `static_assert` enforcement

### Phase 3 — gmCompDeck orchestrator ✅
**Goal:** Cross-zone atomic moves with invariant enforcement.

- [x] `gmCompDeck.hpp` — class declaration + full Doxygen
- [x] `gmCompDeck.cpp` — all method bodies

### Phase 4 — Tests ✅
**Goal:** Verify correctness at each layer.

- [x] `tests/test_gmDeck_v2.cpp`   — 8 test cases
- [x] `tests/test_gmCompDeck.cpp`  — 12 test cases

### Phase 5 — Documentation
**Goal:** Complete API reference in Markdown.

- [x] `gmCompDeck_API.md`

---

## Key Design Decisions

### 1. Compile-time policy enforcement (static_assert)
Disallowed operations (e.g. `shuffle()` on `DiscardPile`, `draw()` on `BanishZone`)
trigger a **compile-time error** via `static_assert`, not a runtime exception.
This eliminates an entire class of logic bugs.

### 2. Uniqueness invariant via single orchestrator
`gmCompDeck` is the **only** code path that calls `take_specific()` + `add()`.
No external code can break the invariant by accident.

### 3. DiscardPile order is semantically meaningful
The discard pile is ordered LIFO (last discarded = top).  This matches
the board-game expectation: players can see the last discarded card.
`DiscardPolicy::can_shuffle = false` prevents accidental reordering.

### 4. BanishZone is insert-only
Banished cards are permanently removed.  `BanishPolicy::is_insert_only = true`
prevents `draw()` and `take_specific()` from even compiling.

### 5. gmDeck auto_shuffle is backward-compatible
Existing code using `gmDeck(tokens, seed)` is unaffected; `auto_shuffle`
defaults to `true`.

---

## Compilation Commands (from game_lib root)

### Test gmDeck v2
```bash
g++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/tests/test_gmDeck_v2.cpp \
    -o test_gmDeck_v2 && ./test_gmDeck_v2
```

### Test gmCompDeck
```bash
g++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/gmCompDeck.cpp \
    gmDeck/tests/test_gmCompDeck.cpp -o test_gmCompDeck && ./test_gmCompDeck
```

---

**Created:** 2026-06-11
**Namespace:** `gmFate`
