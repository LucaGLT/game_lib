# Riassunto Decisioni — Giugno 2026

**Data chiusura thread:** 2026-06-28  
**Sostituisce:** `RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md` (versione precedente, meno aggiornata)  
**Scopo:** Conservare le decisioni operative e lo stato di avanzamento di tutti i filoni
di lavoro, così da poter aprire nuovi thread senza rileggere tutta la storia.

---

## A — Stato librerie C++ (game_lib)

### A1 — gmFlow ✅ Completato (Phase 5–15)

Tutte le fasi del piano `gmFlow/FlowPhase_PLAN.md` sono complete.

**Cosa è stato costruito (additivo, retrocompatibile):**

| Componente | File | Note |
|---|---|---|
| `PhaseContext` | `flow/PhaseContext.hpp/.cpp` | Sottoclasse di `GameContext`; prende in prestito GameState/ActorRegistry/EventBus dal genitore; IDs round/turn/phase locali e isolati |
| `FlowPhase` | `flow/FlowPhase.hpp/.cpp` | Implementa `IPhase`; possiede internamente un `IFlowController` + `PhaseContext`; dal parent è una fase normale |
| `SequentialFlowController` | (modifiche minori) | `_round_index` → `protected`; `advance_phase()` → `virtual protected`; aggiunto `current_phase() const` |
| `FlowRulesGateway` | `bridges/FlowRulesGateway.hpp/.cpp` | Registra fino a 10 callback al lifecycle EventBus; payload `FlowRulesPayload` (struct flat, copiabile) |
| `ActionGateway` | `bridges/ActionGateway.hpp/.cpp` | Decorator `IAction` con `ActionPreCheck` (blocco in `validate()`) e `ActionPostHook` (in `execute()`) |
| Test integrazione | `tests/test_flow_rules_integration.cpp` | 10 test; 7/7 suite CTest gmFlow passano |

**Decisioni di design vincolanti (da non modificare):**

- D1: `PhaseContext` estende `GameContext` via costruttore. Nessun cambio a `GameContext`.
- D2: `GameState`, `ActorRegistry`, `EventBus` sono *presi a prestito* (by reference) — mai duplicati.
- D3: IDs locali (`current_round_id`, `current_turn_id`, `current_phase_id`) sono posseduti da `PhaseContext`.
- D4: `scope_prefix` (es. `"epoch_1"`) è disponibile in `PhaseContext`; il caller costruisce l'ID qualificato se serve.
- D5: `FlowPhase` è un `IPhase` normale per il controller genitore. Gerarchia trasparente.
- D8: `ActionQueue` resta globale e unica per sessione. Le azioni dei livelli interni entrano nella stessa coda.
- D9: Nessun limite di nesting imposto dal framework. Limite pratico consigliato: 3 livelli.
- D10: `GameSession` e tutti i test esistenti non richiedono modifiche.
- R6 (ActionGateway): solo `validate()` può bloccare un'azione; `execute()` chiama sempre inner + post-hook.

**`FlowRulesPayload` — campi:**

```cpp
struct FlowRulesPayload {
    std::string actor_id, action_id, phase_id, round_id, turn_id;
    std::string scope_prefix;  // non-empty se dentro FlowPhase
    std::string event_type;    // costante EVT_* che ha scatenato il callback
};
```

**Chiamate CTest (tutto Release):** `gmFlow_action_queue`, `gmFlow_action_window`,
`gmFlow_sequential`, `gmFlow_campaign`, `gmFlow_timeline`, `gmFlow_flow_phase`,
`gmFlow_flow_rules_integration` → **7/7 PASS**.

---

### A2 — gmRules ✅/🔧 Fasi 1–6 complete, Phase 7 pending

