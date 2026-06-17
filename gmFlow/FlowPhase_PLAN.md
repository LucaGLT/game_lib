# gmFlow — FlowPhase Extension Plan

**Version:** 2.0
**Status:** Phase 5–15 — Completato ✅
**Language:** C++17 Standard
**Namespace:** `gmFlow`
**Scope:** Extension of existing gmFlow infrastructure (additive, backward-compatible)
**Tracks:** FlowPhase (Ph 5–10) · gmFlow↔gmRules Integration (Ph 11–15)

---

## Goal

Introdurre `PhaseContext` e `FlowPhase` come primitivi di primo livello in gmFlow
per supportare gerarchie di gioco arbitrariamente profonde (Campaign → Session →
Epoch → RoundPhase → Turn → Action → ActionStep) senza corrompere il contesto
della sessione radice. `FlowPhase` è un `IPhase` che possiede internamente un
proprio `IFlowController` e un `PhaseContext` isolato; riceve però per riferimento
i servizi condivisi della sessione (`GameState`, `ActorRegistry`, `EventBus`) dal
contesto genitore — garantendo che le mutazioni di stato siano sempre globali
mentre i contatori locali (round, turn, phase ID) rimangono isolati. Il design è
**completamente additivo**: `GameSession`, `IPhase`, `IFlowController` e tutti i
test esistenti compilano senza modifiche.

---

## Decisioni di Design Vincolanti

| # | Decisione | Scelta |
|---|-----------|--------|
| D1 | `PhaseContext` vs `GameContext` | `PhaseContext` **estende** `GameContext` chiamandone il costruttore con i valori estratti dal genitore. Nessun cambio a `GameContext`. |
| D2 | Servizi condivisi | `GameState`, `ActorRegistry`, `EventBus` sono presi a **prestito** (by reference) dal genitore. Non vengono duplicati. |
| D3 | ID locali | `current_phase_id`, `current_round_id`, `current_turn_id` sono **posseduti** da `PhaseContext` e isolati dal genitore. |
| D4 | Scope prefix | `PhaseContext` riceve un `scope_prefix` (es. `"epoch_1"`). Il controller interno genera ID semplici (`"round_1"`); il prefisso è disponibile per chi vuole costruire ID qualificati (`"epoch_1.round_1"`). |
| D5 | `FlowPhase` e `IPhase` | `FlowPhase` **implementa** `IPhase` — dal punto di vista del controller genitore è una fase normale. Internamente possiede `IFlowController` + `PhaseContext`. |
| D6 | `available_actions()` | `FlowPhase` delega all'**ultima fase interna attiva** via `SequentialFlowController::current_phase()` (da aggiungere). |
| D7 | `SequentialFlowController` | Soli tre cambiamenti: `_round_index` → `protected`; `advance_phase()` → `virtual`; aggiunto `current_phase() const`. |
| D8 | `ActionQueue` | Resta **globale** e unica per sessione. Le azioni dei livelli interni entrano nella stessa coda. |
| D9 | Nesting massimo | Nessun limite imposto dal framework. Limite pratico consigliato: 3 livelli. |
| D10 | Backward compatibility | `GameSession` e tutti gli usi esistenti di `IPhase` non richiedono modifiche. |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│ GameSession  (Session root — invariato)                          │
│   owns: GameContext (root), ActionQueue, ActorRegistry, EventBus│
└──────────────────────────┬──────────────────────────────────────┘
                           │ tick() → IFlowController::process()
┌──────────────────────────▼──────────────────────────────────────┐
│ SequentialFlowController  (parent controller)                    │
│   phases[]: IPhase*                                              │
│     ├─ SetupPhase   (plain IPhase)                               │
│     ├─ FlowPhase    (NEW — contiene sub-controller)              │  ← FlowPhase vista come IPhase dal parent
│     │    ├─ PhaseContext (OWNED — IDs locali, servizi borrowed)  │
│     │    └─ SequentialFlowController (OWNED — sub-controller)   │
│     │         phases[]: IPhase*                                  │
│     │           ├─ MorningPhase  (plain IPhase)                  │
│     │           └─ EveningPhase  (plain IPhase)                  │
│     │                └─ Turn → ActionWindow → Action → Step      │
│     └─ EndPhase  (plain IPhase)                                   │
└─────────────────────────────────────────────────────────────────┘

