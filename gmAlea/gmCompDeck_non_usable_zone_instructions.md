# GmCompDeck — Implementation Instructions for NON_USABLE Zone

**Target library:** `gmAlea / GmCompDeck`  
**Language:** C++17  
**Namespace:** `gmAlea`  
**Purpose:** Extend the existing composite deck lifecycle model with an additional zone for cards/tokens that exist in the collection but are not currently usable in the active game/session/scenario.

---

## 1. Goal

Implement a new zone in `GmCompDeck` named:

```cpp
NON_USABLE
```

or, if the existing naming style prefers PascalCase aliases:

```cpp
NonUsableZone
```

This zone represents cards/tokens that belong to the player/entity/global collection but are **not available for use in the current active deck/session**.

Example use case:

```text
A character owns 35 cards in total.
Before a mission, the player chooses 15 cards to bring.
The selected 15 go into MAIN_DECK.
The remaining 20 go into NON_USABLE.
```

The zone must remain generic. Do **not** hardcode mission, character, RPG, campaign, or any specific game concept into the library.

---

## 2. Existing Context

`GmCompDeck` currently models a complete card/token lifecycle across several zones. The current known zones are:

```text
MAIN_DECK
HAND
PLAY_AREA
DISCARD_PILE
BANISH_ZONE
```

A previous planned extension adds:

```text
MEMORY
```

After this task, the full intended zone set becomes:

```text
MAIN_DECK
HAND
PLAY_AREA
MEMORY
DISCARD_PILE
BANISH_ZONE
NON_USABLE
```

The core invariant must remain:

```text
Every token ID exists in exactly one zone at any time.
```

No card/token may appear in two zones simultaneously.

---

## 3. Conceptual Meaning of NON_USABLE

`NON_USABLE` is a holding zone for cards/tokens that are owned or known by the deck system but are not part of the currently playable lifecycle.

It is different from `BANISH_ZONE`.

### BANISH_ZONE

Use for:

```text
removed from game
permanently exiled
destroyed
consumed
out of lifecycle by game effect
```

### NON_USABLE

Use for:

```text
owned but not selected
available for future deck construction
temporarily locked
inactive reserve
sideboard-like cards
disabled by scenario rules
cards not brought into current mission/session
```

This distinction is important:

```text
BANISH = removed by game state/effect
NON_USABLE = unavailable by configuration/rule/loadout
```

---

## 4. Required Zone Semantics

`NON_USABLE` must be:

```text
ordered
non-shufflable
not drawable by normal draw operations
not playable directly by default
insertable
removable
queryable
valid for locate(id)
part of uniqueness checks
serializable
```

It must not participate in automatic reshuffle or normal draw mechanics.

It should be possible to move cards/tokens from `NON_USABLE` into active zones through explicit methods only.

---

## 5. Policy Definition

If the codebase uses policy-based zones, add a policy similar to this:

```cpp
struct NonUsablePolicy {
    static constexpr bool can_shuffle = false;
    static constexpr bool can_draw = false;
    static constexpr bool can_insert = true;
};
```

If the existing policies include more flags, add the equivalent values consistently:

```cpp
static constexpr bool preserves_order = true;
static constexpr bool can_remove = true;
static constexpr bool can_peek = true;
```

Only add new policy flags if the existing design already supports or requires them. Do not over-expand the policy system unless necessary.

---

## 6. Alias

Add a standalone zone alias:

```cpp
using NonUsableZone = PolicyBasedDeck<NonUsablePolicy>;
```

Keep naming consistent with existing aliases such as:

```cpp
MainDeck
CardHand
PlayArea
DiscardPile
BanishZone
MemoryZone
```

If the existing code uses a different naming convention, follow the existing convention.

---

## 7. ZoneType Update

If the library has a zone enum, extend it:

```cpp
enum class ZoneType {
    MAIN_DECK,
    HAND,
    PLAY_AREA,
    MEMORY,
    DISCARD_PILE,
    BANISH_ZONE,
    NON_USABLE
};
```

If `MEMORY` has not yet been implemented in the branch, still design this change to be compatible with it. The final implementation should support both `MEMORY` and `NON_USABLE`.

---

## 8. Required Public API Additions

Add explicit methods to `GmCompDeck`.

### Move cards into NON_USABLE

```cpp
void move_from_main_deck_to_non_usable(TokenId id);
void move_from_hand_to_non_usable(TokenId id);
void move_from_play_area_to_non_usable(TokenId id);
void move_from_discard_to_non_usable(TokenId id);
void move_from_memory_to_non_usable(TokenId id);
```

If `MEMORY` does not exist in the current codebase yet, add the method only after `MEMORY` is implemented, or guard the work behind the same phase.

### Move cards out of NON_USABLE