| Phase | Contenuto | Stato |
|---|---|---|
| 1 | Interfacce & stub (`RuleContext`, `RuleResult`, `EffectSpec`, ecc.) | ✅ |
| 2 | Core resolvers (`TargetResolver`, `ConditionEvaluator`, `EffectResolver`) | ✅ |
| 3 | Status engine (`StatusDefinition`, `StatusInstance`, `StatusEngine`) | ✅ |
| 4 | Facade + RuleGroup registry (`gmRulesEngine`, `RuleGroupRegistry`) | ✅ |
| 5 | RuleBook + Loader (JSON → `RuleBook`, `RuleBookLoader`) | ✅ |
| 6 | `RuleContext::modify_resource` + sandbox Python (`mock_engine.py`) | ✅ |
| 7 | CMakeLists.txt + build + CTest C++ per `test_rule_book` | ⏳ PROSSIMO |

**Architettura confermata:**

- `RuleGroupRegistry` → risponde a QUALE set di regole è attivo (WHAT). Non esegue.
- `RuleBook` → mappa `RuleId → RuleDefinition → EffectSpec[]`.
- `RuleBookLoader` → parser JSON hand-written (no dipendenze esterne); accumulo: più load addono senza azzerare.
- `gmRulesEngine` → orchestra tutto; espone `load_rules_json()`, `resolve_rule()`, `resolve_rules()`.
- Runtime refs GRS (`input.xxx`, `event.xxx`) rendono le regole parametriche e riutilizzabili.
- Add-on rules a runtime: supportato e desiderato. Load multipli si accumulano.

**Policy merge/override (aperta):** non ancora definita per `RuleId` in conflitto tra pacchetti add-on distinti.

---

### A3 — gmAlea ✅ Base stabile

- `GmDeck`, `GmCompDeck`, `GmDice`, `StdDice` implementati e testati.
- `GmCompDeck` supporta zone: `MainDeck`, `CardHand`, `Memory`, `DiscardPile`, `PlayArea`, `Banish`.
- `CardRuleBridge` collega `GmCompDeck::ZoneChangeCallback` a `RuleGroupRegistry`.
- Feature F1 (`CardType` + `SequenceEngine`) completa: 29/29 test.
- **Pending per integrazione:** eventi `TOKEN_PRE_DRAW`, `TOKEN_DRAWN`, `DICE_PRE_ROLL`, `DICE_ROLLED`, `ALEA_RESOLVED` + effetti `SHUFFLE_ZONE`, `LOOK_TOP_CARD`, `ROLL_DICE`, ecc. (Capitolo 5 del piano grs-integration).

---

### A4 — gmActor ✅ Base stabile

- `ActorStateCommon` (HP, status, tag, modifier), `MonsterGroupState`, `BossState` implementati.
- Feature F3 (`FormationValidator`/`Resolver`): 35/35 test.
- Feature F4 (`BehaviorCardProcessor`/Reactions): 20/20 test.
- **Pending per integrazione:** eventi `ACTOR_SPAWNED`, `ACTOR_HP_CHANGED`, `ACTOR_DIED`, ecc. + effetti `MODIFY_RESOURCE`, `EQUIP_ITEM`, `SPAWN_ACTOR`, ecc. (Capitolo 4 del piano grs-integration).

---

### A5 — gmMap ✅ Base stabile

- Location/Tile management, topologia base implementati.
- **Pending per integrazione:** eventi `ACTOR_MOVED`, `PATH_BLOCKED`, `LOS_CHANGED` + effetti `SET_LOCATION_PASSABLE`, `CREATE_BARRIER`, ecc. (Capitolo 6 del piano grs-integration).

---

### A6 — gmDispatch, gmLog, gmSave ✅ Stabili

Non toccati in questo thread. Usati come dipendenze da gmFlow e gmRules.

---

## B — Piano integrazione librerie (grs-integration-implementation-plan.md)

File: `gmRules/specs/grs-integration-implementation-plan.md`

