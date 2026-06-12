# libDeck - Token Deck Library

**Version:** 1.0  
**Status:** Production  
**Language:** C++17 Standard  
**License:** Project License

---

## Table of Contents

- [Overview](#overview)
- [Design Philosophy](#design-philosophy)
- [Installation](#installation)
- [API Reference](#api-reference)
  - [Exceptions](#exceptions)
  - [GmDeck Class](#tokendeck-class)
- [Usage Examples](#usage-examples)
- [Performance Characteristics](#performance-characteristics)
- [Thread Safety](#thread-safety)
- [Implementation Details](#implementation-details)

---

## Overview

**libDeck** is a high-performance, deterministic in-memory token deck management library written in C++17. It provides efficient shuffling, drawing, and manipulation of token collections using `uint32_t` identifiers for maximum performance.

### Key Features

- **Optimized for Speed**: Uses `uint32_t` instead of strings (4 bytes vs 50-100+ bytes)
- **O(1) Operations**: Constant-time lookups and contains checks
- **Deterministic**: Optional seeding for reproducible shuffles
- **Exception Safe**: Custom exception hierarchy for error handling
- **Cache Friendly**: Excellent cache locality for shuffle operations
- **Standard C++**: Uses only C++17 standard library (no external dependencies)

### Why uint32_t?

| Aspect | String | uint32_t |
|--------|--------|----------|
| **Memory per ID** | 50-100+ bytes | 4 bytes |
| **Comparison** | O(n) | O(1) |
| **Shuffle Speed** | 10-100x slower | Baseline |
| **Cache Behavior** | Poor | Excellent |
| **Lookups** | Linear search | Direct comparison |

---

## Design Philosophy

The library follows a **clean separation of concerns**:

- **GmDeck** manages **only** the deck structure (ordering, state, operations)
- **Token details** (name, type, properties) are managed externally
- **Domain rules** stay in the application layer, not in the library

This design ensures:
- ✅ Reusability across different token systems
- ✅ No coupling to specific token metadata
- ✅ Lightweight and fast core library
- ✅ Easy integration into larger systems

---

## Installation

### Requirements

- C++17 compatible compiler
- Standard C++ library (`<vector>`, `<random>`, `<stdexcept>`)
- CMake 3.10+ (optional, for building)

### Basic Integration

1. Copy `libDeck.hpp` and `libDeck.cpp` to your project
2. Include the header:

```cpp
#include "libDeck.hpp"
```

3. Use in your code:

```cpp
using namespace gmAlea;
```

---

## API Reference

### Exceptions

All exceptions inherit from `std::runtime_error` and live in the `gmAlea` namespace.

#### `EAleaError`

Base exception class for all deck-related errors.

```cpp
class EAleaError : public std::runtime_error {
public:
    explicit EAleaError(const std::string& message);
};
```

**When thrown:** As a base class; catch subclasses for specific errors.

---

#### `EAleaDeckEmptyError`

Thrown when attempting to draw from an empty deck.

```cpp
class EAleaDeckEmptyError : public EAleaError {
public:
    explicit EAleaDeckEmptyError(const std::string& message);
};
```

**When thrown:**
- `draw_one()` called on empty deck
- `draw_many(k)` called with k > remaining tokens

**Example:**
```cpp
try {
    deck.draw_one();
} catch (const EAleaDeckEmptyError& e) {
    std::cerr << e.what() << std::endl;
}
```

---

#### `EAleaDuplicateTokenIdError`

Thrown when token IDs are not unique.

```cpp
class EAleaDuplicateTokenIdError : public EAleaError {
public:
    explicit EAleaDuplicateTokenIdError(const std::string& message);
};
```

**When thrown:**
- Constructor receives duplicate token IDs
- `reset()` called with duplicate token IDs

**Example:**
```cpp
try {
    std::vector<uint32_t> tokens = {1, 2, 2, 3};  // Duplicate 2!
    GmDeck deck(tokens);
} catch (const EAleaDuplicateTokenIdError& e) {
    std::cerr << "Duplicates found: " << e.what() << std::endl;
}
```

---

#### `EAleaInvalidDrawCountError`

Thrown when an invalid draw count is requested.

```cpp
class EAleaInvalidDrawCountError : public EAleaError {
public:
    explicit EAleaInvalidDrawCountError(const std::string& message);
};
```

**When thrown:**
- `draw_many(k)` called with k ≤ 0

**Example:**
```cpp
try {
    deck.draw_many(-1);  // Invalid!
} catch (const EAleaInvalidDrawCountError& e) {
    std::cerr << e.what() << std::endl;
}
```

---

### GmDeck Class

Main class for managing token decks.

```cpp
namespace gmAlea {
    class GmDeck {
    public:
        explicit GmDeck(const std::vector<uint32_t>& token_ids, 
                          std::optional<unsigned int> seed = std::nullopt);
        
        void shuffle();
        uint32_t draw_one();
        std::vector<uint32_t> draw_many(int k);
        int remaining_count() const;
        bool is_empty() const;
        void reset(const std::optional<std::vector<uint32_t>>& token_ids = std::nullopt);
        std::vector<uint32_t> peek_all() const;
        void remove(uint32_t token_id);
        bool contains(uint32_t token_id) const;
    };
}
```

---

#### Constructor

```cpp
explicit GmDeck(const std::vector<uint32_t>& token_ids, 
                  std::optional<unsigned int> seed = std::nullopt);
```

**Parameters:**
- `token_ids` - Vector of unique token IDs to populate the deck
- `seed` - Optional random seed for deterministic shuffling
  - If provided: shuffle is reproducible (same seed = same shuffle)
  - If not provided: uses `std::random_device` for true randomness

**Throws:** `EAleaDuplicateTokenIdError` if `token_ids` contains duplicates

**Behavior:** 
1. Validates all token IDs are unique
2. Stores initial token IDs
3. Initializes RNG with provided seed or random seed
4. Calls `shuffle()` automatically

**Complexity:** O(n) where n = number of tokens

**Example:**
```cpp
// Non-deterministic shuffle
std::vector<uint32_t> tokens = {101, 102, 103, 104, 105};
GmDeck deck(tokens);

// Deterministic shuffle (seed=42)
GmDeck deck2(tokens, 42);

// Reproduce exact shuffle
GmDeck deck3(tokens, 42);  // Same order as deck2
```

---

#### shuffle()

```cpp
void shuffle();
```

**Parameters:** None

**Returns:** void

**Throws:** Nothing

**Behavior:** Shuffles the deck using `std::shuffle` with the internal Mersenne Twister RNG

**Complexity:** O(n) where n = number of tokens

**Note:** Automatically called in constructor. Manual shuffle useful after `reset()` or when deck state changes.

**Example:**
```cpp
GmDeck deck(tokens, 42);
auto drawn = deck.draw_one();  // Deck is now different
deck.shuffle();                 // Re-shuffle remaining tokens
```

---

#### draw_one()

```cpp
uint32_t draw_one();
```

**Parameters:** None

**Returns:** Single token ID (uint32_t)

**Throws:** `EAleaDeckEmptyError` if deck is empty

**Behavior:** 
1. Checks if deck is empty
2. Removes and returns first token from deck
3. Advances deck position

**Complexity:** O(n) due to erase from front (see Implementation Details)

**Example:**
```cpp
GmDeck deck(tokens);
while (!deck.is_empty()) {
    uint32_t card = deck.draw_one();
    std::cout << "Drew: " << card << std::endl;
}
```

---

#### draw_many(int k)

```cpp
std::vector<uint32_t> draw_many(int k);
```

**Parameters:**
- `k` - Number of tokens to draw (must be > 0)

**Returns:** Vector of k drawn token IDs

**Throws:**
- `EAleaInvalidDrawCountError` if k ≤ 0
- `EAleaDeckEmptyError` if k > remaining tokens

**Behavior:**
1. Validates k > 0
2. Validates k ≤ remaining count
3. Calls `draw_one()` k times
4. Returns vector of drawn tokens

**Complexity:** O(k * n) due to repeated erase operations (see Implementation Details)

**Example:**
```cpp
GmDeck deck(tokens);
auto drawn = deck.draw_many(3);  // Draw 3 tokens
for (auto token : drawn) {
    std::cout << token << " ";
}
```

---

#### remaining_count()

```cpp
int remaining_count() const;
```

**Parameters:** None

**Returns:** Count of tokens still in deck (int)

**Throws:** Nothing

**Complexity:** O(1)

**Example:**
```cpp
GmDeck deck(tokens);
std::cout << "Remaining: " << deck.remaining_count() << std::endl;
deck.draw_one();
std::cout << "Remaining: " << deck.remaining_count() << std::endl;
```

---

#### is_empty()

```cpp
bool is_empty() const;
```

**Parameters:** None

**Returns:** `true` if deck is empty, `false` otherwise

**Throws:** Nothing

**Complexity:** O(1)

**Equivalent to:** `remaining_count() == 0`

**Example:**
```cpp
while (!deck.is_empty()) {
    auto token = deck.draw_one();
    // Process token
}
```

---

#### reset()

```cpp
void reset(const std::optional<std::vector<uint32_t>>& token_ids = std::nullopt);
```

**Parameters:**
- `token_ids` - Optional new list of token IDs
  - If provided: replaces initial tokens and resets deck
  - If null: restores initial tokens

**Returns:** void

**Throws:** `EAleaDuplicateTokenIdError` if new token_ids contains duplicates

**Behavior:**
1. If `token_ids` provided: validates and stores as new initial tokens
2. Resets `_deck` to copy of `_initial_token_ids`
3. Reinitializes RNG with stored seed
4. Calls `shuffle()` automatically

**Complexity:** O(n)

**Example:**
```cpp
GmDeck deck(tokens1);
deck.draw_one();
deck.draw_one();

// Reset to initial state
deck.reset();  // Back to original tokens, shuffled
std::cout << deck.remaining_count();  // Back to original size

// Reset with new tokens
std::vector<uint32_t> new_tokens = {201, 202, 203};
deck.reset(new_tokens);
```

---

#### peek_all()

```cpp
std::vector<uint32_t> peek_all() const;
```

**Parameters:** None

**Returns:** Copy of all remaining token IDs in current order

**Throws:** Nothing

**Complexity:** O(n) for copying

**Note:** Returns a copy, not a reference; modifications don't affect deck

**Example:**
```cpp
auto all = deck.peek_all();
for (auto token : all) {
    std::cout << token << " ";
}
```

---

#### remove(uint32_t token_id)

```cpp
void remove(uint32_t token_id);
```

**Parameters:**
- `token_id` - Token ID to remove from deck

**Returns:** void

**Throws:** Nothing (silently does nothing if token not found)

**Behavior:**
1. Searches for first occurrence of `token_id`
2. Erases it if found
3. Does nothing if not found

**Complexity:** O(n) for linear search and erase

**Example:**
```cpp
deck.remove(102);  // Remove specific token
```

---

#### contains(uint32_t token_id)

```cpp
bool contains(uint32_t token_id) const;
```

**Parameters:**
- `token_id` - Token ID to search for

**Returns:** `true` if token exists in deck, `false` otherwise

**Throws:** Nothing

**Complexity:** O(n) for linear search

**Example:**
```cpp
if (deck.contains(102)) {
    std::cout << "Token 102 is still in deck" << std::endl;
} else {
    std::cout << "Token 102 has been drawn" << std::endl;
}
```

---

## Usage Examples

### Basic Game Flow

```cpp
#include "libDeck.hpp"
#include <iostream>

using namespace gmAlea;

int main() {
    // Create deck with deterministic shuffle
    std::vector<uint32_t> card_ids = {
        1001, 1002, 1003, 1004, 1005,
        1006, 1007, 1008, 1009, 1010
    };
    GmDeck deck(card_ids, 42);  // seed=42 for reproducible tests
    
    // Draw initial hand
    auto hand = deck.draw_many(5);
    std::cout << "Hand: ";
    for (auto card : hand) {
        std::cout << card << " ";
    }
    std::cout << "\nRemaining: " << deck.remaining_count() << std::endl;
    
    // Draw one more card
    uint32_t drawn = deck.draw_one();
    std::cout << "Drew: " << drawn << std::endl;
    
    // Check what's left
    std::cout << "Deck contents: ";
    for (auto card : deck.peek_all()) {
        std::cout << card << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

### Error Handling

```cpp
#include "libDeck.hpp"
#include <iostream>

using namespace gmAlea;

int main() {
    try {
        // Attempt to create deck with duplicates
        std::vector<uint32_t> bad_tokens = {1, 2, 2, 3};
        GmDeck deck(bad_tokens);
    } catch (const EAleaDuplicateTokenIdError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    try {
        // Attempt to draw from empty deck
        std::vector<uint32_t> tokens = {1, 2, 3};
        GmDeck deck(tokens);
        deck.draw_many(3);  // Draw all
        deck.draw_one();    // ERROR: empty!
    } catch (const EAleaDeckEmptyError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    try {
        // Invalid draw count
        GmDeck deck(tokens);
        deck.draw_many(0);  // ERROR: invalid count
    } catch (const EAleaInvalidDrawCountError& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
```

---

### Reset and Replay

```cpp
#include "libDeck.hpp"
#include <iostream>

using namespace gmAlea;

int main() {
    std::vector<uint32_t> tokens = {10, 20, 30, 40, 50};
    GmDeck deck(tokens, 123);  // Fixed seed
    
    // First game
    std::cout << "Game 1:" << std::endl;
    std::cout << "Draw 1: " << deck.draw_one() << std::endl;
    std::cout << "Draw 2: " << deck.draw_one() << std::endl;
    
    // Reset to restart exact same game
    std::cout << "\nGame 2 (replay):" << std::endl;
    deck.reset();  // Back to start, same shuffle
    std::cout << "Draw 1: " << deck.draw_one() << std::endl;
    std::cout << "Draw 2: " << deck.draw_one() << std::endl;
    
    // Reset with new deck
    std::cout << "\nGame 3 (new deck):" << std::endl;
    std::vector<uint32_t> new_deck = {100, 200, 300};
    deck.reset(new_deck);
    std::cout << "Draw 1: " << deck.draw_one() << std::endl;
    
    return 0;
}
```

---

## Performance Characteristics

### Time Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| `Constructor` | O(n) | Includes shuffle |
| `shuffle()` | O(n) | Mersenne Twister shuffle |
| `draw_one()` | O(n) | Front erase is O(n) |
| `draw_many(k)` | O(k·n) | k calls to draw_one() |
| `remaining_count()` | O(1) | Direct size() |
| `is_empty()` | O(1) | Checks .empty() |
| `reset()` | O(n) | Includes shuffle |
| `peek_all()` | O(n) | Copy operation |
| `remove()` | O(n) | Linear search + erase |
| `contains()` | O(n) | Linear search |

### Space Complexity

| Component | Space | Notes |
|-----------|-------|-------|
| `_deck` | O(n) | Vector of uint32_t |
| `_initial_token_ids` | O(n) | For reset functionality |
| `_rng` | O(1) | Mersenne Twister state |
| **Total** | **O(n)** | Where n = number of tokens |

### Memory Usage Example

For 1000 tokens:
- **With uint32_t**: ~8KB (1000 × 4 bytes × 2 copies)
- **With std::string** (50 bytes avg): ~400KB (same calculation)
- **Savings**: ~98% reduction ✅

---

## Thread Safety

**libDeck is NOT thread-safe by default.**

The library does not use synchronization primitives (mutexes, atomics). If you need multi-threaded access:

### Option 1: External Synchronization (Recommended)

```cpp
#include <mutex>
#include "libDeck.hpp"

class ThreadSafeDeck {
private:
    gmAlea::GmDeck deck;
    mutable std::mutex mutex;
    
public:
    uint32_t draw_one() {
        std::lock_guard<std::mutex> lock(mutex);
        return deck.draw_one();
    }
    
    std::vector<uint32_t> peek_all() const {
        std::lock_guard<std::mutex> lock(mutex);
        return deck.peek_all();
    }
    
    // ... wrap other methods similarly
};
```

### Option 2: Thread-Per-Deck

Each thread creates its own `GmDeck` instance (no contention).

---

## Implementation Details

### Data Structure

```cpp
std::vector<uint32_t> _deck;              // Working deck
std::vector<uint32_t> _initial_token_ids; // For reset()
std::optional<unsigned int> _seed;        // Seed value
std::mt19937 _rng;                        // Mersenne Twister RNG
```

### Why O(n) for draw_one()?

Currently, `draw_one()` uses `std::vector::erase(_deck.begin())` which is O(n) because:
1. Element at front must be removed
2. All remaining elements shift left
3. Total: n-1 copy operations

### Future Optimization

For decks with many draws, consider:
- **Ring buffer** instead of vector (O(1) draw_one, O(1) draw_many)
- **Deque** instead of vector (O(1) pop_front for draw_one)

Trade-off: shuffle would become O(n log n) for deque.

### Random Number Generation

- **Algorithm:** Mersenne Twister (MT19937)
- **Seed source:** User-provided or `std::random_device`
- **Reproducibility:** Same seed = same shuffle sequence
- **Quality:** Excellent uniformity for game purposes

---

## Compilation

### With GCC/Clang

```bash
g++ -std=c++17 -O3 -c libDeck.cpp -o libDeck.o
ar rcs libDeck.a libDeck.o
```

### With CMake

```cmake
add_library(libDeck libDeck.cpp libDeck.hpp)
target_compile_features(libDeck PUBLIC cxx_std_17)
target_compile_options(libDeck PRIVATE -O3)
```

### With MSVC

```bash
cl /std:latest /O2 /c libDeck.cpp
lib libDeck.obj /OUT:libDeck.lib
```

---

## License and Attribution

**libDeck** is part of the **gmAlea** project.

For questions, issues, or contributions, refer to the main project repository.

---

## Changelog

### Version 1.0 (Initial Release)

- ✅ Core GmDeck implementation with uint32_t optimization
- ✅ Four exception types for error handling
- ✅ Deterministic shuffling with optional seed
- ✅ Complete API documentation
- ✅ Doxygen HTML/LaTeX output
- ✅ Markdown documentation (this file)

---

**Generated:** 2026-06-10  
**Documentation Version:** 1.0
