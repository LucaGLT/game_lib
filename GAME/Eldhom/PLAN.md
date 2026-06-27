# Le Pergamene di Eldhôm — Development Plan

**Version:** 0.1.0
**Status:** Phase 0 – Analysis & Architecture ⏳
**Engine:** C++17 (gmXxx libs) + Python 3 / PySide6 (GUI)
**Game Type:** Advanced Dungeon Crawler — Card-based with Deckbuilding

---

## Goal

Le Pergamene di Eldhôm è un dungeon crawler avanzato basato su carte con meccaniche
di deckbuilding, Linea Temporale continua (nessun Round), formazioni tattiche
Prima Linea / Retroguardia e Gruppi Mostri con Carte Comportamento autonome.
Il gioco è costruito sopra le librerie gmXxx esistenti: riusa l'80% della
logica già sviluppata e aggiunge solo la logica di dominio specifica.
Il design privilegia la separazione tra motore C++ (regole, stato, flusso)
e GUI Python/PySide6 (visualizzazione, interazione giocatore).

---

## Architettura

```
┌─────────────────────────────────────────────────────────────────────┐
│  PySide6 GUI  (GAME/Eldhom/GUI/)                                    │
│  TimelineWidget  FormationWidget  SequenceWidget  BehaviorCardWidget │
│  MultiHeroPanelWidget  MissionEventWidget  (GmCompDeckModule reused) │
└────────────────────────────┬────────────────────────────────────────┘
                             │ ZMQ events / commands  (gmDispatch reused)
┌────────────────────────────▼────────────────────────────────────────┐
│  EldhomEngine  (GAME/Eldhom/CoreEngine/)                            │
│  ┌───────────────┐  ┌────────────────┐  ┌──────────────────────┐   │
│  │SequenceEngine │  │FormationEngine │  │MissionEventSystem    │   │
│  │(NEW)          │  │(NEW)           │  │(NEW)                 │   │
│  └──────┬────────┘  └───────┬────────┘  └──────────┬───────────┘   │
│         │                   │                       │               │
│  ┌──────▼───────────────────▼───────────────────────▼───────────┐  │
│  │  EldhomRuleAdapter  (bridges gmRules ↔ game state)           │  │
│  └──────────────────────────────────────────────────────────────┘  │
└───────┬───────────────────────────────────────────────────────────-─┘
        │
┌───────▼────────────────────────────────────────────────────────────┐
│  gmXxx Libraries  (NO MODIFICATION except minor extensions)         │
│                                                                     │
│  gmFlow::TimelineFlowController  ←  Linea Temporale                 │
│  gmActor::HeroState/MonsterGroupState/BossState  ←  tutti gli attori│
│  gmActor::AreaPosition::FRONTLINE/BACKLINE  ←  Prima/Retroguardia   │
│  gmAlea::GmCompDeck  ←  Mazzo Totale/Missione/Mano/Memoria/Scarti   │
│  gmRules::RuleBook + EffectResolver  ←  effetti carte e azioni      │
│  gmMap  ←  Locazioni e connessioni                                  │
│  gmDispatch  ←  event bus                                           │
│  gmLog  ←  logging                                                  │
└────────────────────────────────────────────────────────────────────┘
```

---

## File Structure