Shared (by reference — mai duplicati):
  GameState ─────────────────────────────► tutti i livelli
  ActorRegistry ─────────────────────────► tutti i livelli
  EventBus ──────────────────────────────► tutti i livelli

Isolated (owned per livello):
  PhaseContext._current_phase_id  (solo FlowPhase)
  PhaseContext._current_round_id  (solo FlowPhase)
  PhaseContext._current_turn_id   (solo FlowPhase)
```

---

## File Structure

```
gmFlow/
├── FlowPhase_PLAN.md              ← questo file
│
├── core/
│   └── GameContext.hpp / .cpp     ← INVARIATO
│
├── flow/
│   ├── IPhase.hpp                 ← INVARIATO
│   ├── IFlowController.hpp        ← INVARIATO
│   ├── SequentialFlowController.hpp/.cpp  ← 3 modifiche minori (D7)
│   ├── TurnPolicy.hpp             ← INVARIATO
│   ├── RoundPolicy.hpp            ← INVARIATO
│   ├── Turn.hpp / .cpp            ← INVARIATO
│   ├── Round.hpp / .cpp           ← INVARIATO
│   ├── PhaseContext.hpp           ← NUOVO — subclasse di GameContext (IDs locali)
│   ├── PhaseContext.cpp           ← NUOVO — implementazione
│   ├── FlowPhase.hpp              ← NUOVO — IPhase che possiede controller + PhaseContext
│   └── FlowPhase.cpp              ← NUOVO — implementazione
│
├── tests/
│   └── test_flow_phase.cpp        ← NUOVO — 10 test cases
│
├── CMakeLists.txt                 ← aggiungere PhaseContext.cpp, FlowPhase.cpp, test
└── gmFlow_API.md                  ← aggiungere sezione FlowPhase
```

---

## Development Phases

### Phase 5 — PhaseContext ✅

**Goal:** Classe che estende `GameContext` con IDs locali isolati e scope prefix.

- [x] Creare `gmFlow/flow/PhaseContext.hpp`
  - `class PhaseContext : public GameContext`
  - Costruttore `PhaseContext(GameContext& parent, std::string scope_prefix)`
    chiama `GameContext(parent.session_id(), parent.state(), parent.actor_registry(), parent.event_bus())`
  - Campo `_scope_prefix` (std::string, private)
  - Metodo `const std::string& scope_prefix() const`
  - Include guard: `#ifndef GMFLOW_PHASECONTEXT_HPP`
  - Doxygen su tutti i simboli pubblici
- [x] Creare `gmFlow/flow/PhaseContext.cpp`
  - Implementazione costruttore e metodi
- [x] Verificare che `PhaseContext` accetti i metodi `set_current_round_id()` etc.
  di `GameContext` senza override aggiuntivi
- [x] Compilare senza errori (nessuna modifica al resto)

**Notes:**
`GameContext` non è copiabile ma il costruttore `GameContext(SessionId, GameState&,
ActorRegistry&, EventBus&)` è pubblico — `PhaseContext` lo chiama estraendo i valori
dal genitore. Non è una copia: è una nuova istanza che condivide i riferimenti.
`_current_phase_id`, `_current_round_id`, `_current_turn_id` ereditati da
`GameContext` sono distinti per ogni `PhaseContext` — il sub-controller li imposta
sulla `PhaseContext` senza toccare il `GameContext` radice.

---

### Phase 6 — SequentialFlowController Refactoring ✅

**Goal:** Tre modifiche minori per abilitare subclassing e accesso alla fase corrente.

- [x] In `SequentialFlowController.hpp`:
  - Spostare `_round_index` da `private` a `protected`
  - Rendere `advance_phase(GameContext& ctx)` `virtual` (era `private void`)
  - Aggiungere metodo pubblico `const IPhase* current_phase() const`
    (ritorna `_phases[_current_phase_index].get()`, o `nullptr` se fuori range)
- [x] In `SequentialFlowController.cpp`:
  - Implementare `current_phase()`
