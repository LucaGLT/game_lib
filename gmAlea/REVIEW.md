# Code Review — gmDeck (→ gmAlea)

**Reviewed scope:** `gmDeck/` — full library (7 source files + 2 test files)  
**Date:** 2026-06-12  
**Rule set version:** `cpp-style.yml` v1.0 / `style-rules.md`  
**Reviewer:** AI (GitHub Copilot)

> **Context:** This review combines a **rename** (`gmDeck` → `gmAlea`,
> namespace `gmFate` → `gmAlea`, façades `GmDeck`/`GmCompDeck`) with a
> full **style compliance** audit.  Findings are independent of the rename
> where possible; rename-driven corrections are flagged `(rename)`.
>
> The review does **not** modify any source file.  All fixes are in the
> correction plan below.

---

## Summary

| Category | Status | 🔴 | 🟡 | 🔵 | Total |
|---|---|---|---|---|---|
| CAT-1 Naming | 🔴 Errors | 7 | 1 | 0 | 8 |
| CAT-2 Guards | 🔴 Errors | 5 | 0 | 0 | 5 |
| CAT-3 Formatting | 🔴 Errors | 2 | 0 | 0 | 2 |
| CAT-4 Spacing | ✅ Clean | 0 | 0 | 0 | 0 |
| CAT-5 Switch | ✅ Clean | 0 | 0 | 0 | 0 |
| CAT-6 Signatures | ✅ Clean | 0 | 0 | 0 | 0 |
| CAT-7 Includes | 🟡 Warnings | 0 | 1 | 0 | 1 |
| CAT-8 Preprocessor | ✅ Clean | 0 | 0 | 0 | 0 |
| CAT-9 Documentation | 🟡 Warnings | 0 | 2 | 0 | 2 |
| CAT-10 Constraints | ✅ Clean | 0 | 0 | 0 | 0 |
| **TOTAL** | | **14** | **4** | **0** | **18** |

---

## Findings

---

### CAT-1 · Naming

---

#### F-01 · NS-1 🔴 Error — namespace `gmFate` must become `gmAlea`

**Files:** `gmDeck.hpp`, `gmDeck.cpp`, `gmCompDeck.hpp`, `gmCompDeck.cpp`,
`PolicyBasedDeck.hpp`, `ZonePolicy.hpp`, `CardLocation.hpp`  
**Location:** every `namespace gmFate {` / `} // namespace gmFate`  
**Description:** The project namespace is `gm` + PascalCase.  The current
name `gmFate` is being replaced by `gmAlea` as part of the library rename.
All seven files declare or close `namespace gmFate`.

```diff
- namespace gmFate {
+ namespace gmAlea {

- } // namespace gmFate
+ } // namespace gmAlea
```

---

#### F-02 · CL-2 🔴 Error — façade `gmDeck` must be `GmDeck`

**File:** `gmDeck.hpp`, `gmDeck.cpp`  
**Location:** `class gmDeck`, all method definitions  
**Description:** The main façade class must carry the `Gm` prefix (rule CL-2).
The current name `gmDeck` (lowercase `gm`) does not match.

```diff
- class gmDeck {
+ class GmDeck {

- gmDeck::gmDeck(...)
+ GmDeck::GmDeck(...)
```

---

#### F-03 · CL-2 🔴 Error — façade `gmCompDeck` must be `GmCompDeck`

**File:** `gmCompDeck.hpp`, `gmCompDeck.cpp`  
**Location:** `class gmCompDeck`, all method definitions  
**Description:** Same rule as F-02.  `gmCompDeck` → `GmCompDeck`.

```diff
- class gmCompDeck {
+ class GmCompDeck {

- gmCompDeck::gmCompDeck(...)
+ GmCompDeck::GmCompDeck(...)
```

---

#### F-04 · EX-2 🔴 Error — library base exception missing `E` prefix and wrong name

**File:** `gmDeck.hpp`  
**Location:** `class DeckAdapterError`, line ~13  
**Description:** Each library must have exactly one base exception named
`E<Topic>Error`.  For `gmAlea` this must be `EAleaError`.  The current base
`DeckAdapterError` violates both the prefix rule (EX-1) and the base-naming
rule (EX-2).

```diff
- class DeckAdapterError : public std::runtime_error {
- public:
-     explicit DeckAdapterError(const std::string& message)
-         : std::runtime_error("DeckAdapterError: " + message) {}
- };
+ class EAleaError : public std::runtime_error
+ {
+ public:
+     explicit EAleaError(const std::string& msg)
+         : std::runtime_error(msg) {}
+ };
```

---

#### F-05 · EX-1 🔴 Error — `DeckEmptyError` missing `E` prefix

**File:** `gmDeck.hpp`  
**Location:** `class DeckEmptyError`  
**Description:** Exception classes require the `E` prefix (rule EX-1).