| Capitolo | Libreria | Priorità | Stato |
|---|---|---|---|
| 1 | gmRules (DSL/vocab esteso) | P0 | ⏳ Pending |
| 2 | gmFlow (lifecycle → gmRules) | P1 | ✅ **Completato** (Phase 13–15 gmFlow) |
| 3 | gmDispatch (bridge eventi) | P2 | ⏳ Pending |
| 4 | gmActor (eventi/effetti actor) | P3 | ⏳ Pending |
| 5 | gmAlea (pre/post resolve) | P4 | ⏳ Pending |
| 6 | gmMap (eventi spatial) | P5 | ⏳ Pending |

**Prossimo lavoro C++:** Capitolo 1 (estendere vocabolario GRS) poi Capitolo 7 gmRules (build + CTest).

---

## C — GUI Python (gmGui + gmGui-Sandbox)

### C1 — gmGui ✅ v0.1 production-ready

Libreria di widget generici PySide6 data-driven (nessun concetto di dominio di gioco).

| Widget | Evento consumato |
|---|---|
| `TimelineWidget` | `gmflow.timeline.actors_updated` |
| `FormationWidget` | `gmactor.formation.updated` |
| `SequenceStateWidget` | `gmalea.sequence.state_changed` |
| `BehaviorCardWidget` | `gmactor.behavior.card_changed` |

Architettura: Engine C++ → JSON events → EventBus (gmDispatch) → Widget PySide6.
Styling: theme token + QSS. Nessun accoppiamento a logica di gioco.

### C2 — gmGui-Sandbox (mock engine) ✅ Phase 6 funzionante

File: `GAME/gmGui-Sandbox/mock_engine.py`

Stato attuale (Phase 6 completa):
- Carica `data/dominion_rules.json` e `data/rule_groups.json`.
- Simula `CardRuleBridge + RuleGroupRegistry`: carta in `PlayArea`/`Memory` → emette `gmRules.rule_group.activated/deactivated`.
- Esegue effetti via `_apply_rule_effects`: gestisce `MODIFY_RESOURCE`, `DRAW_CARDS`.
- `GmActorModule` mostra sezione **Risorse** separata dagli Status; si aggiorna live su `gmActor.actor.resource_changed`.
- Smoke test PASS: `mock_engine` stampa `[effect] Player_X.actions +2 (→ 2)` per Village (rg_village → r_add_actions_2).
- Costo automatico: giocata carta da `CardHand` → `PlayArea` deduce 1 azione.
- `DRAW_CARDS` muove carte reali da `MainDeck` a `CardHand` e emette `gmAlea.deck.card_moved`.

**Pending:** Phase 7 mock engine (dipende da P6 Eldhom + P1 JSON data).

**Porta TCP:** EVENT_PORT = 9000 (GUI in ascolto).

---

## D — Gioco Eldhôm (Le Pergamene di Eldhôm)

### D1 — Identità gameplay (immutabile)

- Dungeon crawler card-based con deckbuilding.
- **Timeline continua** (nessun Round classico) — usa `gmFlow::TimelineFlowController`.
- Formazioni tattiche Prima Linea / Retroguardia.
- Gruppi mostri con Carte Comportamento autonome.
- Core C++ (regole/stato/flusso) + GUI Python/PySide6 (visualizzazione/interazione).

### D2 — Stato sviluppo Eldhôm

| Fase | Componente | Stato |
|---|---|---|
| P2 | CardType + SequenceEngine | ✅ 29/29 test |
| P3 | FormationValidator + Resolver | ✅ 35/35 test |
| P4 | BehaviorCardProcessor + Reactions | ✅ 20/20 test |
| P8 | 4 widget PySide6 generici | ✅ base completa |
| P6 | EldhomEngine + RuleAdapter | 🚀 NEXT (nessun blocco) |
| P1 | JSON Data Layer | ⏳ Può partire in parallelo con P6 |
| P5 | MissionEventSystem | ⏳ Dipende da P6 |
| P7 | Mock Engine GUI | ⏳ Dipende da P6+P1 |
| P10 | Integration Test | ⏳ Ultima fase |