- [x] Aggiungere Doxygen al nuovo metodo e al campo promosso
- [ ] Verificare che tutti i test esistenti (`test_flow_sequential.cpp`) passino
  senza modifiche

**Notes:**
Solo `_round_index` viene promosso a `protected`; gli altri campi (
`_current_phase_index`, `_current_actor_index`, `_session_complete`,
`_rounds_exhausted`, `_current_window`) rimangono `private` — non servono a
`FlowPhase`. `advance_phase()` diventa `virtual` così una subclass può ridefinire
quando il round counter si incrementa (es. ad ogni cambio di Phase invece che ad
ogni ciclo di Turn — cfr. discussione architetturale).

---

### Phase 7 — FlowPhase Implementation ✅

**Goal:** `IPhase` che possiede un controller interno e un `PhaseContext` isolato.

- [x] Creare `gmFlow/flow/FlowPhase.hpp`
- [x] Costruttore: riceve scope_prefix e controller (il controller è già costruito con le sub_phases)
- [x] `on_enter(GameContext& parent_ctx)`:
  - Costruisce `_phase_ctx` da `parent_ctx` e `_scope_prefix`
    (`_phase_ctx = PhaseContext(parent_ctx, _scope_prefix)`)
  - Chiama `_controller->start(_phase_ctx)`
  - Imposta `_entered = true`
- [x] `on_exit(GameContext&)`: resetta `_entered = false`, cleanup opzionale
- [x] `is_complete(const GameContext&)`:
  ritorna `_entered && _controller->is_session_complete(_phase_ctx)`
- [x] `available_actions(const GameContext&, actor)`:
  - Ottiene la fase interna corrente via `SequentialFlowController::current_phase()`
    (cast dinamico se necessario)
  - Delega `current_phase->available_actions(_phase_ctx, actor)`
  - Ritorna vuoto se `!_entered` o fase corrente non disponibile
- [x] `tick(GameContext&)`: chiama `_controller->process(_phase_ctx)` — necessario
  se il genitore non chiama `process` abbastanza frequentemente
- [x] Include guard: `#ifndef GMFLOW_FLOWPHASE_HPP`
- [x] Doxygen completo su tutti i simboli pubblici
- [x] Creare `gmFlow/flow/FlowPhase.cpp` con tutte le implementazioni
- [x] `accept_action()` e `can_actor_act()`: routing trasparente alla FlowPhase
  da `SequentialFlowController::accept_action()` e `can_actor_act()` (via dynamic_cast nel .cpp)
- [ ] Verificare compilazione pulita

**Notes:**
`_phase_ctx` non può essere inizializzato nel costruttore di `FlowPhase` perché
il `GameContext` genitore non è ancora disponibile — arriva solo in `on_enter()`.
Si usa un `PhaseContext` con costruttore default oppure `std::optional<PhaseContext>`.
Preferire `std::optional<PhaseContext> _phase_ctx` per chiarezza semantica e
per evitare un default-constructor su `GameContext` che è non-triviale.
Il `tick()` è separato da `on_enter()` perché il controller interno deve essere
guidato ogni tick dalla sessione esterna; il genitore chiama `process()` sulla
`FlowPhase` (che internamente chiama `_controller->process(_phase_ctx)`).

---

### Phase 8 — Unit Tests ✅

**Goal:** Verificare isolamento dei contesti, condivisione dello stato, nesting.

- [ ] Creare `gmFlow/tests/test_flow_phase.cpp`
- [ ] **Test 1** — `PhaseContext` costruito da `GameContext` condivide `GameState`
  (mutazione visibile in entrambi i livelli)
- [ ] **Test 2** — `PhaseContext` costruito da `GameContext` ha `round_id` isolato
  (set su `PhaseContext` non modifica `GameContext` genitore)
- [ ] **Test 3** — `PhaseContext` costruito da `GameContext` ha `phase_id` isolato
- [ ] **Test 4** — `PhaseContext` costruito da `GameContext` ha `turn_id` isolato
- [ ] **Test 5** — `FlowPhase` a singolo livello: sub-phase completate → `is_complete()`
  ritorna `true`
- [ ] **Test 6** — `FlowPhase` a singolo livello: eventi `EVT_PHASE_ENTERED` /
  `EVT_ROUND_STARTED` sono pubblicati sull'`EventBus` condiviso
