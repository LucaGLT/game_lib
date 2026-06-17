# gmFlow — FlowPhase Extension Plan

**Version:** 2.0
**Status:** Phase 5 — Planned ⏳
**Language:** C++17 Standard
**Namespace:** `gmFlow`
**Scope:** Extension of existing gmFlow infrastructure (additive, backward-compatible)

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

### Phase 5 — PhaseContext ⏳

**Goal:** Classe che estende `GameContext` con IDs locali isolati e scope prefix.

- [ ] Creare `gmFlow/flow/PhaseContext.hpp`
  - `class PhaseContext : public GameContext`
  - Costruttore `PhaseContext(GameContext& parent, std::string scope_prefix)`
    chiama `GameContext(parent.session_id(), parent.state(), parent.actor_registry(), parent.event_bus())`
  - Campo `_scope_prefix` (std::string, private)
  - Metodo `const std::string& scope_prefix() const`
  - Include guard: `#ifndef GMFLOW_PHASECONTEXT_HPP`
  - Doxygen su tutti i simboli pubblici
- [ ] Creare `gmFlow/flow/PhaseContext.cpp`
  - Implementazione costruttore e metodi
- [ ] Verificare che `PhaseContext` accetti i metodi `set_current_round_id()` etc.
  di `GameContext` senza override aggiuntivi
- [ ] Compilare senza errori (nessuna modifica al resto)

**Notes:**
`GameContext` non è copiabile ma il costruttore `GameContext(SessionId, GameState&,
ActorRegistry&, EventBus&)` è pubblico — `PhaseContext` lo chiama estraendo i valori
dal genitore. Non è una copia: è una nuova istanza che condivide i riferimenti.
`_current_phase_id`, `_current_round_id`, `_current_turn_id` ereditati da
`GameContext` sono distinti per ogni `PhaseContext` — il sub-controller li imposta
sulla `PhaseContext` senza toccare il `GameContext` radice.

---

### Phase 6 — SequentialFlowController Refactoring ⏳

**Goal:** Tre modifiche minori per abilitare subclassing e accesso alla fase corrente.

- [ ] In `SequentialFlowController.hpp`:
  - Spostare `_round_index` da `private` a `protected`
  - Rendere `advance_phase(GameContext& ctx)` `virtual` (era `private void`)
  - Aggiungere metodo pubblico `const IPhase* current_phase() const`
    (ritorna `_phases[_current_phase_index].get()`, o `nullptr` se fuori range)
- [ ] In `SequentialFlowController.cpp`:
  - Implementare `current_phase()`
- [ ] Aggiungere Doxygen al nuovo metodo e al campo promosso
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

### Phase 7 — FlowPhase Implementation ⏳

**Goal:** `IPhase` che possiede un controller interno e un `PhaseContext` isolato.

- [ ] Creare `gmFlow/flow/FlowPhase.hpp`

```cpp
class FlowPhase : public IPhase {
public:
    FlowPhase(std::string                          scope_prefix,
              std::unique_ptr<IFlowController>     controller,
              std::vector<std::unique_ptr<IPhase>> sub_phases);

    PhaseId id() const override;

    void on_enter(GameContext& ctx) override;
    void on_exit(GameContext& ctx) override;

    std::vector<std::unique_ptr<IAction>>
        available_actions(const GameContext& ctx,
                          const ActorId& actor) const override;

    bool is_complete(const GameContext& ctx) const override;

    void tick(GameContext& ctx);          // chiamato da on_enter o dal tick esterno

    const PhaseContext& phase_context() const;
    const std::string&  scope_prefix()  const;

private:
    std::string                      _scope_prefix;
    std::unique_ptr<IFlowController> _controller;
    PhaseContext                     _phase_ctx;   // costruito in on_enter
    bool                             _entered = false;
};
```

- [ ] Costruttore: riceve scope_prefix, controller, sub_phases (le passa al controller
  se è `SequentialFlowController`)
- [ ] `on_enter(GameContext& parent_ctx)`:
  - Costruisce `_phase_ctx` da `parent_ctx` e `_scope_prefix`
    (`_phase_ctx = PhaseContext(parent_ctx, _scope_prefix)`)
  - Chiama `_controller->start(_phase_ctx)`
  - Imposta `_entered = true`