```cpp
void move_from_non_usable_to_main_deck(TokenId id);
void move_from_non_usable_to_hand(TokenId id);
void move_from_non_usable_to_discard(TokenId id);
void move_from_non_usable_to_memory(TokenId id);
void banish_from_non_usable(TokenId id);
```

The method `move_from_non_usable_to_play_area(TokenId id)` should be considered optional. In most games, cards should not go directly from unavailable to play area unless a game rule explicitly says so. If included, document that it is an advanced/explicit operation.

Recommended optional method:

```cpp
void move_from_non_usable_to_play_area(TokenId id);
```

### Queries

```cpp
const NonUsableZone& non_usable() const;
std::size_t non_usable_size() const;
bool is_in_non_usable(TokenId id) const;
std::vector<TokenId> non_usable_tokens() const;
```

### Bulk operations

Add bulk helpers for deck construction workflows:

```cpp
void move_many_to_non_usable(const std::vector<TokenId>& ids);
void move_many_from_non_usable_to_main_deck(const std::vector<TokenId>& ids);
```

Optional, but useful:

```cpp
void set_active_main_deck_from_non_usable(const std::vector<TokenId>& selected_ids);
```

However, avoid embedding deckbuilding-specific assumptions into `GmCompDeck`. If this method becomes too opinionated, keep it outside `GmCompDeck` in a game-specific layer.

---

## 9. Initial Construction Support

Support initializing `GmCompDeck` with some tokens in `NON_USABLE`.

Example conceptual constructor input:

```text
main_deck_tokens: [1, 2, 3, 4, 5]
non_usable_tokens: [6, 7, 8, 9, 10]
```

All tokens across all initial zones must still be globally unique.

If current constructors only accept a main deck list, do not break them. Add an overload or configuration struct.

Recommended structure:

```cpp
struct gmCompDeckConfig {
    std::vector<TokenId> main_deck_tokens;
    std::vector<TokenId> hand_tokens;
    std::vector<TokenId> play_area_tokens;
    std::vector<TokenId> memory_tokens;
    std::vector<TokenId> discard_tokens;
    std::vector<TokenId> banish_tokens;
    std::vector<TokenId> non_usable_tokens;
    bool shuffle_main_deck = true;
    std::optional<uint32_t> seed;
};
```

If this is too large for the current design, add only:

```cpp
GmCompDeck(std::vector<TokenId> main_deck_tokens,
           std::vector<TokenId> non_usable_tokens,
           std::optional<uint32_t> seed = std::nullopt);
```

but prefer the config struct if the library is already evolving toward more zones.

---

## 10. locate(id) Integration

Update `locate(id)` so that it can return `ZoneType::NON_USABLE`.

Example:

```cpp
auto zone = deck.locate(42);
// zone == ZoneType::NON_USABLE
```

If `locate(id)` currently returns a custom result type, extend that type without breaking current behavior.

---

## 11. Invariants

The following invariants are mandatory:

```text
1. A token exists in exactly one zone.
2. NON_USABLE participates in uniqueness validation.
3. Moving into NON_USABLE removes the token from its previous zone.
4. Moving out of NON_USABLE removes the token from NON_USABLE first.
5. Failed operations must leave the deck unchanged.
6. locate(id) must find tokens in NON_USABLE.
7. non_usable_size() must match the actual token count.
8. NON_USABLE cannot be shuffled.
9. NON_USABLE cannot be drawn from with normal draw operations.
10. NON_USABLE is serialized and restored correctly.
```

Atomicity is required. If a move fails halfway, the previous state must be restored or never modified.

---

## 12. Error Handling

Reuse the existing exception style of `GmDeck` / `GmCompDeck`.

Potential errors:

```text
EAleaTokenNotFoundError
EAleaDuplicateTokenIdError
InvalidZoneOperationError
DeckInvariantError
```

If an `InvalidZoneOperationError` or equivalent already exists, reuse it.

Do not introduce inconsistent exception naming.

Examples:

```text
Trying to draw from NON_USABLE → invalid operation.
Trying to move an unknown token from NON_USABLE → EAleaTokenNotFoundError.
Initializing a token in both MAIN_DECK and NON_USABLE → EAleaDuplicateTokenIdError.
```

---

## 13. Serialization

If `GmCompDeck` supports serialization or is used through `gmSave`, update `to_json` / `from_json` support.

The serialized form must include:

```json
{
  "main_deck": [1, 2, 3],
  "hand": [4],
  "play_area": [],
  "memory": [5],
  "discard": [6],
  "banish": [],
  "non_usable": [7, 8, 9]
}
```

If the current names use different field names, follow existing naming.

Versioning note:

```text
Saves created before NON_USABLE existed should load with an empty NON_USABLE zone.
```

If versioned save migration exists, add a migration rule. Otherwise, make `non_usable` optional during load and default it to empty.

---

## 14. Documentation Updates

Update:

```text
gmAlea/gmCompDeck_API.md
Game-Lib_readme.md if library overview lists zones
any README section that describes GmCompDeck zones
```