- [ ] **Test 7** — `FlowPhase` a due livelli (Epoch → Day): `GameState` condiviso
  visibile a entrambi i livelli
- [ ] **Test 8** — `FlowPhase` a due livelli: `round_id` di Epoch e `round_id` di
  Day sono stringhe diverse e non si sovrascrivono
- [ ] **Test 9** — `FlowPhase::available_actions()` ritorna le azioni della fase
  interna corrente
- [ ] **Test 10** — `FlowPhase` dentro `SequentialFlowController` come fase normale:
  la sessione radice completa correttamente dopo che `FlowPhase::is_complete()`
  diventa `true`
- [ ] Aggiornare `gmFlow/CMakeLists.txt` con nuovo test target `gmFlow_flow_phase`

**Notes:**
I test usano implementazioni mock di `GameState` (derivata), `IFlowController`
(stub) e `IPhase` (stub) già presenti nei test esistenti. Evitare dipendenze
esterne. Ogni test è indipendente (no shared fixtures state).

---

### Phase 9 — Build Integration ✅

**Goal:** Aggiungere nuovi file al sistema di build CMake.

- [x] In `gmFlow/CMakeLists.txt`:
  - Aggiunto `flow/PhaseContext.cpp` alla lista sorgenti della libreria
  - Aggiunto `flow/FlowPhase.cpp` alla lista sorgenti della libreria
  - Aggiunto target test `gmFlow_flow_phase` con `tests/test_flow_phase.cpp`
- [x] Eseguire `cmake --build build --config Debug`
- [x] Eseguire `ctest --test-dir build -R gmFlow` — tutti i test esistenti devono
  passare (regressione zero)
- [x] Eseguire `ctest --test-dir build -R gmFlow_flow_phase` — nuovi 10 test passano

---

### Phase 10 — Documentation ✅

**Goal:** Aggiornare la documentazione pubblica di gmFlow.

- [x] Aggiornato `gmFlow/gmFlow_API.md`:
  - Aggiornato **Status** a `Phase 5–7 — FlowPhase implemented`
  - Aggiornato **File Structure** con `PhaseContext.hpp/.cpp` e `FlowPhase.hpp/.cpp`
  - Aggiornato **Table of Contents** con `PhaseContext` e `FlowPhase`
  - Aggiunta sezione `PhaseContext` nella parte `flow/ — Flow Control`
  - Aggiunta sezione `FlowPhase` nella parte `flow/ — Flow Control`
  - Aggiornata sezione `SequentialFlowController` con V2 API (`current_phase()`, `_round_index` protected, `advance_phase()` virtual)
  - Aggiunto esempio d’uso `Epoch Nested with FlowPhase` in `Usage Examples`
- [ ] Aggiungere `PhaseContextId` (alias `ScopeId`) in `gmFlow/core/Ids.hpp` (opzionale)

---

---

## Capitolo 2 — gmFlow ↔ gmRules Integration

> Queste fasi sono **indipendenti** da FlowPhase (Fasi 5–10) e possono procedere
> in parallelo. Dipendono solo dall'infrastruttura gmFlow V1 già completa.

---

### Decisioni di Design — Integrazione gmFlow/gmRules

| # | Decisione | Scelta |
|---|-----------|--------|
| R1 | gmRules non dipende da gmFlow | Confermato — il contratto è basato su stringhe evento e `RuleContext` astratto |
| R2 | Adapter in `gmFlow/bridges/` | L'adapter `GmRulesFlowBridge` vive in `gmFlow/bridges/` — dipende da entrambe le lib ma non le modifica |
| R3 | Lifecycle hooks via EventBus | gmFlow pubblica eventi già esistenti (`EVT_TURN_STARTED` ecc.); l'adapter sottoscrive e invoca gmRules |
| R4 | Action gateway | `IAction::validate()` può invocare gmRules via `RuleContext`; è game-specific code, non infrastruttura |
| R5 | Payload minimo stabile | Ogni evento gateway porta: `actor_id`, `action_id`, `phase_id`, `round_index`, `turn_index` |
| R6 | Outcome deterministico | Se gmRules blocca un'azione → `ValidationResult::fail(RULE_VIOLATION, reason)` |
| R7 | FlowPhase e scope_prefix | Con FlowPhase attiva, il payload include anche `scope_prefix` per identificare il livello gerarchico |
| R8 | Thread safety | Bridge sincrono (stessa strategia di `SyncDispatcher`); async fuori scope V2 |