```
GAME/Eldhom/
├── PLAN.md                          ← questo file
├── info/
│   └── regole_base_eldhom_ai_spec.md  ← regolamento sorgente
├── data/
│   ├── cards_base.json              ← carte azione base PG
│   ├── cards_etnia_thael.json       ← 3 carte Etnia Thael
│   ├── cards_etnia_velyr.json       ← 3 carte Etnia Velyr
│   ├── cards_etnia_khar.json        ← 3 carte Etnia Khar
│   ├── cards_etnia_erranti.json     ← 3 carte Etnia Erranti
│   ├── cards_affiliazione_leone.json← 6 carte Compagnia del Leone
│   ├── rules_base.json              ← regole effetti azioni semplici
│   ├── rules_cards.json             ← regole risoluzione carte
│   └── missions/
│       └── mission_01.json          ← prima missione di test
├── CoreEngine/
│   ├── CMakeLists.txt
│   ├── engine/
│   │   ├── EldhomEngine.hpp/.cpp    ← orchestratore principale
│   │   └── EldhomTypes.hpp          ← command_id / event_id costanti
│   ├── sequence/
│   │   ├── SequenceEngine.hpp/.cpp  ← state machine Inizio/Continuo/Fine/Istantanea
│   │   └── CardType.hpp             ← enum Singola/InizioSeq/ContinuoSeq/FineSeq/Istantanea
│   ├── formation/
│   │   ├── FormationEngine.hpp/.cpp ← valida RG ≤ PL, Schieramento vs Scompaginamento
│   │   └── ScompaginamentoResolver.hpp/.cpp  ← algoritmo priorità §18
│   ├── monsters/
│   │   ├── BehaviorCardResolver.hpp/.cpp     ← step loop per gruppo
│   │   └── MonsterReactionSystem.hpp/.cpp    ← trigger ⚡ + discard/draw behavior card
│   ├── mission/
│   │   ├── MissionEventSystem.hpp/.cpp       ← soglie timeline + trigger locazione
│   │   └── MissionDefinition.hpp             ← struct missione caricata da JSON
│   ├── targeting/
│   │   └── TargetingFilter.hpp/.cpp          ← Prima Linea scherma Retroguardia
│   └── rules/
│       └── EldhomRuleAdapter.hpp/.cpp        ← bridge gmRules ↔ EldhomEngine
├── GUI/
│   ├── main.py
│   ├── app/
│   │   ├── eldhom_main_window.py    ← finestra principale
│   │   ├── eldhom_bridge.py         ← ZMQ bridge (fork dungeon_bridge.py)
│   │   └── event_router.py          ← riusato da Dungeon Crawler
│   └── widgets/
│       ├── timeline_widget.py            ← Linea Temporale visiva
│       ├── location_formation_widget.py  ← Prima Linea / Retroguardia per locazione
│       ├── behavior_card_widget.py       ← Carta Comportamento attiva gruppo
│       ├── sequence_widget.py            ← indicatore stato sequenza
│       ├── multi_hero_panel_widget.py    ← pannello 2-5 PG
│       ├── mission_event_widget.py       ← soglie eventi missione
│       └── [GmCompDeckModule riusato]
└── mock_eldhom_engine.py            ← motore mock Python per sviluppo GUI
```

---

## Development Phases

### Phase 0 — Analisi e Mapping ✅

- [x] Analisi regolamento Eldhôm (44 sezioni)
- [x] Mapping funzionalità → librerie gmXxx esistenti
- [x] Identificazione componenti da costruire ex novo
- [x] Stesura PLAN.md

**Notes:** Riuso stimato: ~75% della logica coperta dalle gmXxx libs.
Le aree principali da costruire sono SequenceEngine, FormationEngine,
BehaviorCardResolver e i 6 widget GUI.

---

### Phase 1 — Data Layer (JSON card definitions)

- [ ] Definire schema JSON esteso per carte Eldhôm:
  - campo `card_type` (Singola / InizioSeq / ContinuoSeq / FineSeq / Istantanea)
  - campo `card_origin` (Base / Etnia / Affiliazione / Ruolo / Arma / Armatura / Artefatto)
  - campo `icons` (array: ▶ / ⏸ / ⏺ / ⚡)
  - campo `trigger` (per Istantanee: stringa evento che le attiva)
  - campo `sequence_continues` (bool, per Inizio/Continuo Seq)
- [ ] `cards_base.json` — Azioni Semplici come carte (Movimento, Attacco, Interazione, Recupero)
- [ ] `cards_etnia_thael.json` — Mani da Cava, Attrezzo Giusto, Schiena Spezzata
- [ ] `cards_etnia_velyr.json` — Parola Giusta, Loto Bianco, Maschera Gentile
- [ ] `cards_etnia_khar.json` — Respiro dell'Oasi, Acqua Nascosta, Memoria del Sale
- [ ] `cards_etnia_erranti.json` — Strada di Traverso, Debito di Carovana, Segno sul Sentiero
- [ ] `cards_affiliazione_leone.json` — Formazione!, Serrate la Formazione!, Supporto, Pianta di Ferro, Spalla alla Linea, Colpo che Apre
- [ ] `rules_base.json` — regole per ogni effetto dei tipi Azione Semplice
- [ ] `rules_cards.json` — regole per ogni carta (incluse condizioni su area_position, affiliazione)

**Notes:** Il formato JSON riusa la struttura di `dungeon_rules.json` già funzionante.
Il nuovo campo `card_type` è puro metadata JSON; la logica di sequenza è nel
`SequenceEngine` C++, non in gmRules.