Document clearly:

```text
NON_USABLE is not BANISH.
NON_USABLE is for unavailable/reserve/not-selected tokens.
BANISH is for removed/exiled tokens.
```

Include a short example:

```cpp
// Player owns 20 cards but brings only 10.
GmCompDeck deck({/* selected */ 1,2,3,4,5,6,7,8,9,10},
                {/* not selected */ 11,12,13,14,15,16,17,18,19,20});

// Later, a rule unlocks card 12.
deck.move_from_non_usable_to_discard(12);
```

---

## 15. Required Tests

Add or update tests under:

```text
GmDeck/tests/
```

Suggested file:

```text
test_gmCompDeck_non_usable.cpp
```

### Test cases

1. **Initialization with NON_USABLE**

```text
Create deck with main deck tokens and non usable tokens.
Verify sizes.
Verify locate(id) for non usable tokens.
```

2. **Move HAND → NON_USABLE**

```text
Draw card to hand.
Move it to NON_USABLE.
Verify it is no longer in HAND.
Verify locate(id) returns NON_USABLE.
```

3. **Move NON_USABLE → MAIN_DECK**

```text
Move token from NON_USABLE to MAIN_DECK.
Verify it is drawable afterward.
```

4. **Move NON_USABLE → HAND**

```text
Move token from NON_USABLE to HAND.
Verify locate(id) returns HAND.
```

5. **Move NON_USABLE → MEMORY**

```text
Move token from NON_USABLE to MEMORY.
Verify locate(id) returns MEMORY.
Skip or condition this test if MEMORY is not yet implemented.
```

6. **Move NON_USABLE → BANISH**

```text
Move token from NON_USABLE to BANISH_ZONE.
Verify locate(id) returns BANISH_ZONE.
```

7. **No duplicate token across zones**

```text
Attempt to initialize token 5 in both MAIN_DECK and NON_USABLE.
Expect EAleaDuplicateTokenIdError.
```

8. **Cannot draw from NON_USABLE**

```text
Verify there is no public draw operation from NON_USABLE.
If a generic zone draw method exists, verify it rejects NON_USABLE.
```

9. **Cannot shuffle NON_USABLE**

```text
If a generic shuffle-zone method exists, verify it rejects NON_USABLE.
```

10. **Atomic failure**

```text
Attempt invalid move from NON_USABLE with an unknown token.
Verify all zones remain unchanged.
```

11. **Serialization roundtrip**

```text
Save deck state with NON_USABLE tokens.
Load it back.
Verify exact zone membership and order.
```

12. **Backward compatibility load**

```text
Load serialized deck without non_usable field.
Verify NON_USABLE exists and is empty.
```

---

## 16. Development Order

Follow the project convention used in the other `gmXxx` libraries:

```text
1. Inspect existing GmDeck / GmCompDeck style, naming, exceptions, tests.
2. Add declarations and stubs first.
3. Update API documentation to describe expected behavior.
4. Implement methods progressively.
5. Add tests.
6. Run existing tests to ensure no regression.
```

Do not rewrite `GmCompDeck` from scratch.

This task is an extension, not a redesign.

---

## 17. Compatibility Requirements

The implementation must:

```text
- Preserve all current public APIs.
- Preserve all current tests.
- Add NON_USABLE without changing existing zone semantics.
- Remain C++17 only.
- Avoid external dependencies.
- Follow existing namespace gmAlea.
- Follow existing naming/style conventions from GmDeck and GmCompDeck.
- Keep error behavior consistent with current library.
```

---

## 18. Non-Goals

Do not implement:

```text
- Deckbuilding rules.
- Mission loadout validation.
- Character progression.
- UI selection.
- Campaign ownership.
- Rule-specific unlock logic.
- Scripting.
```

`GmCompDeck` must only provide the storage/lifecycle mechanics.

Game-specific systems decide why a card is non-usable and when it may become usable.

---

## 19. Final Expected Result

After implementation, `GmCompDeck` should support this lifecycle:

```text
NON_USABLE → MAIN_DECK
NON_USABLE → HAND
NON_USABLE → MEMORY
NON_USABLE → DISCARD_PILE
NON_USABLE → BANISH_ZONE

MAIN_DECK → NON_USABLE
HAND → NON_USABLE
PLAY_AREA → NON_USABLE
MEMORY → NON_USABLE
DISCARD_PILE → NON_USABLE
```

The final zone model should be:

```text
MAIN_DECK      = active draw pile
HAND           = currently held cards
PLAY_AREA      = cards being resolved / in play
MEMORY         = cards retained in a special active reserve
DISCARD_PILE   = used cards
BANISH_ZONE    = removed cards
NON_USABLE     = owned/known but not currently usable cards
```

This provides a generic foundation for loadouts, reserves, locked cards, sideboards, inactive cards, and future deck construction systems.