---

### Phase 11 — FlowRulesGateway ✅

**Goal:** Interfaccia che connette il lifecycle di gmFlow al motore di regole gmRules.
Nessuna modifica ai file esistenti di gmFlow o gmRules.

**File nuovo:** `gmFlow/bridges/FlowRulesGateway.hpp`

```cpp
namespace gmFlow {

/// Payload minimale passato a gmRules per ogni evento lifecycle.
struct FlowRulesPayload {
    std::string actor_id;       // attore corrente (vuoto se evento non-actor)
    std::string action_id;      // azione in corso (vuoto se non applicabile)
    std::string phase_id;       // fase corrente dal GameContext
    std::string round_id;       // round corrente dal GameContext
    std::string turn_id;        // turn corrente dal GameContext
    std::string scope_prefix;   // prefisso PhaseContext (vuoto se root session)
    std::string event_type;     // stringa EVT_xxx di gmFlow
};

/// Callback invocata dal bridge per ogni evento lifecycle.
/// Il game engine implementa questo per delegare a gmRulesEngine.
using FlowRulesCallback = std::function<bool(const FlowRulesPayload&)>;
// Ritorna true se l'evento è consentito/passato; false se bloccato.

/// Registra i callback sui lifecycle events dell'EventBus.
void register_flow_rules_gateway(
    EventBus&          event_bus,
    FlowRulesCallback  on_turn_started,
    FlowRulesCallback  on_turn_ended,
    FlowRulesCallback  on_round_started,
    FlowRulesCallback  on_round_ended,
    FlowRulesCallback  on_phase_entered,
    FlowRulesCallback  on_phase_exited,
    FlowRulesCallback  on_window_opened,
    FlowRulesCallback  on_window_closed,
    FlowRulesCallback  on_action_submitted,   // pre-check
    FlowRulesCallback  on_action_completed);  // post-check

} // namespace gmFlow
```

Checklist:

- [x] Creare `gmFlow/bridges/` directory
- [x] Creare `gmFlow/bridges/FlowRulesGateway.hpp`
  - Definire `struct FlowRulesPayload` con i campi R5
  - Definire `using FlowRulesCallback`
  - Dichiarare `register_flow_rules_gateway()`
  - Include guard `#ifndef GMFLOW_FLOWRULESGATEWAY_HPP`
  - Doxygen completo
- [x] Creare `gmFlow/bridges/FlowRulesGateway.cpp`
  - Implementare `register_flow_rules_gateway()`:
    sottoscrive all’`EventBus` per ogni evento elencato
    chiama il callback corrispondente con payload costruito dall’evento
  - Costruire `FlowRulesPayload` castando l’`IEvent` al tipo concreto
    (`TurnStartedEvent`, `RoundStartedEvent`, `ActionSubmittedEvent`, ecc.)
- [x] Aggiungere `bridges/FlowRulesGateway.cpp` a `gmFlow/CMakeLists.txt`

**Notes:**
`register_flow_rules_gateway()` non ritorna nulla e non lancia — se un callback
è `nullptr`, la sottoscrizione per quell'evento viene saltata silenziosamente.
Il `scope_prefix` nel payload è vuoto quando si opera sul `GameContext` radice;
viene popolato quando il chiamante passa un `PhaseContext` (Fase 11 è compatibile
con FlowPhase ma non ne dipende).

---

### Phase 12 — ActionGateway (pre/post check) ✅

**Goal:** Meccanismo per bloccare o modificare un'azione prima e dopo l'esecuzione
tramite gmRules, senza modificare `IAction` o `GameSession`.

**Strategia:** `ActionGateway` è un adapter che wrappa una `IAction` esistente,
aggiungendo un pre-check (via `validate()` esteso) e un post-check
(via `on_completed` callback). Il game engine usa `ActionGateway::wrap()` per
decorare ogni azione prima di passarla a `submit_action()`.