**Nessun blocco.** Tutte le dipendenze F1–F5 sono production-ready.

### D3 — Ruolo delle librerie in Eldhôm

| Libreria | Ruolo |
|---|---|
| `gmFlow` | Ordine attori (timeline), avanzamento fasi, FlowPhase per gerarchie |
| `gmRules` | Risoluzione effetti carte/azioni, condizioni trigger missione |
| `gmAlea` | Deck/zone lifecycle (mano, scarti, memoria, mazzo), sequenze tipo carta |
| `gmActor` | Stato attori, formazione Frontline/Backline, behavior/reaction mostri |
| `gmDispatch` | Bridge eventi Core↔GUI, serializzazione JSON payload |
| `gmGui` | Widget visualizzazione timeline, formazione, sequenze, carte comportamento |

---

## E — Decision Log (per i prossimi thread)

| ID | Decisione |
|---|---|
| DLOG-01 | Eldhôm usa timeline continua gmFlow (`TimelineFlowController`), non round classici. |
| DLOG-02 | gmRules separa attivazione gruppi (`RuleGroupRegistry`) ed esecuzione (`RuleBook`/Engine). |
| DLOG-03 | Regole add-on runtime abilitate via load multiplo (`load_rules_json`); accumulo senza reset. |
| DLOG-04 | Core C++ mantiene autorità su stato e regole; GUI Python è consumer event-driven. |
| DLOG-05 | Estensioni a gmXxx ammesse solo additive e retrocompatibili. |
| DLOG-06 | `FlowPhase` è un `IPhase` normale per il parent controller; gerarchia di sessioni trasparente. |
| DLOG-07 | `ActionGateway`: solo `validate()` blocca (pre-check); `execute()` sempre esegue inner + post-hook. |
| DLOG-08 | `ActionQueue` è globale per sessione; le azioni di livelli FlowPhase interni entrano nella stessa coda. |
| DLOG-09 | Parser JSON di `RuleBookLoader` è hand-written, senza dipendenze esterne. |
| DLOG-10 | Sandbox mock_engine simula CardRuleBridge+RuleGroupRegistry in Python; porta TCP 9000. |

---

## F — Prossimi passi prioritari (ordine suggerito)

1. **gmRules Phase 7** — Aggiungere `test_rule_book` al CMakeLists.txt, compilare, CTest PASS.
2. **grs-integration Cap 1** — Estendere vocabolario GRS (nuovi TriggerType, EffectType, parser).
3. **Eldhôm P6** — `EldhomEngine` + `RuleAdapter` (il prossimo milestone di gioco).
4. **grs-integration Cap 4** — Adapter gmActor (eventi HP/morte/risorse + effetti).
5. **grs-integration Cap 5** — Adapter gmAlea (pre/post resolve draw/roll).

---

## G — File di riferimento chiave

| Argomento | File |
|---|---|
| gmFlow piano completo | `gmFlow/FlowPhase_PLAN.md` |
| gmFlow API | `gmFlow/gmFlow_API.md` |
| gmRules piano fasi | `gmRules/PLAN.md` |
| Piano integrazione librerie | `gmRules/specs/grs-integration-implementation-plan.md` |
| Piano feature generiche | `plan_gm_lib.md` |
| Piano Eldhôm | `GAME/Eldhom/info/PLAN.md` |
| Sandbox mock engine | `GAME/gmGui-Sandbox/mock_engine.py` |
| gmGui API | `pyLib/gmGui/gmGui_API.md` |
| Regole Dominion (esempio) | `GAME/gmGui-Sandbox/data/dominion_rules.json` |
| GRS spec | `gmRules/specs/grs-spec.md` |
| GRS manuale | `tools/GRS_MANUAL.md` |
| Style rules C++ | `style-rules.md` |
