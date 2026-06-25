# gmRules – Development Plan

**Version:** 2.1
**Status:** Phase 6 – In Progress 🔧
**Language:** C++17 Standard
**Namespace:** `gmRules`

---

## Goal

`gmRules` è il motore di regole di `game_lib`: valuta condizioni, risolve
effetti e gestisce stati tramite un'interfaccia astratta (`RuleContext`) che
isola la libreria dallo stato di gioco concreto.  La versione 2.0 introduce
**`RuleBook`** — il livello mancante che collega le stringhe `RuleId` a reali
`RuleDefinition` eseguibili — e un **`RuleBookLoader`** che carica definizioni
da file JSON, trasformando le regole descritte in `cards_dominion.json` /
`rule_groups.json` in chiamate effettive a `EffectResolver`.  La scelta
progettuale centrale è separare la *descrizione* (JSON/YAML) dalla *registrazione*
(`RuleBook`) e dall'*esecuzione* (`gmRulesEngine`), mantenendo `gmRules`
indipendente da file system, parser JSON e da qualsiasi altra libreria `game_lib`.

---

## Architecture

```
Game-specific code (es. GAME/Dungeon-Crawler)
      │
      │  cards.json + rule_groups.json + rules_dominion.json
      │         ↓
      │  RuleBookLoader::load_json(path)
      │         ↓ registra RuleDefinition
      ▼
  RuleBook                               ← NUOVO — mappa RuleId → RuleDefinition
      │ .resolve_rule(rule_id, actor, targets, ctx)
      ▼
  gmRulesEngine (façade esistente)
      ├── TargetResolver   ← già implementato
      ├── ConditionEvaluator ← già implementato
      └── EffectResolver   ← già implementato
               │
               ▼  via RuleContext (interfaccia)
         Stato di gioco concreto (gmActor, gmAlea, gmMap…)

══════════════════════════════════════════════════════
Ciclo completo: carta giocata → ZoneChangeCallback
══════════════════════════════════════════════════════

GmCompDeck::play_card(101)
      │ _fire_zone_change(101, HAND, PLAY_AREA)
      ▼
CardRuleBridge
      │ RuleGroupRegistry::activate("rg_village")
      ▼
RuleGroupRegistry::active_rule_ids()
      │ → ["r_add_action_1", "r_add_actions_2"]
      ▼
RuleBook::resolve_rule("r_add_action_1", actor, targets, ctx)
      │ → RuleDefinition{ effects: [MODIFY_RESOURCE "actions" +1] }
      ▼
EffectResolver::resolve(EffectSpec{MODIFY_RESOURCE, SELF, "actions", +1}, ctx)
      │
      ▼
RuleContext::modify_resource("Player_X", "actions", +1)  ← implementato dal gioco
```

---

## File Structure

```
gmRules/
├── PLAN.md                                ← questo file
├── CMakeLists.txt
├── gmRules_API.md
├── core/
│   ├── Ids.hpp                            ← RuleId, ActorId, CardId… (esistente)
│   ├── RuleContext.hpp                    ← interfaccia astratta (esistente)
│   ├── RuleResult.hpp/.cpp                ← esito operazioni (esistente)
│   ├── RuleEvent.hpp                      ← eventi emessi (esistente)
│   ├── RuleTypes.hpp                      ← gerarchia eccezioni (esistente)
│   ├── RuleGroup.hpp                      ← RuleGroup + lifecycle (aggiunto v1.1)
│   ├── RuleGroupRegistry.hpp/.cpp         ← registry gruppi attivi (aggiunto v1.1)
│   ├── RuleDefinition.hpp                 ← NUOVO struct: id + effects[] + conditions[]
│   └── RuleBook.hpp/.cpp                  ← NUOVO RuleId → RuleDefinition + resolve
├── loader/
│   ├── RuleBookLoader.hpp                 ← NUOVO parser JSON → RuleBook
│   └── RuleBookLoader.cpp
├── effect/
│   ├── EffectType.hpp                     ← enum EffectType (esistente)
│   ├── EffectSpec.hpp                     ← struct EffectSpec (esistente)
│   ├── EffectResolver.hpp/.cpp            ← esecuzione effetti (esistente)
│   └── EffectResult.hpp/.cpp              ← esito effetto (esistente)
├── condition/
│   └── …                                  ← esistente
├── target/
│   └── …                                  ← esistente
├── status/
│   └── …                                  ← esistente
├── facade/
│   ├── gmRulesEngine.hpp/.cpp             ← facade (da estendere con RuleBook)
│   └── …
├── specs/
│   ├── game-rules.schema.json             ← schema YAML/JSON regole (esistente)
│   ├── dungeon-crawler-basic.example.yaml ← esempio esistente
│   └── dominion.example.json              ← NUOVO esempio carte Dominion
└── tests/
    ├── test_rule_book.cpp                 ← NUOVO test RuleBook + Loader
    └── …                                  ← test esistenti
```