**File nuovo:** `gmFlow/bridges/ActionGateway.hpp` / `.cpp`

```cpp
namespace gmFlow {

/// Funzione di pre-check: ritorna fail se le regole bloccano l'azione.
using ActionPreCheck  = std::function<ValidationResult(
    const FlowRulesPayload&)>;

/// Funzione di post-hook: chiamata dopo execute() con il risultato.
using ActionPostHook  = std::function<void(
    const FlowRulesPayload&, const ActionResult&)>;

/// Wrappa una IAction con pre/post hooks gmRules.
class ActionGateway : public IAction {
public:
    ActionGateway(std::unique_ptr<IAction> inner,
                  FlowRulesPayload         payload,
                  ActionPreCheck           pre_check,
                  ActionPostHook           post_hook);

    ValidationResult validate(const GameContext& ctx) const override;
    ActionResult     execute(GameContext& ctx) override;

    // Delega tutto il resto all'inner action
    const ActionId&  id()           const override;
    const ActorId&   actor_id()     const override;
    ActionPriority   priority()     const override;
    ActionStatus     status()       const override;
    bool             is_async()     const override;
    bool             requires_turn()const override;
    bool             is_multi_step()const override;
};

} // namespace gmFlow
```

Checklist:

- [x] Creare `gmFlow/bridges/ActionGateway.hpp`
  - Dichiarare `using ActionPreCheck` e `using ActionPostHook`
  - Dichiarare `class ActionGateway : public IAction`
  - Include guard `#ifndef GMFLOW_ACTIONGATEWAY_HPP`
  - Doxygen su tutti i simboli pubblici
- [x] Creare `gmFlow/bridges/ActionGateway.cpp`
  - `validate()`: chiama prima `_inner->validate()`; se ok chiama `_pre_check(_payload)`;
    ritorna il primo fallimento
  - `execute()`: chiama `_inner->execute()`; poi chiama `_post_hook(_payload, result)`;
    ritorna il result dell’inner
  - Tutti gli altri metodi delegano a `*_inner`
- [x] Aggiungere `bridges/ActionGateway.cpp` a `gmFlow/CMakeLists.txt`

**Notes:**
`ActionGateway` **non blocca** in `execute()` — il post-hook è informativo.
Il blocco avviene solo in `validate()` via `pre_check`. Questa scelta garantisce
che un'azione già in esecuzione non venga interrotta a metà. Il `_payload` viene
costruito prima del `validate()` e include `phase_id`, `round_id`, `turn_id`
letti dal `GameContext` al momento della submission. Con `FlowPhase` attivo, il
chiamante può passare il `PhaseContext` corrente per ottenere gli ID locali.

---

### Phase 13 — Integration Tests ✅

**Goal:** Verificare il contratto completo gmFlow → gmRules su turno e round.

- [x] Creare `gmFlow/tests/test_flow_rules_integration.cpp`
- [x] **Test 1** — Gateway registrato: `EVT_TURN_STARTED` chiama il callback con
  `actor_id` corretto
- [x] **Test 2** — Gateway registrato: `EVT_ROUND_STARTED` chiama il callback con
  `round_id` corretto
- [x] **Test 3** — `ActionGateway` pre-check che ritorna `fail` → azione **non**
  arriva in `execute()`
- [x] **Test 4** — `ActionGateway` pre-check che ritorna `ok` → `execute()` viene
  chiamato
- [x] **Test 5** — `ActionGateway` post-hook chiamato con `ActionResult::success()`
  dopo esecuzione riuscita
- [x] **Test 6** — `ActionGateway` post-hook chiamato con `ActionResult::failure()`
  dopo esecuzione fallita
- [x] **Test 7** — Sequenza completa su un turno: `TURN_STARTED` → submit →
  pre-check ok → execute → post-hook → `TURN_ENDED` — tutti i callback invocati
  nell'ordine corretto
- [x] **Test 8** — Blocco da pre-check: `TURN_STARTED` scatta, azione bloccata da
  `RULE_VIOLATION`, `TURN_ENDED` scatta comunque
- [x] **Test 9** — Con `FlowPhase`: eventi del sub-controller includono
  `scope_prefix` nel payload
