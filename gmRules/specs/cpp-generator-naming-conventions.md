# C++ Generator Naming Conventions (Factory + Registry)

## Obiettivo

Definire una convenzione stabile per generare codice C++ da file regole YAML/JSON,
con output prevedibile e diff pulite.

## Scope

Queste regole valgono per file generati da schema in `gmRules/specs/game-rules.schema.json`.

## Namespace

- Namespace base generato: `gmRulesGen`
- Namespace gioco: derivato da `meta.namespace`, con `.` convertito in `::`
- Esempio: `checkers.core` -> `gmRulesGen::checkers::core`

## Struttura file generati

- Header API pubblica:
  - `generated/<game_id>/rules_registry.hpp`
- Sorgente registro:
  - `generated/<game_id>/rules_registry.cpp`
- Sorgente factory:
  - `generated/<game_id>/rules_factory.cpp`
- Dati status:
  - `generated/<game_id>/status_registry.cpp`
- Trigger pack:
  - `generated/<game_id>/trigger_registry.cpp`

Regola generale filename:

- lowercase snake_case
- suffisso semantico obbligatorio: `_registry`, `_factory`, `_ids`, `_builders`

## Naming ID -> simboli C++

Trasformazione canonica da id (`a.b-c:d`) a token C++:

1. sostituisci `.`, `-`, `:` con `_`
2. comprimi underscore multipli
3. se inizia con cifra, prefissa `id_`
4. mantieni lowercase per costanti e PascalCase per funzioni builder

Esempio:

- id: `capture.jump-and-mark:v1`
- token base: `capture_jump_and_mark_v1`
- funzione builder: `BuildCaptureJumpAndMarkV1`
- costante id: `kRuleId_capture_jump_and_mark_v1`

## Costanti stringa ID

Per ogni regola/status/trigger generare sempre costante dedicata:

```cpp
inline constexpr const char* kRuleId_move_simple_diagonal = "move.simple.diagonal";
inline constexpr const char* kStatusId_captured = "captured";
inline constexpr const char* kTriggerId_trigger_after_move_promotion = "trigger.after_move.promotion";
```

## Factory naming

### Regole

- Builder singola regola:
  - `gmRules::GeneratedRule Build<RuleToken>();`
- Factory dispatch per id:
  - `gmRules::GeneratedRule BuildRuleById(const std::string& rule_id);`

### Status

- Builder singolo status:
  - `gmRules::StatusDefinition BuildStatus<StatusToken>();`
- Factory dispatch:
  - `gmRules::StatusDefinition BuildStatusById(const std::string& status_id);`

### Trigger

- Builder singolo trigger:
  - `gmRules::GeneratedTrigger BuildTrigger<TriggerToken>();`
- Factory dispatch:
  - `gmRules::GeneratedTrigger BuildTriggerById(const std::string& trigger_id);`

## Registry naming

### Tipi suggeriti

```cpp
using RuleBuilderFn = gmRules::GeneratedRule (*)();
using StatusBuilderFn = gmRules::StatusDefinition (*)();
using TriggerBuilderFn = gmRules::GeneratedTrigger (*)();
```

### Map statiche

```cpp
const std::unordered_map<std::string, RuleBuilderFn>& GetRuleRegistry();
const std::unordered_map<std::string, StatusBuilderFn>& GetStatusRegistry();
const std::unordered_map<std::string, TriggerBuilderFn>& GetTriggerRegistry();
```

### API lookup

```cpp
bool HasRuleId(const std::string& id);
bool HasStatusId(const std::string& id);
bool HasTriggerId(const std::string& id);
```

## Convenzioni tipi runtime consigliate

Per evitare dipendenze implicite, il generator dovrebbe emettere wrappers espliciti:

```cpp
struct GeneratedRule
{
    std::string id;
    int priority;
    bool enabled;
    gmRules::TargetSpec target;
    std::vector<gmRules::ConditionSpec> condition_roots;
    std::vector<gmRules::EffectSpec> effects;
};

struct GeneratedTrigger
{
    std::string id;
    int priority;
    bool enabled;
    gmRules::TriggerType type;
    std::vector<gmRules::ConditionSpec> condition_roots;
    std::vector<gmRules::EffectSpec> effects;
};
```

## Convenzioni errori generator

- Prefisso errori codegen: `ECodegen...`
- Messaggio sempre con coordinate:
  - `<file>:<json_path>: <message>`

Esempio:

- `checkers.rules.yaml:$.rules[2].effects[0].value_ref: unresolved ref 'input.dst'`

## Ordinamento deterministico (obbligatorio)

- Generazione in ordine lessicografico per `id`.
- Proprieta serializzate in ordine fisso.
- Nessun timestamp nei file generati.

Questo garantisce diff minime e build riproducibili.

## Header guard / macro

Per header generati:

- formato: `GMRULESGEN_<GAMEID>_<FILENAME>_HPP`
- esempio: `GMRULESGEN_CHECKERS_RULES_REGISTRY_HPP`

## Esempio API finale minima

```cpp
namespace gmRulesGen::checkers::core
{

const std::unordered_map<std::string, RuleBuilderFn>& GetRuleRegistry();
const std::unordered_map<std::string, StatusBuilderFn>& GetStatusRegistry();
const std::unordered_map<std::string, TriggerBuilderFn>& GetTriggerRegistry();

gmRules::GeneratedRule BuildRuleById(const std::string& rule_id);
gmRules::StatusDefinition BuildStatusById(const std::string& status_id);
gmRules::GeneratedTrigger BuildTriggerById(const std::string& trigger_id);

} // namespace gmRulesGen::checkers::core
```

## Suggerimento pratico per il generatore

Pipeline consigliata:

1. Parse YAML
2. Validazione JSON Schema Draft 2020-12
3. Validazione semantica cross-reference
4. Normalizzazione in IR interna
5. Emissione `.hpp/.cpp` con naming sopra
6. Test smoke su lookup e build di ogni id