---

### Phase 2 — CardType + SequenceEngine (C++)

- [ ] `CardType.hpp` — enum `CardType { SINGLE, SEQ_START, SEQ_CONTINUE, SEQ_END, INSTANT }`
- [ ] `CardOrigin.hpp` — enum `CardOrigin { BASE, ETHNICITY, AFFILIATION, ROLE, WEAPON, ARMOR, TRINKET, ARTIFACT, AEON_ALCHEMIC }`
- [ ] `CardMetadata.hpp` — struct con `type`, `origin`, `icons`, `trigger`, `sequence_continues`
- [ ] `SequenceEngine.hpp/.cpp`:
  - `bool can_play(const CardMetadata& card, const SequenceState& state) const`
  - `SequenceState advance(const CardMetadata& card, const SequenceState& state) const`
  - `bool is_turn_ending(const CardMetadata& card) const`
  - `void interrupt(SequenceState& state)` — interruzione da effetto esterno
- [ ] Test unitari `SequenceEngine`

**Notes:** `SequenceState` è una struct POD: `{ bool active; CardType last_played; int cards_played_count; }`.
La macchina a stati è deterministica: nessun input asincrono. Usata dall'`EldhomEngine`
che chiama `advance()` per ogni carta giocata dal PG.

---

### Phase 3 — FormationEngine + ScompaginamentoResolver (C++)

- [ ] `FormationEngine.hpp/.cpp`:
  - `bool is_valid(const FormationSnapshot& snap) const` — verifica RG ≤ PL per fazione
  - `FormationChangeType classify(const FormationChange& change) const` — SCHIERAMENTO o SCOMPAGINAMENTO
  - `void apply_formation_change(ActorStore& store, const FormationChange& change)`
- [ ] `ScompaginamentoResolver.hpp/.cpp`:
  - `std::vector<ActorId> resolve_priority(const std::vector<ActorId>& candidates, const ActorStore& store) const`
  - Criteri §18 in ordine: max HP → max cards_in_hand → min timeline_position → dado
- [ ] `TargetingFilter.hpp/.cpp`:
  - `bool can_melee_target(const ActorStateCommon& attacker, const ActorStateCommon& target, const ActorStore& store) const`
  - `bool can_ranged_target(..., int distance) const`
- [ ] Test unitari `FormationEngine` e `TargetingFilter`
- [ ] Estensione minore a `gmRules::ConditionSpec`: aggiungere `ACTOR_IN_POSITION` check

**Notes:** `FormationSnapshot = { location_id, faction_id, int frontline_count, int backline_count }`.
Il resolver non conosce GUI — emette eventi `eldhom.formation.changed` via gmDispatch.
`ScompaginamentoResolver` dipende da `ActorStore` ma non da `gmRules` — nessuna
dipendenza circolare.

---

### Phase 4 — BehaviorCardResolver + MonsterReactionSystem (C++)

- [ ] `BehaviorCardResolver.hpp/.cpp`:
  - `void resolve_group_turn(MonsterGroupState& group, ActorStore& store, RuleContext& ctx)` — loop §22.3
  - Ogni step: per ogni membro → tenta risoluzione → skip se impossibile → `group.timeline += step.cost`
  - Se carta non risolvibile: `resolve_fallback(group, store, ctx)` — comportamento base §23
- [ ] `MonsterReactionSystem.hpp/.cpp`:
  - `bool check_reaction(const MonsterGroupState& group, const RuleEvent& trigger_event) const`
  - `void apply_reaction(MonsterGroupState& group, GmCompDeck& behavior_deck, ...)` — risolvi → scarta → pesca nuova
- [ ] Comportamento base mostri §23 come fallback JSON (`rules_monster_base.json`)
- [ ] Test unitari con mock ActorStore e RuleContext

**Notes:** `BehaviorCardResolver` usa `gmRules::EffectResolver` per applicare effetti.
I passi della Carta Comportamento sono `std::vector<EffectSpec>` in `RuleDefinition`.
La carta viene completamente eliminata nel sistema delle Reazioni (scarta tutto) e
rimpiazzata con `GmCompDeck::draw_to_play()`.

---

### Phase 5 — MissionEventSystem (C++)