- [x] **Test 10** — Regressione: senza `ActionGateway`, il comportamento originale
  di `SequentialFlowController` è invariato
- [x] Aggiornare `gmFlow/CMakeLists.txt` con test target
  `gmFlow_flow_rules_integration`

---

### Phase 14 — Build Integration (Cap 2) ✅

- [x] In `gmFlow/CMakeLists.txt`:
  - Aggiungere `bridges/FlowRulesGateway.cpp`
  - Aggiungere `bridges/ActionGateway.cpp`
  - Aggiungere test target `gmFlow_flow_rules_integration`
- [x] `cmake --build build --config Release` — 0 errori
- [x] `ctest --test-dir build -R gmFlow` — 7/7 test passano (regressione zero)
- [x] `ctest --test-dir build -R gmFlow_flow_rules` — 10/10 PASS

---

### Phase 15 — Documentation (Cap 2) ✅

- [x] Aggiornare `gmFlow/gmFlow_API.md`:
  - Aggiungere sezione `bridges/ — Rules Integration`
  - Documentare `FlowRulesGateway` con tabella eventi + payload
  - Documentare `ActionGateway` con sequenza pre/post check
  - Aggiungere esempio d'uso end-to-end (turno completo con regole)
- [x] Aggiornare `gmRules/specs/grs-integration-implementation-plan.md`:
  - Capitolo 2 marcato come completato

---

## Dipendenze tra Fasi

```
Phase 5 (PhaseContext)
    │
    ▼
Phase 6 (SequentialFlowController refactor)
    │
    ▼
Phase 7 (FlowPhase)
    │
    ├─► Phase 8 (Tests FlowPhase)
    └─► Phase 9 (Build FlowPhase) ──► Phase 10 (Docs FlowPhase)


Phase 11 (FlowRulesGateway)        ← indipendente da 5-10, parallela
    │
    ▼
Phase 12 (ActionGateway)
    │
    ▼
Phase 13 (Integration Tests)
    │
    └─► Phase 14 (Build Cap2) ──► Phase 15 (Docs Cap2)
```

FlowPhase (Fasi 5–10) e gmRules Integration (Fasi 11–15) sono **indipendenti** e
possono procedere in parallelo. La Fase 9 (test `gmFlow_flow_phase` include il
test 9 che usa entrambi) è l'unico punto di convergenza opzionale.

---

## Criteri di Accettazione

### FlowPhase (Fasi 5–10)

| Criterio | Verifica |
|----------|----------|
| `GameContext` invariato | `git diff gmFlow/core/GameContext.*` mostra zero righe modificate |
| `GameSession` invariato | `git diff gmFlow/session/GameSession.*` mostra zero righe modificate |
| Tutti i test esistenti passano | `ctest -R gmFlow` — 5/5 suite passano (regressione zero) |
| 10 nuovi test passano | `ctest -R gmFlow_flow_phase` — 10/10 PASS |
| Nesting a 2 livelli funziona | Test 7 e 8 verificano Epoch → Day |
| `GameState` condiviso | Test 1 e 7 verificano che mutazioni siano visibili cross-livello |
| IDs locali isolati | Test 2, 3, 4, 8 verificano nessuna corruzione del GameContext radice |
| Build pulita (0 warning) | `cmake --build build --config Debug` — 0 errori, 0 warning |

### gmRules Integration (Fasi 11–15)

| Criterio | Verifica |
|----------|----------|
| Nessuna modifica a gmRules | `git diff gmRules/` mostra zero righe modificate |
| Nessuna modifica a IFlowController / IPhase | File invariati |
| Gateway callback invocati nell'ordine corretto | Test 7 verifica sequenza completa |
| Blocco pre-check deterministico | Test 3 + 8: azione bloccata → `RULE_VIOLATION` coerente |
| Post-hook sempre invocato | Test 5 + 6: sia su success che failure |
| 10 test integrazione passano | `ctest -R gmFlow_flow_rules` — 10/10 PASS |
| Regressione zero | `ctest -R gmFlow` — tutte le suite preesistenti passano |

---

## Riepilogo Impatto sui File Esistenti

### FlowPhase (Fasi 5–10)

