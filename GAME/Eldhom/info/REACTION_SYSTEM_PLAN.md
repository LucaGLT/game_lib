# Eldhôm — Reaction & Priority System — Development Plan

**Version:** 0.1.0
**Status:** Phase 1 — Engine infrastructure 🔧
**Engine:** C++17 CoreEngine (`eldhom`) + Python 3 / PySide6 GUI
**Owner doc:** complements `GAME/Eldhom/info/PLAN.md`

---

## Goal

Rendere **interattiva e centralizzata nel CoreEngine** la risoluzione di ogni Azione e
Carta che produce un evento per cui esistono reazioni possibili. Il motore è l'unica
fonte di verità: la GUI raccoglie solo le scelte dell'utente e attende che il CoreEngine
decida e applichi gli effetti. Il sistema introduce due finestre di reazione con priorità
fissa — la **finestra Istantanee** (carte INSTANT giocabili da qualunque Actor il cui
`reaction_trigger` combacia con l'evento in corso) e la **finestra Difesa** del bersaglio
(`TAKE` / `BLOCK` / `DODGE`) — risolte nell'ordine *Istantanee → Difesa → Effetti*.

---

## Modello concordato

- **Un solo User / un solo Client** controlla tutti gli Actor (no rete, no hot-seat fisico).
- Quando un'Azione/Carta è in corso, il CoreEngine verifica se esiste **almeno una** Carta
  INSTANT (in qualunque mano) con `reaction_trigger` uguale all'evento generato dall'azione.
- Se sì, la GUI mostra un dialog con **lista di check-box**: `<Actor_X>` può giocare
  `<Carta_Istant_Y>`. L'User spunta quali giocare (tutte, alcune, nessuna).
- Le Istantanee selezionate vengono risolte dal CoreEngine **prima** della finestra Difesa.
- La finestra si apre **solo** se esiste almeno una istantanea idonea (altrimenti si procede
  direttamente alla difesa / agli effetti).

### Mappatura trigger → evento

| Azione in corso                         | Evento trigger              |
|-----------------------------------------|-----------------------------|
| Eroe attacca un Mostro (attacco/carta danno) | `eldhom.monster.damaged` |
| Mostro attacca un PG                    | `eldhom.pg.attacked`        |

---

## Architecture

```text
GUI (PySide6)                         CoreEngine (C++)
─────────────                         ────────────────
declare_attack / play_card / action ─►  EldhomEngine
                                        │  detect_eligible_instants(trigger)
        ◄── instant.window_opened ──────┤  (PendingResolution: AWAITING_INSTANTS)
play_instants [{actor,card}, …] ───────►│  resolve_instants(selected)
        ◄── instant.window_closed ──────┤
                                        │  if attack → (AWAITING_DEFENSE)
        ◄── reaction.window_opened ─────┤
react_defense {reaction} ──────────────►│  resolve_reaction → apply effects
        ◄── reaction.window_closed ─────┤
        ◄── attack.resolved ────────────┘
```

---

## File Structure (touched)

```text
GAME/Eldhom/CoreEngine/
  engine/EldhomTypes.hpp     ← + EVT_INSTANT_WINDOW_*, CMD_PLAY_INSTANTS, error codes
  engine/EldhomEngine.hpp    ← + PendingResolution, InstantOption, API
  engine/EldhomEngine.cpp    ← + detect/resolve instants, stage machine
  main.cpp                   ← + handle_play_instants, instant-window guard/events
GAME/Eldhom/GUI/
  widgets/instant_window_dialog.py  ← NEW: check-box dialog (engine-driven)
  app/eldhom_main_window.py         ← + wiring + router for instant events
```

---

## Development Phases

### Phase 1 — Engine instant infrastructure 🔧

- [ ] `EldhomTypes.hpp`: events `EVT_INSTANT_WINDOW_OPEN`, `EVT_INSTANT_WINDOW_CLOSED`;
      command `CMD_PLAY_INSTANTS`; error codes `ERR_NO_PENDING_INSTANTS`.
- [ ] `EldhomEngine`: `struct InstantOption { actor_id; card_id; card_name; trigger; }`,
      `std::vector<InstantOption> eligible_instants(const std::string& trigger) const`.
- [ ] `EldhomEngine`: `ActionResult resolve_instants(const std::vector<std::pair<HeroId,CardId>>&)`
      (valida idoneità, applica effetti istantanea, avanza timeline, sposta carta in scarti,
      emette eventi).

**Notes:** Scansiona `_hand_states` di tutti gli eroi; un'istantanea è idonea se in mano,
`card_type == INSTANT` e `reaction_trigger == trigger`. Riusa `_rule_adapter.apply_effect`.

### Phase 2 — Attack flow integration ⏳

- [ ] `declare_attack` apre la finestra Istantanee (trigger `eldhom.monster.damaged`)
      se idonee; altrimenti procede direttamente alla finestra Difesa.
- [ ] Stadi della `PendingResolution`: `AWAITING_INSTANTS` → `AWAITING_DEFENSE` → resolved.
- [ ] `main.cpp`: `handle_play_instants` emette `instant.window_closed` poi apre la
      finestra Difesa; guard estesa per accettare solo `CMD_PLAY_INSTANTS`/`CMD_REACT_DEFENSE`.

**Notes:** L'ordine è Istantanee → Difesa → Effetti. Le istantanee possono modificare lo
stato (es. `FORMATION_PUSH`) prima del calcolo del danno.

### Phase 3 — Cards & simple actions ⏳

- [ ] `play_card` e `do_simple_action`: prima di applicare effetti di DANNO, determinano il
      trigger e (se idonee) aprono la finestra Istantanee, poi gli effetti.
- [ ] Carte/azioni senza danno: finestra Istantanee solo se esiste un trigger idoneo.

**Notes:** Richiede sospensione/ripresa della risoluzione della carta (PendingResolution
porta gli effetti residui). Da affinare dopo la validazione di Phase 2.

### Phase 4 — GUI ⏳

- [ ] `instant_window_dialog.py`: dialog modale con una check-box per opzione
      (`<Actor> — <Carta>`), pulsanti *Gioca selezionate* / *Nessuna*.
- [ ] `eldhom_main_window.py`: router per `instant.window_opened/closed`; invio
      `eldhom.play_instants` con la lista selezionata.

**Notes:** Il dialog è puramente di raccolta input; nessuna logica di gioco lato GUI.

---

## Key Design Decisions

1. **Engine-authoritative.** La GUI non applica mai effetti: invia selezioni e attende eventi.
2. **Finestra solo se utile.** Nessuna pausa quando non esistono istantanee idonee.
3. **Priorità fissa.** Istantanee prima della Difesa, Difesa prima degli Effetti finali.
4. **Single-client.** Una sola lista di check-box aggrega le istantanee di tutti gli Actor.