- [ ] `MissionDefinition.hpp` — struct missione caricata da JSON:
  - `victory_conditions`, `defeat_conditions`
  - `timeline_events: vector<{threshold, event_type, payload}>`
  - `location_events: vector<{location_id, trigger, event_type}>`
- [ ] `MissionEventSystem.hpp/.cpp`:
  - `void on_timeline_advance(int64_t new_time)` — controlla soglie
  - `void on_location_entered(const ActorId&, const AreaId&)` — controlla trigger locazione
  - `void on_actor_defeated(const ActorId&)` — trigger per morte mostro/boss
  - Emette eventi `eldhom.mission.*` via gmDispatch
- [ ] Loader JSON missione (riusa pattern `RuleBookLoader`)
- [ ] `mission_01.json` — missione base di test: 1 locazione, 1 gruppo goblin, 1 obiettivo semplice

**Notes:** Il sistema di eventi missione è stateless: riceve notifiche e emette eventi
senza modificare direttamente lo stato degli attori. Il motore principale
(`EldhomEngine`) reagisce agli eventi `eldhom.mission.*` per resolvere vittoria/sconfitta.

---

### Phase 6 — EldhomEngine + EldhomRuleAdapter (C++)

- [ ] `EldhomTypes.hpp` — costanti `command_id` / `event_id` Eldhôm
- [ ] `EldhomRuleAdapter.hpp/.cpp` — implementa `RuleContext` per Eldhôm:
  - Delega a `ActorStore` per HP, risorse, posizioni
  - Delega a `gmMap` per location queries
  - Delega a `GmCompDeck` per operazioni mazzo
  - Gestisce `apply_extended_effect()` per TRIGGER_RULE, DELAY_EFFECT
- [ ] `EldhomEngine.hpp/.cpp` — orchestratore principale:
  - Riceve comandi via ZMQ (o direct call)
  - Chiama `TimelineFlowController::tick()` ogni turno
  - Gestisce turno PG: `SequenceEngine` + `FormationEngine` + `EldhomRuleAdapter`
  - Gestisce turno Gruppo Mostri: `BehaviorCardResolver` + `MonsterReactionSystem`
  - Chiama `MissionEventSystem` dopo ogni avanzamento timeline
- [ ] CMakeLists.txt per CoreEngine

**Notes:** `EldhomEngine` implementa `ITimelineAdapter` per gmFlow.
Il tie-break order §2.2 (PG > PNG > Gruppi > Boss) è hardcoded nell'adapter
come `tie_break_rank`: PG=1, PNG_ALLY=2, MONSTER_GROUP=3, BOSS=4.

---

### Phase 7 — mock_eldhom_engine.py (sviluppo GUI)

- [ ] Fork di `mock_dungeon_engine.py` con:
  - 2 PG (hero_1: Thael guerriero, hero_2: Velyr supporto)
  - 2 Gruppi Mostri (goblin_group_a, goblin_group_b) con carte comportamento
  - Linea Temporale con segnalini (posizione numerica aggiornata)
  - Formazione per ogni locazione (conteggio PL/RG per fazione)
  - Sequenza attiva (campo `active_sequence_type`)
- [ ] Emette tutti gli eventi `eldhom.*` necessari alla GUI
- [ ] Gestisce comandi `eldhom.play_card`, `eldhom.use_simple_action`
- [ ] Porta EVENT_PORT=9300, COMMAND_PORT=9301 (evita conflitti)

**Notes:** Il mock serve esclusivamente per sviluppare e testare la GUI in isolamento
dal CoreEngine C++. Non implementa tutte le regole — solo abbastanza per
alimentare ogni widget con dati realistici.

---

### Phase 8 — GUI Widgets (PySide6)

- [ ] `timeline_widget.py` — tracciato orizzontale con segnalini attori:
  - Ordine da sinistra (più indietro) a destra
  - Colore per tipo attore (PG, PNG, Gruppo, Boss)
  - Soglie eventi missione come marcatori
- [ ] `location_formation_widget.py` — per ogni locazione visibile:
  - Sezione Prima Linea (barra) / Retroguardia (barra)
  - Per fazione PG e per fazione Mostri separatamente
  - Indicazione visiva se formazione illegale
- [ ] `behavior_card_widget.py` — per ogni gruppo mostro:
  - Nome carta comportamento attiva
  - Step corrente (evidenziato)
  - Indicatore se ha Reazione ⚡