---

## Development Phases

### Phase 1 — Interfaces & Stubs ✅

- [x] `core/Ids.hpp` — alias `RuleId`, `ActorId`, `CardId`, …
- [x] `core/RuleContext.hpp` — interfaccia astratta
- [x] `core/RuleResult.hpp/.cpp`
- [x] `core/RuleEvent.hpp`
- [x] `core/RuleTypes.hpp` — gerarchia eccezioni (`ERulesError`, …)
- [x] `effect/EffectType.hpp` — enum `EffectType`
- [x] `effect/EffectSpec.hpp` — struct `EffectSpec`
- [x] Smoke test: compilazione senza errori ← smoke test **PASS**

### Phase 2 — Core resolvers ✅

- [x] `target/TargetSpec.hpp`, `TargetRef.hpp`, `TargetResult.hpp`
- [x] `target/TargetResolver.hpp/.cpp`
- [x] `condition/ConditionSpec.hpp`, `ConditionEvaluator.hpp/.cpp`
- [x] `effect/EffectResolver.hpp/.cpp`, `EffectResult.hpp/.cpp`
- [x] `tests/test_target_resolver.cpp`
- [x] `tests/test_condition_evaluator.cpp`
- [x] `tests/test_effect_resolver.cpp`
- [x] Smoke test: tutti i test esistenti passano ← smoke test **PASS**

### Phase 3 — Status engine ✅

- [x] `status/StatusDefinition.hpp`
- [x] `status/StatusInstance.hpp`
- [x] `status/StatusEngine.hpp/.cpp`
- [x] `tests/test_status_engine.cpp`
- [x] Smoke test: `test_status_engine` PASS ← smoke test **PASS**

### Phase 4 — Facade + RuleGroup registry ✅

- [x] `facade/gmRulesEngine.hpp/.cpp` — orchestra tutti i resolver
- [x] `core/RuleGroup.hpp` — `RuleGroupLifecycle` + `RuleGroup`
- [x] `core/RuleGroupRegistry.hpp/.cpp` — `activate()` / `deactivate()` / `active_rule_ids()`
- [x] `tests/test_rules_integration.cpp`
- [x] Smoke test: `test_rules_integration` PASS ← smoke test **PASS**

**Notes:**
- `RuleGroupRegistry` gestisce QUALE set di regole è attivo; non esegue nulla.
- Il bridge `gmAlea/bridges/gmAlea_gmRules/CardRuleBridge.hpp` collega
  `GmCompDeck::ZoneChangeCallback` al registry.

### Phase 5 — RuleBook + Loader 🔧

**Obiettivo:** Collegare `RuleId → RuleDefinition → EffectSpec[]` in modo che
le stringhe provenienti da `RuleGroupRegistry::active_rule_ids()` possano
essere effettivamente eseguite tramite `EffectResolver`.

