# gmCompDeck – Composite Deck Library

**Version:** 1.0
**Status:** Production
**Language:** C++17 Standard
**Namespace:** `gmFate`
**License:** Project License

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Zone Reference](#zone-reference)
- [API Reference](#api-reference)
  - [New gmDeck v2 methods](#new-gmdeck-v2-methods)
  - [ZoneId enum](#zoneid-enum)
  - [ZonePolicy structs](#zonepolicy-structs)
  - [PolicyBasedDeck\<Policy\>](#policybaseddeckpolicy)
  - [Type aliases](#type-aliases)
  - [gmCompDeck class](#gmcompdeck-class)
- [Usage Examples](#usage-examples)
- [Compile-time Safety](#compile-time-safety)
- [Exception Reference](#exception-reference)
- [Build Commands](#build-commands)
- [File Structure](#file-structure)

---

## Overview

**gmCompDeck** extends the base `gmDeck` library with a full multi-zone card
lifecycle management system.  It is designed around a common board/card game
model where each entity (player, faction, AI opponent) has its own private
set of interconnected decks.

### What gmCompDeck manages

```
┌────────────────────────── gmCompDeck ("Player1") ───────────────────────────┐
│                                                                               │
│  MainDeck      CardHand      PlayArea      DiscardPile    BanishZone         │
│  (shuffled)    (private)     (on table)    (ordered)      (removed forever)  │
│  {101,105,107} {102,104}     {103}         {106}          {}                 │
│                                                                               │
│  Invariant: every token ID appears in EXACTLY ONE zone                       │
└───────────────────────────────────────────────────────────────────────────────┘
```

### Difference from using gmDeck directly

| | Raw `gmDeck` | `gmCompDeck` |
|---|---|---|
| Zones | 1 | 5 (main, hand, play area, discard, banish) |
| Policy enforcement | None | Compile-time `static_assert` |
| Uniqueness invariant | Not managed | Guaranteed by design |
| Discard order | N/A | Preserved (LIFO, no shuffle) |
| Banish | N/A | Insert-only permanent removal |

---

## Architecture

The library is structured in three layers:

```
Layer 0  gmDeck                   raw token list (shuffle, draw, push_back, ...)
           │
Layer 1  PolicyBasedDeck<Policy>  per-zone wrapper — compile-time rules
           │
Layer 2  gmCompDeck               multi-zone orchestrator — invariant guarantor
```

---

## Zone Reference

| Zone        | Type alias   | Policy struct    | can_shuffle | can_direct_access | is_insert_only |
|-------------|--------------|------------------|-------------|-------------------|----------------|
| Main Deck   | `MainDeck`   | `MainDeckPolicy` | ✅           | ✅                 | ❌              |
| Hand        | `CardHand`   | `HandPolicy`     | ❌ *compile* | ✅                 | ❌              |
| Play Area   | `PlayArea`   | `PlayAreaPolicy` | ❌ *compile* | ✅                 | ❌              |
| Discard     | `DiscardPile`| `DiscardPolicy`  | ❌ *compile* | ✅                 | ❌              |
| Banish Zone | `BanishZone` | `BanishPolicy`   | ❌ *compile* | ❌ *compile*        | ✅              |

*"compile"* — the operation triggers a `static_assert` compile-time error.

---

## API Reference

### New gmDeck v2 methods

The following methods were added to `gmDeck` in v2 to support `PolicyBasedDeck`:

#### `push_back(uint32_t token_id)`

Appends a token at the back (bottom) of the deck without shuffling.

```cpp
deck.push_back(42);
```

**Throws:** `DuplicateTokenIdError` if `token_id` is already present.

---

#### `push_front(uint32_t token_id)`

Prepends a token at the front (top) of the deck without shuffling.

```cpp
deck.push_front(42);
```

**Throws:** `DuplicateTokenIdError` if `token_id` is already present.

---

#### `draw_specific(uint32_t token_id)`

Finds, removes, and returns a specific token regardless of its position.

```cpp
uint32_t card = deck.draw_specific(42);
```

**Throws:** `TokenNotFoundError` if `token_id` is not in the deck.

---

#### Constructor `auto_shuffle` flag

The constructor now accepts a third parameter `bool auto_shuffle = true`.
Existing code is fully backward-compatible.

```cpp
gmDeck deck(tokens, seed);             // auto_shuffle = true (default)
gmDeck deck(tokens, std::nullopt, false); // no initial shuffle
```

---

### ZoneId enum

Defined in `CardLocation.hpp`:

```cpp
enum class ZoneId {
    MAIN_DECK,  // primary shuffled draw deck
    HAND,       // cards held by the owner
    PLAY_AREA,  // cards currently in play on the table
    DISCARD,    // ordered discard pile
    BANISHED,   // permanently removed from game
    NOT_FOUND   // sentinel: token not tracked by this CompDeck
};
```

#### `zone_name(ZoneId zone)` — inline helper

Returns a human-readable uppercase string for a zone:

```cpp
gmFate::zone_name(gmFate::ZoneId::DISCARD);  // → "DISCARD"
```

---

### ZonePolicy structs

Defined in `ZonePolicy.hpp`.  Each struct provides three `constexpr bool` flags:

| Struct             | can_shuffle | can_direct_access | is_insert_only |
|--------------------|-------------|-------------------|----------------|
| `MainDeckPolicy`   | `true`      | `true`            | `false`        |
| `HandPolicy`       | `false`     | `true`            | `false`        |
| `PlayAreaPolicy`   | `false`     | `true`            | `false`        |
| `DiscardPolicy`    | `false`     | `true`            | `false`        |
| `BanishPolicy`     | `false`     | `false`           | `true`         |

Custom policies can be defined by providing a struct with the same three fields:

```cpp
struct EventQueuePolicy {
    static constexpr bool can_shuffle       = false;
    static constexpr bool can_direct_access = false;
    static constexpr bool is_insert_only    = false;
};
using EventQueue = gmFate::PolicyBasedDeck<EventQueuePolicy>;
```

---

### PolicyBasedDeck\<Policy\>

```cpp
template <typename Policy>
class PolicyBasedDeck {
public:
    explicit PolicyBasedDeck(std::string zone_name);

    PolicyBasedDeck(std::string zone_name,
                    const std::vector<uint32_t>& initial_tokens,
                    std::optional<unsigned int> seed = std::nullopt);

    void              add(uint32_t token_id);
    void              add_to_top(uint32_t token_id);
    uint32_t          draw();                          // static_assert if is_insert_only
    uint32_t          take_specific(uint32_t token_id); // static_assert if !can_direct_access
    void              shuffle();                       // static_assert if !can_shuffle

    std::vector<uint32_t> peek_all() const;
    std::vector<uint32_t> peek_top(int n) const;
    bool                  contains(uint32_t token_id) const;
    int                   count() const;
    bool                  is_empty() const;
    const std::string&    zone_name() const;
};
```

> **Note:** All bodies are defined inline in `PolicyBasedDeck.hpp` (C++ template requirement).

---

### Type aliases

```cpp
using MainDeck    = PolicyBasedDeck<MainDeckPolicy>;
using CardHand    = PolicyBasedDeck<HandPolicy>;
using PlayArea    = PolicyBasedDeck<PlayAreaPolicy>;
using DiscardPile = PolicyBasedDeck<DiscardPolicy>;
using BanishZone  = PolicyBasedDeck<BanishPolicy>;
```

These aliases can be used standalone, independently of `gmCompDeck`:

```cpp
gmFate::DiscardPile discard("Shared Graveyard");
discard.add(202);
discard.add(305);
// discard.shuffle();  ← compile error
```

---

### gmCompDeck class

```cpp
class gmCompDeck {
public:
    explicit gmCompDeck(std::string owner_name,
                        const std::vector<uint32_t>& deck_tokens = {},
                        std::optional<unsigned int>  seed        = std::nullopt);

    // Cross-zone moves
    void draw_to_hand(int count);
    void draw_specific_to_hand(uint32_t token_id);
    void play_card(uint32_t token_id);
    void resolve_card(uint32_t token_id);
    void discard_from_hand(uint32_t token_id);
    void discard_from_table(uint32_t token_id);
    void take_from_discard(uint32_t token_id);
    void return_from_discard_to_deck(uint32_t token_id);
    void banish(uint32_t token_id);
    void reshuffle_discard_into_deck();

    // Query
    ZoneId             locate(uint32_t token_id) const;
    int                count_in(ZoneId zone)     const;
    int                total_count()             const;
    const std::string& owner_name()              const;

    // Read-only zone accessors
    const MainDeck&    main_deck()   const;
    const CardHand&    hand()        const;
    const PlayArea&    play_area()   const;
    const DiscardPile& discard()     const;
    const BanishZone&  banish_zone() const;
};
```

#### Constructor

```cpp
explicit gmCompDeck(std::string owner_name,
                    const std::vector<uint32_t>& deck_tokens = {},
                    std::optional<unsigned int>  seed        = std::nullopt);
```

- `deck_tokens` populates the **main deck** and is shuffled automatically.
- All other zones start empty.
- `seed` controls the deterministic shuffle.

**Throws:** `DuplicateTokenIdError` if `deck_tokens` has duplicate IDs.

---

#### Cross-zone moves — full reference

| Method | Source → Destination | Throws |
|--------|----------------------|--------|
| `draw_to_hand(count)` | Main Deck → Hand (top × count) | `DeckEmptyError`, `InvalidDrawCountError` |
| `draw_specific_to_hand(id)` | Main Deck (any position) → Hand | `TokenNotFoundError` |
| `play_card(id)` | Hand → Play Area | `TokenNotFoundError` |
| `resolve_card(id)` | Play Area → Discard | `TokenNotFoundError` |
| `discard_from_hand(id)` | Hand → Discard | `TokenNotFoundError` |
| `discard_from_table(id)` | Play Area → Discard | `TokenNotFoundError` |
| `take_from_discard(id)` | Discard → Hand | `TokenNotFoundError` |
| `return_from_discard_to_deck(id)` | Discard → Main Deck (bottom) | `TokenNotFoundError` |
| `banish(id)` | Any movable zone → Banish | `TokenNotFoundError` |
| `reshuffle_discard_into_deck()` | Discard → Main Deck + shuffle | — |

---

#### `locate(uint32_t token_id) → ZoneId`

Returns the zone currently holding the token, or `ZoneId::NOT_FOUND`.

```cpp
gmFate::ZoneId loc = player.locate(102);
std::cout << gmFate::zone_name(loc);  // e.g. "HAND"
```

---

#### `count_in(ZoneId zone) → int`

```cpp
int in_hand = player.count_in(gmFate::ZoneId::HAND);
```

---

#### `total_count() → int`

Sum of all zones.  Should remain constant over a play session (except when
`banish()` adds tokens that were previously not tracked).

---

## Usage Examples

### Full game flow

```cpp
#include "gmDeck/gmCompDeck.hpp"
#include <iostream>

int main() {
    std::vector<uint32_t> cards = {101, 102, 103, 104, 105, 106, 107, 108};
    gmFate::gmCompDeck player("Alice", cards, /*seed=*/42);

    std::cout << "Deck size: " << player.count_in(gmFate::ZoneId::MAIN_DECK) << "\n"; // 8

    // Alice draws 3 cards
    player.draw_to_hand(3);
    std::cout << "Hand size: " << player.count_in(gmFate::ZoneId::HAND) << "\n";      // 3

    // Alice plays the first card in her hand
    uint32_t played = player.hand().peek_all().front();
    player.play_card(played);

    // The played card is resolved (e.g. effect triggered), goes to discard
    player.resolve_card(played);

    // Alice discards another card directly from hand
    uint32_t discarded = player.hand().peek_all().front();
    player.discard_from_hand(discarded);

    // Discard order is preserved — last discarded is first in list
    std::cout << "Discard top: " << player.discard().peek_all().front() << "\n";

    // Alice retrieves a specific card from discard back to hand
    player.take_from_discard(played);

    // Banish a card permanently
    player.banish(played);
    std::cout << "Banished: " << player.count_in(gmFate::ZoneId::BANISHED) << "\n";  // 1

    // Locate where a card is
    gmFate::ZoneId loc = player.locate(discarded);
    std::cout << gmFate::zone_name(loc) << "\n";  // "DISCARD"

    return 0;
}
```

---

### Reshuffle discard into deck

```cpp
gmFate::gmCompDeck player("Bob", {1, 2, 3, 4, 5});
player.draw_to_hand(5);  // draw all
for (uint32_t id : player.hand().peek_all()) {
    player.discard_from_hand(id);  // discard all — original order: 5,4,3,2,1 (LIFO)
}

player.reshuffle_discard_into_deck();  // discard → main deck, reshuffled
std::cout << player.count_in(gmFate::ZoneId::MAIN_DECK) << "\n";  // 5
std::cout << player.count_in(gmFate::ZoneId::DISCARD) << "\n";    // 0
```

---

### Custom zone standalone (without gmCompDeck)

```cpp
// Use PolicyBasedDeck independently for a shared game deck
gmFate::MainDeck event_deck("Events", {201, 202, 203, 204}, /*seed=*/7);
uint32_t next_event = event_deck.draw();

gmFate::DiscardPile graveyard("Graveyard");
graveyard.add(next_event);
// graveyard.shuffle();  ← compile error: DiscardPolicy::can_shuffle == false
```

---

## Compile-time Safety

`PolicyBasedDeck<Policy>` uses `static_assert` in method bodies to catch
misuse at compile time:

```cpp
gmFate::DiscardPile discard("pile");
discard.shuffle();
// error: static_assert failed "shuffle() is not allowed on this zone type..."

gmFate::BanishZone banish("out");
banish.draw();
// error: static_assert failed "draw() is not allowed on insert-only zones..."

banish.take_specific(42);
// error: static_assert failed "take_specific() is not allowed on zones with
//        can_direct_access == false..."
```

---

## Exception Reference

| Exception                | Header              | When thrown                                     |
|--------------------------|---------------------|-------------------------------------------------|
| `DeckAdapterError`       | `gmDeck.hpp`        | Base class; catch subclasses                    |
| `DeckEmptyError`         | `gmDeck.hpp`        | `draw_to_hand()` when main deck is empty        |
| `DuplicateTokenIdError`  | `gmDeck.hpp`        | Constructor or `add()` with duplicate ID        |
| `InvalidDrawCountError`  | `gmDeck.hpp`        | `draw_to_hand(count)` with `count <= 0`         |
| `TokenNotFoundError`     | `gmDeck.hpp`        | `draw_specific`, `take_specific`, move methods  |
| `ZonePolicyViolation`    | `PolicyBasedDeck.hpp` | Runtime policy violation (future use)         |

---

## Build Commands

Compile from the `game_lib` root directory:

### gmDeck v2 tests
```bash
g++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/tests/test_gmDeck_v2.cpp \
    -o test_gmDeck_v2 && ./test_gmDeck_v2
```

### gmCompDeck tests
```bash
g++ -std=c++17 -I. gmDeck/gmDeck.cpp gmDeck/gmCompDeck.cpp \
    gmDeck/tests/test_gmCompDeck.cpp -o test_gmCompDeck && ./test_gmCompDeck
```

### Compiler requirements
- C++17 or later (`-std=c++17`)
- Headers: `<vector>`, `<optional>`, `<stdexcept>`, `<random>`, `<algorithm>`, `<string>`
- No external dependencies

---

## File Structure

```
gmDeck/
├── PLAN.md
├── ai-instructions.md
├── gmDeck.hpp / gmDeck.cpp        ← base deck v2
├── gmDeck_API.md
├── CardLocation.hpp               ← ZoneId enum + zone_name()
├── ZonePolicy.hpp                 ← 5 policy structs
├── PolicyBasedDeck.hpp            ← template zone wrapper (bodies inline)
├── gmCompDeck.hpp                 ← CompositeDeck declaration
├── gmCompDeck.cpp                 ← CompositeDeck implementation
├── gmCompDeck_API.md              ← this file
└── tests/
    ├── test_gmDeck_v2.cpp
    └── test_gmCompDeck.cpp
```

---

**Generated:** 2026-06-11
**Documentation Version:** 1.0