- [ ] `sequence_widget.py` — overlay/panel:
  - "Nessuna sequenza" / "Inizio giocato" / "In sequenza (Continuo o Fine)"
  - Carte compatibili evidenziate nella mano
- [ ] `multi_hero_panel_widget.py` — dock laterale:
  - Scheda PG compatta per ogni hero (HP, timeline pos, mano count, stato)
  - Selezionabile per mostrare dettaglio nella GmCompDeckModule esistente
- [ ] `mission_event_widget.py` — barra superiore:
  - Tempo missione corrente
  - Prossima soglia evento (countdown)
  - Stato obiettivi (completati / attivi / falliti)
- [ ] `eldhom_main_window.py` — integra tutti i widget

**Notes:** I widget non hanno logica di gioco. Reagiscono tutti a eventi `eldhom.*`
via `event_router.py` riusato invariato. `GmCompDeckModule` è riusato senza
modifiche per visualizzare mano e scarti.

---

### Phase 9 — Estensioni minori gmXxx libs

- [ ] `gmActor::HeroState` — aggiungere `EthnicityId ethnicity_id`
- [ ] `gmRules::ConditionSpec` — aggiungere `ACTOR_IN_POSITION` (controlla FRONTLINE/BACKLINE)
- [ ] `gmRules::RuleBookLoader` — aggiungere `SHIFT_POSITION` nella parse table (già nell'enum, mancante nel loader)
- [ ] `gmMap` — verificare/aggiungere `path_length(from, to)` per distanza N locazioni
- [ ] Aggiornare API markdown dove modificato

**Notes:** Tutte le estensioni sono additive e backward-compatible.
Nessuna modifica a interfacce pubbliche esistenti.
Ogni modifica segue le regole cpp-style.instructions.md e
documentation.instructions.md.

---

### Phase 10 — Integration Test + Prima Missione

- [ ] `mission_01.json` — mappa con 3 locazioni, 2 gruppi goblin, 1 boss
- [ ] Test integrazione: PG con Etnia Thael + Velyr, mazzo missione 10 carte
- [ ] Sequenza completa: turno PG → Sequenza 2 carte → turno Gruppo Mostri con Reazione
- [ ] Verifica Linea Temporale: costi ⌛ applicati correttamente
- [ ] Verifica FormationEngine: Scompaginamento da Colpo che Apre risolto correttamente
- [ ] GUI: tutti i widget aggiornati in real-time

---

## Key Design Decisions

1. **Nessun Round** — `gmFlow::TimelineFlowController` implementa già il modello
   esatto del regolamento. Il costo ⌛ di ogni azione/carta si traduce in
   `timeline_position += cost`.

2. **GmCompDeck come Mazzo Missione** — ogni PG ha una istanza GmCompDeck
   per il mazzo missione (MainDeck=Mazzo, Hand=Mano, Memory=Memoria, Discard=Scarti).
   Il "Mazzo Totale" è una struttura dati pre-missione (selezione), non un secondo deck attivo.

3. **SequenceEngine è stateless** — lo stato della sequenza corrente è una struct POD
   passata per valore. Non c'è stato globale mutabile nella macchina a stati.

4. **Scompaginamento ≠ Schieramento** — la distinzione è contestuale (§16-§17):
   `FormationEngine::classify()` determina il tipo dalla sorgente del cambiamento,
   non dall'effetto risultante.

5. **MonsterReactionSystem scarta l'intera carta comportamento** — in accordo con §25.2:
   dopo una Reazione, la carta attiva è consumata e viene pescata una nuova carta.
   Nessun segnalino "Reazione usata" necessario.

6. **TargetingFilter come query pura** — non muta stato. Ogni attacco (in gmRules o
   in EldhomEngine) chiama `TargetingFilter::can_melee_target()` prima di applicare
   il danno. Se il target è protetto dalla Prima Linea, il danno non viene applicato.

7. **Etnia è campo dedicato su HeroState** — non è un'Affiliazione generica perché
   il regolamento la distingue esplicitamente da Ruolo e Affiliazione (§29.1).

8. **Carta Istantanea e Trigger** — il `trigger` della carta è una stringa evento
   (`"eldhom.actor.attacked"`, `"eldhom.formation.disrupted"`, ecc.).
   `MonsterReactionSystem` e `EldhomEngine` confrontano l'evento corrente con
   `card_metadata.trigger` di ogni carta in mano dei PG.