- [x] `core/RuleDefinition.hpp` — struct `RuleDefinition`
- [x] `core/RuleBook.hpp` — classe `RuleBook`: registro + `resolve_rule()`
- [x] `core/RuleBook.cpp` — implementazione
- [x] `loader/RuleBookLoader.hpp` — interfaccia loader
- [x] `loader/RuleBookLoader.cpp` — parser JSON → `RuleBook`
- [x] `specs/dominion.example.json` — definizioni regole Dominion di esempio
- [x] `facade/gmRulesEngine.hpp` — aggiungi `RuleBook` come membro + `resolve_rule()`
- [x] `facade/gmRulesEngine.cpp` — implementa nuovi metodi
- [x] `tests/test_rule_book.cpp` — unit test RuleBook e Loader
- [x] Smoke test: `load_json("dominion.example.json")` + `resolve_rule("r_add_action_1", ctx)` ritorna `RuleResult::success()` ← smoke test **PASS** (test_rule_book Group 4)

**Notes:**
- Parser JSON hand-written nel loader (no dipendenze esterne); supporta
  oggetti, array, stringhe e interi — sufficiente per il formato regole usato.
- `RuleBookLoader` segnala tutti gli errori via `ERuleBookError`.
- `gmRulesEngine` espone `load_rules_json()`, `load_rules_json_string()`,
  `resolve_rule()`, `resolve_rules()` e l'accessor `rule_book()`.
- Accumulo: chiamare `load_rules_json*` più volte aggiunge definizioni senza
  cancellare quelle esistenti; necessario per caricare più file di regole.

### Phase 6 — RuleContext::modify_resource + integrazione sandbox 🔧

**Obiettivo:** Estendere `RuleContext` con il metodo `modify_resource()` per
permettere agli effetti `MODIFY_RESOURCE` di modificare risorse di gioco
(azioni, monete, acquisti). Aggiornare il mock sandbox.

- [x] `core/RuleContext.hpp` — `modify_resource(actor_id, resource, delta)` (già presente)
- [x] `tests/MockRuleContext.hpp` — stub `modify_resource` + `set_resource_max` (già presente)
- [x] `GAME/gmGui-Sandbox/data/dominion_rules.json` — file effetti carte Dominion
- [x] `GAME/gmGui-Sandbox/mock_engine.py` — carica `dominion_rules.json`, esegue regole nel terminale
- [ ] Smoke test: mock_engine stampa `[effect] Player_X.actions += 1` quando Village va in PlayArea ← smoke test

---

## Key Design Decisions

### 1. `RuleBook` è separato da `RuleGroupRegistry`

`RuleGroupRegistry` risponde a "quali gruppi sono attivi" (WHAT).
`RuleBook` risponde a "cosa fa una regola" (HOW).
Questa separazione permette di usare i due componenti in modo indipendente.

### 2. Formato JSON di input per `RuleBookLoader`

```json
{
  "rules": [
    {
      "rule_id": "r_add_action_1",
      "description": "+1 Azione",
      "effects": [
        {
          "type": "MODIFY_RESOURCE",
          "target": "SELF",
          "value": "actions",
          "amount": 1
        }
      ]
    },
    {
      "rule_id": "r_draw_3",
      "description": "Pesca 3 carte",
      "effects": [
        {
          "type": "DRAW_CARDS",
          "target": "SELF",
          "value": "main_deck",
          "amount": 3
        }
      ]
    }
  ]
}
```

### 3. `RuleBook::resolve_rule` vs `gmRulesEngine::resolve_rule`

`RuleBook::resolve_rule(id, actor, targets, ctx)` esegue la sequenza completa:
precondizioni → effetti.  `gmRulesEngine` espone il metodo come facciata.
Il chiamante può usare direttamente `RuleBook` o passare per `gmRulesEngine`.

### 4. Nessuna dipendenza da nlohmann/json

Il `RuleBookLoader` userà un parser JSON minimalista senza dipendenze esterne,
compatibile con C++17 stdlib.  Supporta solo il sottoinsieme necessario:
oggetti, array, stringhe, interi.  Tipi non supportati (float, null, bool
nidificato) generano `ERuleBookError`.