```diff
- class DeckEmptyError : public DeckAdapterError {
+ class EAleaDeckEmptyError : public EAleaError {
```

---

#### F-06 · EX-1 🔴 Error — `DuplicateTokenIdError`, `InvalidDrawCountError`, `TokenNotFoundError` missing `E` prefix

**File:** `gmDeck.hpp`  
**Location:** `class DuplicateTokenIdError`, `class InvalidDrawCountError`,
`class TokenNotFoundError`  
**Description:** Three exception classes missing the mandatory `E` prefix.

```diff
- class DuplicateTokenIdError : public DeckAdapterError {
+ class EAleaDuplicateTokenIdError : public EAleaError {

- class InvalidDrawCountError : public DeckAdapterError {
+ class EAleaInvalidDrawCountError : public EAleaError {

- class TokenNotFoundError : public DeckAdapterError {
+ class EAleaTokenNotFoundError : public EAleaError {
```

---

#### F-07 · EX-1 🔴 Error — `ZonePolicyViolation` missing `E` prefix and `Error` suffix

**File:** `PolicyBasedDeck.hpp`  
**Location:** `class ZonePolicyViolation`  
**Description:** Exception class missing both the `E` prefix and the required
`Error` suffix on the name.

```diff
- class ZonePolicyViolation : public std::runtime_error {
+ class EAleaZonePolicyViolationError : public EAleaError {
```

---

#### F-08 · VAR-2 🟡 Warning — `GmCompDeck` private members use trailing `_` (inconsistency)

**File:** `gmCompDeck.hpp`, `gmCompDeck.cpp`  
**Location:** `owner_name_`, `main_deck_`, `hand_`, `play_area_`, `memory_`,
`discard_`, `banish_zone_`  
**Description:** The style rules mandate a **leading** underscore for private
members (`_deck`, `_allow_duplicates`).  `GmDeck` already follows this.
`GmCompDeck` uses a trailing underscore instead, creating an inconsistency
within the same library.

```diff
-     std::string   owner_name_;
-     MainDeck      main_deck_;
-     CardHand      hand_;
+     std::string   _owner_name;
+     MainDeck      _main_deck;
+     CardHand      _hand;
```

Private method `_remove_from_zone` already uses the correct leading convention.

---

### CAT-2 · Include Guards

---

#### F-09 · IG-1 🔴 Error — all guards use `GMFATE_` prefix instead of `GMALEA_`

**Files:** `gmDeck.hpp`, `gmCompDeck.hpp`, `PolicyBasedDeck.hpp`,
`ZonePolicy.hpp`, `CardLocation.hpp`  
**Description:** Guard macro format is `<NAMESPACE_UPPER>_<FILENAME_UPPER>_HPP`.
After the rename the prefix must be `GMALEA_`.  Additionally the filenames
will change, so the full guard macros must be updated.

| Current | Corrected |
|---------|-----------|
| `GMFATE_GMDECK_HPP` | `GMALEA_GMDECK_HPP` |
| `GMFATE_GMCOMPDECK_HPP` | `GMALEA_GMCOMPDECK_HPP` |
| `GMFATE_POLICYBASEDDECK_HPP` | `GMALEA_POLICYBASEDDECK_HPP` |
| `GMFATE_ZONEPOLICY_HPP` | `GMALEA_ZONEPOLICY_HPP` |
| `GMFATE_CARDLOCATION_HPP` | `GMALEA_CARDLOCATION_HPP` |

```diff
- #ifndef GMFATE_GMDECK_HPP
- #define GMFATE_GMDECK_HPP
+ #ifndef GMALEA_GMDECK_HPP
+ #define GMALEA_GMDECK_HPP

- #endif // GMFATE_GMDECK_HPP
+ #endif // GMALEA_GMDECK_HPP
```

---

### CAT-3 · Formatting

---

#### F-10 · FMT-1 🔴 Error — all files use 4-space indentation instead of real tabs

**Files:** all 7 source files + 2 test files  
**Description:** The project mandates real tab characters (visual width 4).
All files currently use 4 ASCII spaces per indent level (confirmed by the
alignment of method bodies and class members).

```diff
- class GmDeck {
- public:
-     void shuffle();          // 4 spaces
+ class GmDeck {
+ public:
+ 	void shuffle();            // 1 tab
```

> This is a global find-replace on every indented line in every file.
> An editor or `unexpand` tool is the most reliable approach.

---

#### F-11 · FMT-2 🔴 Error — opening braces on same line (K&R style) throughout

**Files:** all 7 source files  
**Description:** The project mandates Allman style: opening `{` always on
its own line for classes, structs, functions, methods, `if`/`else`,
`switch`, `for`, `while`.  The entire library uses K&R style with `{` at
the end of the declaration line.

Affected constructs (representative sample):