| File | Tipo modifica | Dettaglio |
|------|---------------|-----------|
| `gmFlow/core/GameContext.hpp` | **Nessuna** | Usato come base class ma non toccato |
| `gmFlow/core/GameContext.cpp` | **Nessuna** | — |
| `gmFlow/flow/SequentialFlowController.hpp` | **Minima** | `_round_index` → protected, `advance_phase` → virtual, `current_phase()` aggiunto |
| `gmFlow/flow/SequentialFlowController.cpp` | **Minima** | Implementare `current_phase()` |
| `gmFlow/flow/PhaseContext.hpp` | **NUOVO** | Subclasse di GameContext con IDs locali |
| `gmFlow/flow/PhaseContext.cpp` | **NUOVO** | Implementazione |
| `gmFlow/flow/FlowPhase.hpp` | **NUOVO** | IPhase con controller + PhaseContext interni |
| `gmFlow/flow/FlowPhase.cpp` | **NUOVO** | Implementazione |
| `gmFlow/tests/test_flow_phase.cpp` | **NUOVO** | 10 test cases |
| `gmFlow/CMakeLists.txt` | **Minima** | +2 sorgenti +1 test target |
| `gmFlow/gmFlow_API.md` | **Additiva** | Nuove sezioni, nessuna cancellazione |
| Tutti gli altri file gmFlow esistenti | **Nessuna** | — |

### gmRules Integration (Fasi 11–15)

| File | Tipo modifica | Dettaglio |
|------|---------------|-----------|
| `gmFlow/bridges/FlowRulesGateway.hpp` | **NUOVO** | Payload, callback type, register function |
| `gmFlow/bridges/FlowRulesGateway.cpp` | **NUOVO** | Sottoscrizioni EventBus + dispatch callback |
| `gmFlow/bridges/ActionGateway.hpp` | **NUOVO** | IAction wrapper con pre/post hook |
| `gmFlow/bridges/ActionGateway.cpp` | **NUOVO** | Implementazione validate+execute con hooks |
| `gmFlow/tests/test_flow_rules_integration.cpp` | **NUOVO** | 10 test casi |
| `gmFlow/CMakeLists.txt` | **Minima** | +2 sorgenti bridges +1 test target |
| `gmFlow/gmFlow_API.md` | **Additiva** | Sezione bridges/ aggiunta |
| `gmRules/**` | **Nessuna** | gmRules non viene toccato |
| `gmDispatch/**` | **Nessuna** | — |

---

## Utilizzo Tipico (pseudocodice)

```cpp
// Creare le sub-fasi dell'Epoch
std::vector<std::unique_ptr<gmFlow::IPhase>> epoch_sub_phases;
epoch_sub_phases.push_back(std::make_unique<MorningPhase>());
epoch_sub_phases.push_back(std::make_unique<AfternoonPhase>());
epoch_sub_phases.push_back(std::make_unique<EveningPhase>());

// Creare il controller dell'Epoch (sub-controller)
auto epoch_ctrl = std::make_unique<gmFlow::SequentialFlowController>(
    std::move(epoch_sub_phases));

// Creare la FlowPhase Epoch (viene usata come IPhase normale dal parent)
auto epoch_phase = std::make_unique<gmFlow::FlowPhase>(
    "epoch_1",
    std::move(epoch_ctrl));

// Creare le fasi della sessione principale
std::vector<std::unique_ptr<gmFlow::IPhase>> session_phases;
session_phases.push_back(std::make_unique<SetupPhase>());
session_phases.push_back(std::move(epoch_phase));   // FlowPhase usata come IPhase
session_phases.push_back(std::make_unique<EndPhase>());

// Creare la sessione radice — invariata rispetto a V1
auto session_ctrl = std::make_unique<gmFlow::SequentialFlowController>(
    std::move(session_phases));

gmFlow::GameSession session(cfg, std::move(session_ctrl), std::move(state), dispatcher);
session.start();
while (!session.is_finished()) {
    session.submit_action("player_1", std::make_unique<MyAction>());
    session.tick();
}
```

---

*Creato: 2026-06-17*
*Dipende da: gmFlow V1 (Phase 1–4 complete)*
*Impatto su test esistenti: regressione zero*
