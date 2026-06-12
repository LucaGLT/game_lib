==============================
# Library: gmAlea (GmDeck + GmCompDeck)
==============================

# 1. Context and Goal

## Position in the project

`GmDeck` is the foundational token deck management library for `game_lib`.
It provides two layers:

1. **`GmDeck`** (base): raw token-list operations (shuffle, draw, push_back, etc.)
2. **`GmCompDeck`** (composite): multi-zone orchestrator — manages the full card
   lifecycle for a single game entity (player, faction, event source).

## Relationship with other libs

| Library     | Relationship                                          |
|-------------|-------------------------------------------------------|
| gmLog       | Independent. Tests may use gmLog for output.          |
| gmDispatch  | Independent. A future bridge may dispatch deck events.|
| gmSave      | Independent. GmCompDeck state can be serialised by gmSave. |

---

# 2. Code Style

- Follow the same style as `gmDispatch`: `#ifndef` guards, Doxygen `@brief/@param/@return`
- **Namespace:** `gmAlea`
- **No `auto` as return type** in public prototypes (user preference)
- **No external dependencies** — only C++17 stdlib
- **English only** for all comments, doc strings, and error messages
- Include guards pattern: `GMALEA_<FILENAME>_HPP` (uppercase, underscores)
- TODO comments in stubs: `// TODO: Phase N — description`

---

# 3. Include chain (dependency order)

```
GmDeck.hpp                  ← no deps within gmDeck/
CardLocation.hpp            ← <string> only
ZonePolicy.hpp              ← nothing (pure constexpr)
PolicyBasedDeck.hpp         ← "GmDeck.hpp", "ZonePolicy.hpp", <string>, <vector>, <stdexcept>
GmCompDeck.hpp              ← "PolicyBasedDeck.hpp", "CardLocation.hpp", <string>, <vector>, <optional>
GmCompDeck.cpp              ← "GmCompDeck.hpp"
```

External consumers include only `"gmAlea/GmCompDeck.hpp"` (pulls everything).

---

# 4. Template rules

`PolicyBasedDeck<Policy>` is a class template.  All method bodies **must**
reside in `PolicyBasedDeck.hpp` — no separate `.cpp`.

---

# 5. Policy enforcement strategy

| Violation type           | Mechanism        | When triggered        |
|--------------------------|------------------|-----------------------|
| shuffle on ordered zone  | `static_assert`  | compile time          |
| draw from insert-only    | `static_assert`  | compile time          |
| take_specific from banish| `static_assert`  | compile time          |
| token not in zone        | `EAleaTokenNotFoundError` exception | runtime  |
| duplicate token in zone  | `EAleaDuplicateTokenIdError` exception | runtime |
| policy violation (fallback) | `EAleaZonePolicyViolationError` exception | runtime |

---

# 6. Uniqueness invariant

`GmCompDeck` is the **sole** entry point for cross-zone moves.  Every public
method atomically calls `take_specific()` on the source zone and `add()` on
the destination zone.  The private helper `_remove_from_zone(zone, id)` is
called only when the target zone is known — never for `BANISHED` or
`NOT_FOUND`.

---

# 7. Exception hierarchy

```
std::runtime_error
└── EAleaError           (GmDeck.hpp — base for all deck errors)
    ├── EAleaDeckEmptyError         (draw from empty deck)
    ├── EAleaDuplicateTokenIdError  (push/add duplicate token)
    ├── EAleaInvalidDrawCountError  (draw_many with k ≤ 0)
    └── EAleaTokenNotFoundError     (draw_specific / take_specific: ID not found)

std::runtime_error
└── EAleaZonePolicyViolationError        (PolicyBasedDeck.hpp — policy runtime violation)
```

---

# 8. Zone operation matrix

| Method              | MainDeck | Hand | PlayArea | Discard | BanishZone |
|---------------------|----------|------|----------|---------|------------|
| `add()`             | ✅       | ✅   | ✅       | ✅      | ✅ (only)  |
| `add_to_top()`      | ✅       | ✅   | ✅       | ✅      | ❌*        |
| `draw()`            | ✅       | ✅   | ✅       | ✅      | ❌ compile |
| `take_specific()`   | ✅       | ✅   | ✅       | ✅      | ❌ compile |
| `shuffle()`         | ✅       | ❌ compile | ❌ compile | ❌ compile | ❌ compile |
| `peek_all()`        | ✅       | ✅   | ✅       | ✅      | ✅         |
| `peek_top(n)`       | ✅       | ✅   | ✅       | ✅      | ✅         |
| `contains()`        | ✅       | ✅   | ✅       | ✅      | ✅         |

*`add_to_top()` on BanishZone: technically compiles but semantically unused — add at back or top is equivalent for an insert-only zone.

---