- [ ] `on_exit(GameContext&)`: resetta `_entered = false`, cleanup opzionale
- [ ] `is_complete(const GameContext&)`:
  ritorna `_entered && _controller->is_session_complete(_phase_ctx)`
- [ ] `available_actions(const GameContext&, actor)`:
  - Ottiene la fase interna corrente via `SequentialFlowController::current_phase()`
    (cast dinamico se necessario)
  - Delega `current_phase->available_actions(_phase_ctx, actor)`
  - Ritorna vuoto se `!_entered` o fase corrente non disponibile
- [ ] `tick(GameContext&)`: chiama `_controller->process(_phase_ctx)` — necessario
  se il genitore non chiama `process` abbastanza frequentemente
- [ ] Include guard: `#ifndef GMFLOW_FLOWPHASE_HPP`
- [ ] Doxygen completo su tutti i simboli pubblici
- [ ] Creare `gmFlow/flow/FlowPhase.cpp` con tutte le implementazioni
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

### Phase 8 — Unit Tests ⏳

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

### Phase 9 — Build Integration ⏳

**Goal:** Aggiungere nuovi file al sistema di build CMake.

- [ ] In `gmFlow/CMakeLists.txt`:
  - Aggiungere `flow/PhaseContext.cpp` alla lista sorgenti della libreria
  - Aggiungere `flow/FlowPhase.cpp` alla lista sorgenti della libreria
  - Aggiungere target test `gmFlow_flow_phase` con `tests/test_flow_phase.cpp`
- [ ] Eseguire `cmake --build build --config Debug`
- [ ] Eseguire `ctest --test-dir build -R gmFlow` — tutti i test esistenti devono
  passare (regressione zero)
- [ ] Eseguire `ctest --test-dir build -R gmFlow_flow_phase` — nuovi 10 test passano

---

### Phase 10 — Documentation ⏳

**Goal:** Aggiornare la documentazione pubblica di gmFlow.

- [ ] Aggiornare `gmFlow/gmFlow_API.md`:
  - Aggiornare **Status** a `Phase 5 — FlowPhase implemented`
  - Aggiungere sezione `PhaseContext` nella parte `flow/ — Flow Control`
  - Aggiungere sezione `FlowPhase` nella parte `flow/ — Flow Control`
  - Aggiungere esempio d'uso in `Usage Examples`:
    "Epoch annidata con FlowPhase" (Session → Epoch → Morning/Evening)
  - Aggiornare diagramma Architecture con `FlowPhase` e `PhaseContext`
- [ ] Aggiungere `PhaseContextId` (alias `ScopeId`) in `gmFlow/core/Ids.hpp`
  se ritenuto utile (opzionale)

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
    ├─► Phase 8 (Tests)
    └─► Phase 9 (Build integration) ──► Phase 10 (Documentation)
```

Fasi 5 e 6 sono **indipendenti** e possono essere sviluppate in parallelo.
Fase 7 dipende da entrambe. Fasi 8, 9, 10 dipendono da Fase 7.

---

## Criteri di Accettazione

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

---

## Riepilogo Impatto sui File Esistenti

| File | Tipo modifica | Dettaglio |
|------|---------------|-----------|
| `gmFlow/core/GameContext.hpp` | **Nessuna** | Usato come base class ma non toccato |
| `gmFlow/core/GameContext.cpp` | **Nessuna** | — |
| `gmFlow/flow/SequentialFlowController.hpp` | **Minima** | `_round_index` → protected, `advance_phase` → virtual, `current_phase()` aggiunto |
| `gmFlow/flow/SequentialFlowController.cpp` | **Minima** | Implementare `current_phase()` |
| `gmFlow/CMakeLists.txt` | **Minima** | Aggiungere 2 sorgenti + 1 test target |
| `gmFlow/gmFlow_API.md` | **Additiva** | Nuove sezioni, nessuna cancellazione |
| Tutti gli altri file esistenti | **Nessuna** | — |

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
