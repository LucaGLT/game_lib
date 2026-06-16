# GmDice / StdDice Test Plan

## Objective

Add dedicated tests for `GmDice` and `StdDice` covering:

- single roll (`roll_one`)
- multi-roll (`roll`)
- algorithms (`ALGO_SUM`, `ALGO_MIN`, `ALGO_MAX`, `ALGO_MEAN_ROUND`)
- deterministic behavior with seed and `reseed`

## Scope

In scope:

- `gmAlea/tests/test_gmDice.cpp`
- `gmAlea/tests/test_stdDice.cpp`

Out of scope:

- performance benchmarks
- probabilistic distribution tests with strict thresholds

## Test Strategy

Use deterministic checks and exact expectations where possible.
Avoid flaky statistical assertions.

- For determinism: compare sequences from two instances built with same config/seed.
- For aggregation correctness: verify result against `rolled_out` values.
- For validation: verify correct exception type.

## Phase 1 — `GmDice` tests

### Phase 1 File

- `gmAlea/tests/test_gmDice.cpp`

### Phase 1 Cases

1. `constructor_empty_faces_throws`
2. `roll_one_value_from_face_set`
3. `roll_invalid_count_throws`
4. `roll_sum_matches_rolled_out`
5. `roll_min_matches_rolled_out`
6. `roll_max_matches_rolled_out`
7. `roll_mean_round_matches_rolled_out`
8. `roll_with_null_output_pointer`
9. `faces_count_includes_duplicates`
10. `same_seed_same_sequence`
11. `reseed_same_state_same_future_sequence`
12. `weighted_faces_are_representable` (construction + no throw + legal outputs)

### Phase 1 Build/Run command

```bash
g++ -std=c++17 -I. gmAlea/GmDeck.cpp gmAlea/SimpleDeck.cpp gmAlea/GmDice.cpp \
    gmAlea/tests/test_gmDice.cpp -o test_gmDice && ./test_gmDice
```

## Phase 2 — `StdDice` tests

### Phase 2 File

- `gmAlea/tests/test_stdDice.cpp`

### Phase 2 Cases

1. `default_constructor_is_d6`
2. `single_param_constructor_builds_1_to_max`
3. `range_constructor_builds_min_to_max`
4. `single_param_invalid_max_throws`
5. `range_invalid_bounds_throws`
6. `roll_one_within_range`
7. `roll_sum_matches_rolled_out`
8. `roll_min_matches_rolled_out`
9. `roll_max_matches_rolled_out`
10. `roll_mean_round_matches_rolled_out`
11. `same_seed_same_sequence`
12. `reseed_same_state_same_future_sequence`

### Phase 2 Build/Run command

```bash
g++ -std=c++17 -I. gmAlea/GmDeck.cpp gmAlea/SimpleDeck.cpp gmAlea/GmDice.cpp \
    gmAlea/StdDice.cpp gmAlea/tests/test_stdDice.cpp -o test_stdDice && ./test_stdDice
```

## Phase 3 — Regression verification

Run existing tests to ensure no regressions in gmAlea core:

```bash
g++ -std=c++17 -I. gmAlea/GmDeck.cpp gmAlea/tests/test_gmDeck_v2.cpp \
    -o test_gmDeck_v2 && ./test_gmDeck_v2

g++ -std=c++17 -I. gmAlea/GmDeck.cpp gmAlea/GmCompDeck.cpp \
    gmAlea/tests/test_gmCompDeck.cpp -o test_gmCompDeck && ./test_gmCompDeck
```

## Acceptance Criteria

- New tests compile and run successfully.
- All listed cases are implemented.
- Existing gmAlea tests still pass.
- No new compile diagnostics in modified/created files.