| Construct | Found | Required |
|-----------|-------|----------|
| Class definition | `class GmDeck {` | `class GmDeck\n{` |
| Constructor body | `) {` | `)\n{` |
| Method body | `void shuffle() {` | `void shuffle()\n{` |
| `if` body | `if (x) {` | `if (x)\n{` |
| `switch` body | `switch (zone) {` | `switch (zone)\n{` |
| Inline function | `{ return ...; }` | keep on one line only if trivially simple |

```diff
- void GmDeck::shuffle() {
-     std::shuffle(_deck.begin(), _deck.end(), _rng);
- }
+ void GmDeck::shuffle()
+ {
+ 	std::shuffle(_deck.begin(), _deck.end(), _rng);
+ }

- class EAleaError : public std::runtime_error {
+ class EAleaError : public std::runtime_error
+ {
```

> **Exception (FMT-5):** truly trivial one-liners in struct/class bodies
> (e.g. `const std::string& owner_name() const { return _owner_name; }`)
> may keep `{}` on the same line if the full line fits within 100 columns.

---

### CAT-7 · Includes

---

#### F-12 · CAT-7 🟡 Warning — `gmDeck.cpp`: missing blank line between include groups

**File:** `gmDeck.cpp`  
**Location:** lines 1–3  
**Description:** The include order rule requires one blank line between
the own-header group and the standard-library group.

```diff
  #include "gmDeck.hpp"
+ 
  #include <algorithm>
  #include <sstream>
```

---

### CAT-9 · Documentation

---

#### F-13 · DOC 🟡 Warning — `@class` tags use old class names

**Files:** `gmDeck.hpp`, `gmCompDeck.hpp`  
**Location:** Doxygen `@class` comment blocks  
**Description:** After renaming, the `@class gmDeck` and `@class gmCompDeck`
Doxygen tags will refer to non-existent names.

```diff
- * @class gmDeck
+ * @class GmDeck

- * @class gmCompDeck
+ * @class GmCompDeck
```

---

#### F-14 · DOC 🟡 Warning — code examples in Doxygen use old namespace and class names

**File:** `gmCompDeck.hpp`  
**Location:** `@code` block inside `@class gmCompDeck` brief  
**Description:** The usage example references `gmFate::gmCompDeck` which
will no longer exist after the rename.

```diff
- *   gmFate::gmCompDeck player("Alice", {101, 102, 103, 104, 105, 106, 107});
+ *   gmAlea::GmCompDeck player("Alice", {101, 102, 103, 104, 105, 106, 107});
```

---

## Correction Plan

> Ordered by severity (🔴 first), then by file dependency.
> The `gmDeck/` folder itself must be **renamed to `gmAlea/`** as the first
> physical step.  File renames:
> `gmDeck.hpp/.cpp` → `GmDeck.hpp/.cpp`,
> `gmCompDeck.hpp/.cpp` → `GmCompDeck.hpp/.cpp`.

| Priority | File | Rules violated | Touch-points |
|----------|------|----------------|--------------|
| 🔴 1 | `gmDeck.hpp` → `GmDeck.hpp` | NS-1, CL-2, EX-1, EX-2, IG-1, FMT-1, FMT-2, DOC | ~60 |
| 🔴 2 | `gmDeck.cpp` → `GmDeck.cpp` | NS-1, CL-2, FMT-1, FMT-2, CAT-7 | ~35 |
| 🔴 3 | `PolicyBasedDeck.hpp` | NS-1, EX-1, IG-1, FMT-1, FMT-2 | ~30 |
| 🔴 4 | `gmCompDeck.hpp` → `GmCompDeck.hpp` | NS-1, CL-2, VAR-2, IG-1, FMT-1, FMT-2, DOC | ~50 |
| 🔴 5 | `gmCompDeck.cpp` → `GmCompDeck.cpp` | NS-1, CL-2, VAR-2, FMT-1, FMT-2 | ~40 |
| 🔴 6 | `ZonePolicy.hpp` | NS-1, IG-1, FMT-1, FMT-2 | ~20 |
| 🔴 7 | `CardLocation.hpp` | NS-1, IG-1, FMT-1, FMT-2 | ~15 |
| 🟡 8 | `tests/test_gmDeck_v2.cpp` | NS-1, CL-2, EX-1, FMT-1 | ~25 |
| 🟡 9 | `tests/test_gmCompDeck.cpp` | NS-1, CL-2, EX-1, FMT-1 | ~20 |

---

## Planned addition: `GmDice`

The user requested a new `GmDice` class as a third main façade of `gmAlea`.
This is outside the scope of the style review; it requires a separate planning
step before implementation.  Suggested location: `gmAlea/GmDice.hpp` +
`gmAlea/GmDice.cpp`.

---

## Statistics

| Metric | Value |
|--------|-------|
| Total findings | 18 |
| 🔴 Errors | 14 |
| 🟡 Warnings | 4 |
| 🔵 Info | 0 |
| Files reviewed | 9 |
| Files with violations | 9 |
| Violation rate | 100% |
