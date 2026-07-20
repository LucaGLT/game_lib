# plan_gm_lib.md — Feature generiche da promuovere a libreria

**Data:** 2026-06-27
**Origine:** analisi de *Le Pergamene di Eldhôm* + revisione gmXxx esistenti
**Scopo:** Identificare le meccaniche di gioco pianificate per Eldhôm che sono
           indipendenti dal gioco specifico e devono essere implementate nei
           livelli gmAlea / gmFlow / gmActor / gmRules / gmGui prima
           (o in parallelo) allo sviluppo del gioco.

---

## Principio guida

Una meccanica va in libreria se:
1. **Appare in più di un tipo di gioco** (dungeon crawler, giochi di carte, wargame, RPG tattico, ecc.).
2. **Non contiene concetti di dominio del gioco** (non sa cos'è un Goblin, un PG o una Missione).
3. **Può essere parametrizzata** con dati o policy iniettati dall'esterno senza modificare il core.

---

## Stato attuale: già generico nelle gmXxx

Prima di pianificare il nuovo, è utile ricordare cosa è *già* generico e non va ritoccato:

| Meccanica | Dove | Note |
|---|---|---|
| Linea Temporale senza Round | `gmFlow::TimelineFlowController` | Seleziona chi è più indietro; tie-break per rank |
| Posizione tattica Prima Linea / Retroguardia | `gmActor::AreaPosition::FRONTLINE / BACKLINE` | Enum già definito in `core/Enums.hpp` |
| Condizione su posizione tattica | `gmRules::ConditionType::ACTOR_IN_POSITION` | Già implementato; `value = "FRONTLINE"` ecc. |
| Finestra di reazione fuori turno | `gmFlow::ActionWindow(CompletionPolicy::ANY_SUBMITTED)` | Chiude alla prima submission → exact match ⚡ |
| Azione multi-step (passo per passo) | `gmFlow::StepBasedAction` | Già supporta passi sequenziali con intermezzo |
| Stato attore (HP, statuses, tags, modifiers) | `gmActor::ActorStateCommon` | Generico, nessun dato di dominio |
| Comportamento Mostro: struttura gruppo | `gmActor::MonsterGroupState` | Ha `behavior_deck_id`, `active_behavior_card_id` |
| Boss come estensione del gruppo | `gmActor::BossState` | Ha `phase_index`, `rage`, `linked_objectives` |
| Effetti carte (danno, cura, stato, tag…) | `gmRules::EffectResolver` (46 tipi) | Completo inclusi i 5 nuovi tipi Dungeon Crawler |
| Deck management multi-zona | `gmAlea::GmCompDeck` | MainDeck, Hand, Memory, Discard, PlayArea, Banish |

---

## Feature generiche da implementare — Piano

---

### F1 — `gmAlea` : CardType + SequenceEngine

**Perché è generico:**
La meccanica di "sequenza di carte" (una carta può seguirne un'altra solo se il tipo lo permette)
è presente in decine di giochi: Marvel Champions, Arkham Horror LCG, Dominion combos, Hero Quest,
Chronicles of Drunagor, ecc. Non ha nulla di Eldhôm-specifico.

**Cosa aggiungere a `gmAlea`:**

```
gmAlea/
├── CardType.hpp          ← enum CardType
├── SequenceState.hpp     ← struct POD dello stato corrente della sequenza
└── SequenceEngine.hpp/.cpp ← state machine pura, nessun dato di gioco
```

**`CardType` (enum):**

```cpp
enum class CardType {
    SINGLE,         // Carta autonoma — termina il turno
    SEQ_START,      // Apre una Sequenza (può continuare)
    SEQ_CONTINUE,   // Valida solo dentro una Sequenza già aperta
    SEQ_END,        // Chiude la Sequenza obbligatoriamente
    INSTANT         // Fuori turno; ha trigger esterno
};
```

**`SequenceState` (struct POD):**

```cpp
struct SequenceState {
    bool     active         = false;
    CardType last_type      = CardType::SINGLE;
    int      cards_played   = 0;
    bool     interrupted    = false;
};
```

**`SequenceEngine` (interfaccia pubblica):**

```cpp
class SequenceEngine {
public:
    bool         can_play(CardType card, const SequenceState& state) const;
    SequenceState advance(CardType card, const SequenceState& state) const;
    bool         is_turn_ending(CardType card, const SequenceState& state) const;
    SequenceState interrupt(const SequenceState& state) const;
    SequenceState reset() const;
};
```

**Regole della macchina a stati:**

| Stato corrente | CardType giocato | Valido? | Nuovo stato |
|---|---|---|---|
| not active | SINGLE | ✅ | not active |
| not active | SEQ_START | ✅ | active, last=START |
| not active | SEQ_CONTINUE | ❌ | — |
| not active | SEQ_END | ❌ | — |
| not active | INSTANT | ✅ | invariato (fuori turno) |
| active | SINGLE | ❌ | — |
| active | SEQ_START | ❌ | — |
| active | SEQ_CONTINUE | ✅ | active, last=CONTINUE |
| active | SEQ_END | ✅ | not active (fine) |
| active | INSTANT | ✅ | invariato (fuori turno) |

**Dipendenze:** nessuna dipendenza da altre gmXxx. Solo STL.

---

### F2 — `gmFlow` : TimelineMilestoneSystem

**Perché è generico:**
Qualsiasi gioco con Linea Temporale continua (e non solo) può avere eventi
temporali: "al tempo 12 scatta la valvola", "al tempo 60 la missione fallisce",
"al tempo 24 arrivano i rinforzi". È la versione "senza Round" dei classici
trigger di fase. Appartiene a `gmFlow` perché dipende solo da `TimelineValue`.

**`ITimelineAdapter`** già notifica via `on_time_advanced()`: il sistema
di milestone è un decoratore/helper sopra quella notifica.

**Cosa aggiungere a `gmFlow`:**

```
gmFlow/flow/
└── TimelineMilestoneSystem.hpp/.cpp
```

**Interfaccia pubblica:**

```cpp
class TimelineMilestoneSystem {
public:
    using Callback = std::function<void(TimelineValue threshold, GameContext&)>;

    // Registra un callback da eseguire quando il tempo raggiunge o supera threshold.
    // one_shot = true → si rimuove automaticamente dopo il primo fuoco.
    void add_milestone(TimelineValue threshold, Callback cb, bool one_shot = true);

    // Rimuove tutti i milestone con quel threshold.
    void remove_milestone(TimelineValue threshold);

    // Chiama i callback scattati nell'intervallo (old_time, new_time].
    // Chiamato dall'adapter in on_time_advanced().
    void advance(TimelineValue old_time, TimelineValue new_time, GameContext& ctx);

    // Restituisce il threshold del prossimo milestone non ancora scattato.
    std::optional<TimelineValue> next_threshold() const;
};
```

**Integrazione:** il game adapter chiama `milestone_system_.advance(old, new, ctx)`
dentro `on_time_advanced()`. Zero modifica all'interfaccia `ITimelineAdapter`.

**Dipendenze:** solo `gmFlow/flow/TimelineTypes.hpp` e `gmFlow/core/GameContext.hpp`.

---

### F3 — `gmActor` : FormationValidator + FormationResolver

**Perché è generico:**
La regola `Retroguardia ≤ Prima Linea` è presente in quasi tutti i giochi
tattici con posizionamento front/back: Descent, Gloomhaven (formazione dei
nemici), Imperial Assault, Massive Darkness, ecc. L'invariante è identica;
solo i valori soglia cambiano per gioco.

La risoluzione forzata di una formazione illegale (chi si sposta per primo?)
con criteri prioritizzati è ugualmente generica: sempre si valuta una lista
ordinata di criteri fino al primo discriminante.

**Cosa aggiungere a `gmActor`:**

```
gmActor/formation/
├── FormationRules.hpp        ← policy configurabile (soglie, criteri)
├── FormationValidator.hpp/.cpp
└── FormationResolver.hpp/.cpp
```

**`FormationRules` (policy POD):**

```cpp
struct FormationRules {
    int max_backline_per_frontline = 1;    // RG ≤ PL × questo fattore
    int max_frontline = -1;                // -1 = illimitato
    int max_backline  = -1;                // -1 = illimitato
    bool backline_requires_frontline = true; // false = RG può esistere senza PL
};
```

**`FormationValidator`:**

```cpp
class FormationValidator {
public:
    explicit FormationValidator(FormationRules rules = {});

    bool is_valid(int frontline_count, int backline_count) const;
    int  backline_overflow(int frontline_count, int backline_count) const;
};
```

**`FormationResolver`:**
Risolve "chi si sposta da Retroguardia a Prima Linea" con criteri
iniettabili dall'esterno (zero accoppiamento con i dati di dominio).

```cpp
// Un criterio è una funzione che compara due attori.
// Ritorna true se a è "migliore candidato" di b (deve spostarsi per primo).
using FormationCriterion = std::function<bool(const ActorId& a,
                                             const ActorId& b,
                                             const ActorStore& store)>;

class FormationResolver {
public:
    explicit FormationResolver(std::vector<FormationCriterion> criteria);

    // Ordina i candidati dal più adatto al meno adatto (da spostare per primo).
    std::vector<ActorId> sort_candidates(const std::vector<ActorId>& backline_actors,
                                         const ActorStore& store) const;
};
```

**Criteri predefiniti** (da usare come helper standalone):

```cpp
// gmActor/formation/FormationCriteria.hpp
namespace FormationCriteria {
    FormationCriterion by_highest_hp();         // più HP va in Prima Linea
    FormationCriterion by_lowest_timeline();    // chi è più indietro sulla LT
    FormationCriterion random(unsigned seed);   // dado come ultima istanza
}
```

I criteri specifici del gioco (es. "chi ha più carte in mano") sono implementati
dal gioco e iniettati nel `FormationResolver` al momento della costruzione.

**Dipendenze:** `gmActor::ActorStore`, `gmActor::ActorStateCommon`. Nessuna dipendenza da gmRules.

---

### F4 — `gmActor` : BehaviorCardProcessor

**Perché è generico:**
Il pattern "per ogni passo della Carta Comportamento, per ogni Mostro del Gruppo:
tenta / salta; Gruppo.timeline += costo una volta sola" è **identico** in
Descent, Mansions of Madness, Gloomhaven, Return to Dark Tower, ecc.
La carta attiva viene usata fino a esaurimento o scartata da una Reazione.

Nota: `gmActor::MonsterGroupState` ha già `behavior_deck_id` e
`active_behavior_card_id` — la struttura dati è pronta.

**Cosa aggiungere a `gmActor`:**

```
gmActor/behavior/
├── BehaviorStep.hpp          ← step atomico di una carta comportamento
├── BehaviorCardProcessor.hpp/.cpp
└── BehaviorReactionSystem.hpp/.cpp
```

**`BehaviorStep`:**

```cpp
struct BehaviorStep {
    std::string effect_type;    // nome EffectType (es. "DEAL_DAMAGE")
    int         amount = 0;
    std::string value;
    int         timeline_cost = 1;  // ⌛ pagato dal Gruppo per questo step
    bool        optional = false;   // se true, skip senza fallimento
};
```

**`BehaviorCardProcessor` (interfaccia di callback):**
Non dipende da `gmRules` (no dipendenza circolare). Delega l'applicazione
degli effetti tramite callback iniettato.

```cpp
using StepExecutor = std::function<
    bool(const ActorId& group_id,
         const ActorId& target_id,
         const BehaviorStep& step)   // ritorna false se lo step non è risolvibile
>;

class BehaviorCardProcessor {
public:
    // Esegue tutti gli step della carta attiva del gruppo.
    // Chiama executor per ogni step, per ogni membro.
    // Aggiorna group.timeline_position dopo ogni step.
    void process_group_turn(MonsterGroupState&   group,
                            ActorStore&          store,
                            const StepExecutor&  executor) const;

    // Risolve il comportamento di fallback quando la carta non è eseguibile.
    void process_fallback(MonsterGroupState& group,
                          ActorStore&        store,
                          const StepExecutor& executor) const;
};
```

**`BehaviorReactionSystem`:**
Gestisce il ciclo reazione → scarta attiva → pesca nuova carta.

```cpp
class BehaviorReactionSystem {
public:
    // Verifica se la carta attiva del gruppo ha una reazione per questo trigger.
    bool has_reaction(const MonsterGroupState& group,
                      const std::string& trigger_event_type) const;

    // Risolve la reazione: esegue effetti, scarta carta attiva, pesca nuova.
    // Ritorna true se la reazione interrompe la sequenza corrente.
    bool fire_reaction(MonsterGroupState& group,
                       GmCompDeck&        behavior_deck,
                       const StepExecutor& executor) const;
};
```

**Dipendenze:** `gmActor`, `gmAlea::GmCompDeck`. NO dipendenze da `gmRules`.

---

### F5 — `gmGui` : 4 Widget generici PySide6

**Perché è generico:**
Questi widget visualizzano meccaniche già presenti nelle librerie C++ (Linea
Temporale, formazioni, sequenze, comportamento mostri). Sono riusabili da
qualsiasi GUI di gioco che usi le stesse librerie — non contengono testo,
icone o colori specifici di Eldhôm.

**Widget da aggiungere a `gmGui`:**

```
gmGui/
└── widgets/
    ├── timeline_widget.py          ← F5a
    ├── formation_widget.py         ← F5b
    ├── sequence_state_widget.py    ← F5c
    └── behavior_card_widget.py     ← F5d
```

#### F5a — `TimelineWidget`

Visualizza la Linea Temporale continua di un set di attori.

**Dati in ingresso** (via evento `gmflow.timeline.actors_updated`):
```json
{
  "actors": [
    { "id": "pg_1", "label": "Eran", "position": 5, "kind": "hero" },
    { "id": "goblin_a", "label": "Goblin A", "position": 7, "kind": "monster_group" }
  ],
  "milestones": [12, 24, 60],
  "active_id": "pg_1"
}
```

**Comportamento:**
- Tracciato orizzontale con segnalini attori ordinati per posizione
- Colore segnalino per `kind` (definito via QSS token, non hardcoded)
- Marcatori verticali per i milestone
- Il segnalino `active_id` è evidenziato

#### F5b — `FormationWidget`

Visualizza Prima Linea / Retroguardia per una locazione, per fazione.

**Dati in ingresso** (via evento `gmactor.formation.updated`):
```json
{
  "location_id": "stanza_1",
  "factions": [
    { "id": "heroes", "label": "PG", "frontline": 2, "backline": 1 },
    { "id": "monsters", "label": "Mostri", "frontline": 3, "backline": 0 }
  ],
  "max_frontline": -1,
  "max_backline": -1
}
```

**Comportamento:**
- Due colonne (Prima Linea / Retroguardia) per ogni fazione
- Badge numerico per count
- Bordo rosso se formazione illegale (`backline > frontline`)

#### F5c — `SequenceStateWidget`

Mostra lo stato corrente della sequenza attiva per un attore.

**Dati in ingresso** (via evento `gmalea.sequence.state_changed`):
```json
{
  "actor_id": "pg_1",
  "active": true,
  "last_type": "SEQ_START",
  "cards_played": 1,
  "valid_next": ["SEQ_CONTINUE", "SEQ_END", "INSTANT"]
}
```

**Comportamento:**
- Label compatta: "Nessuna sequenza" / "Sequenza aperta (carta 1)"
- Badge per ogni tipo carta valido come prossima mossa
- Grigio se non attivo, colorato se in sequenza

#### F5d — `BehaviorCardWidget`

Visualizza la Carta Comportamento attiva di un Gruppo Mostri.

**Dati in ingresso** (via evento `gmactor.behavior.card_changed`):
```json
{
  "group_id": "goblin_a",
  "card_id": "bc_goblin_charge",
  "card_label": "Carica",
  "steps": [
    { "index": 0, "label": "Muovi 2", "cost": 1, "state": "done" },
    { "index": 1, "label": "Attacca 2❌", "cost": 2, "state": "active" }
  ],
  "has_reaction": true,
  "reaction_trigger": "gmflow.hero.played_card"
}
```

**Comportamento:**
- Lista di step con indicatore visivo (fatto / attivo / in attesa)
- Badge ⚡ se ha una Reazione disponibile con il trigger leggibile
- Si svuota quando la carta viene scartata per reazione

---

## Riepilogo priorità di sviluppo

| # | Feature | Lib | Dipendenze | Priorità |
|---|---|---|---|---|
| F1 | CardType + SequenceEngine | `gmAlea` | nessuna | **Alta** — bloccante per Eldhôm Phase 2 |
| F2 | TimelineMilestoneSystem | `gmFlow` | gmFlow esistente | **Alta** — bloccante per MissionEventSystem |
| F3 | FormationValidator + FormationResolver + Criteri | `gmActor` | ActorStore | **Alta** — bloccante per Eldhôm Phase 3 |
| F4 | BehaviorCardProcessor + BehaviorReactionSystem | `gmActor` | gmAlea::GmCompDeck | **Media** — bloccante per Eldhôm Phase 4 |
| F5a | TimelineWidget | `gmGui` | gmFlow events | **Media** — può svilupparsi col mock |
| F5b | FormationWidget | `gmGui` | gmActor events | **Media** — può svilupparsi col mock |
| F5c | SequenceStateWidget | `gmGui` | gmAlea events | **Media** — può svilupparsi col mock |
| F5d | BehaviorCardWidget | `gmGui` | gmActor events | **Bassa** — ultima GUI necessaria |

---

## Impatto sul PLAN.md di Eldhôm

Con queste feature in libreria, il PLAN di Eldhôm si semplifica:

| Fase Eldhôm (originale) | Diventa |
|---|---|
| Phase 2 — CardType + SequenceEngine | Usa `gmAlea::SequenceEngine` — solo adattatore |
| Phase 3 — FormationEngine | Usa `gmActor::FormationValidator + FormationResolver` — aggiunge solo criteri Eldhôm-specifici |
| Phase 4 — BehaviorCardResolver | Usa `gmActor::BehaviorCardProcessor` — aggiunge solo StepExecutor callback |
| Phase 5 — MissionEventSystem | Usa `gmFlow::TimelineMilestoneSystem` — solo registrazione callback |
| Phase 8 — 4 widget generici GUI | Usa widget da `gmGui` già pronti — zero implementazione |

Il codice Eldhôm-specifico scende da ~5 componenti robusti a ~5 thin adapter layer.

---

## Sequenza di sviluppo consigliata

```
gmAlea::CardType + SequenceEngine  (F1)
    ↓
gmFlow::TimelineMilestoneSystem    (F2)   ← parallelo a F1
    ↓
gmActor::FormationValidator+Resolver (F3)
    ↓
gmActor::BehaviorCardProcessor       (F4)
    ↓
gmGui widgets F5a-F5d                (sviluppabili col mock Eldhôm in parallelo)
    ↓
Eldhôm: Phase 1 JSON + Phase 2 adapter
    ↓
Eldhôm: Phase 3-6 (motore di gioco)
```

F1 e F2 possono procedere in parallelo essendo completamente indipendenti.
F3 dipende solo da gmActor (già stabile).
F4 può iniziare non appena F3 è completo.
