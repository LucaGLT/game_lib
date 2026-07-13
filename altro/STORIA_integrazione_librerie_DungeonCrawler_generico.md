# Storia delle decisioni — Integrazione librerie, Dungeon Crawler generico, gmFlow, gmRules

**Scopo:** memoria storica per i thread futuri. Copre le decisioni prese nel
periodo 2026-06-16 → 2026-06-28. Questo file può sostituire il thread di
conversazione originale.

**Fonti verificate:** artefatti reali nel workspace (`plan_gm_lib.md`,
`GAME/Eldhom/info/PLAN.md`, sorgenti gmXxx, widget Python).

---

## 1. Principio architetturale cardine

> **Il CoreEngine comanda SEMPRE. La GUI SOLO visualizza.**
> La GUI non deve mai prendere decisioni di logica di gioco.

- Architettura **due processi** comunicanti via TCP con protocollo
  lunghezza-prefisso + JSON UTF-8.
- CoreEngine = source of truth per stato, regole, flusso.
- GUI = thin client: riceve eventi serializzati, visualizza, invia comandi
  dell'utente. Non ricostruisce logica di dominio.
- Applicato sia a **Dungeon Crawler Basic** (porte 9200/9201) sia a
  **Le Pergamene di Eldhôm** (porte 9210/9211).

---

## 2. Integrazione librerie C++ (gmXxx) — Mappa di responsabilità

### 2.1 Cosa è già generico e NON va modificato

| Meccanica di gioco | Libreria | Componente |
|---|---|---|
| Linea Temporale senza Round | `gmFlow` | `TimelineFlowController` |
| Ordine attori (chi è più indietro agisce prima) | `gmFlow` | `TimelineFlowController` — tie-break per rank |
| Finestra di reazione fuori turno | `gmFlow` | `ActionWindow(CompletionPolicy::ANY_SUBMITTED)` |
| Azione multi-step (passo per passo) | `gmFlow` | `StepBasedAction` |
| Stato attore (HP, statuses, tags, modifiers) | `gmActor` | `ActorStateCommon` |
| Scheda PG (livello, limiti mano/memoria/mazzo) | `gmActor` | `HeroState` |
| Prima Linea / Retroguardia | `gmActor` | `AreaPosition::FRONTLINE / BACKLINE` |
| Comportamento Mostro / struttura gruppo | `gmActor` | `MonsterGroupState` |
| Boss (fasi, rage, obiettivi collegati) | `gmActor` | `BossState` |
| Effetti carte (46 tipi) | `gmRules` | `EffectResolver` |
| Condizioni su posizione tattica | `gmRules` | `ConditionType::ACTOR_IN_POSITION` |
| Deck management multi-zona | `gmAlea` | `GmCompDeck` (6 zone: MainDeck / Hand / Memory / Discard / PlayArea / Banish) |
| Locazioni e connessioni mappa | `gmMap` | — |
| Event bus tra componenti | `gmDispatch` | — |
| Logging strutturato | `gmLog` | — |

### 2.2 Feature generiche aggiunte in questa sessione (F1–F5)

Piano formale: [plan_gm_lib.md](../plan_gm_lib.md).
Criterio di promozione a libreria: (1) appare in più tipi di gioco,
(2) no concetti di dominio, (3) parametrizzabile con policy esterne.

| Feature | Libreria | Componenti principali | Stato / Test |
|---|---|---|---|
| **F1** CardType + SequenceEngine | `gmAlea` | `CardType.hpp`, `SequenceState.hpp`, `SequenceEngine.hpp/.cpp` | ✅ 29/29 |
| **F2** TimelineMilestoneSystem | `gmFlow` | `TimelineMilestoneSystem.hpp/.cpp` | ✅ 19/19 |
| **F3** FormationValidator + Resolver | `gmActor` | `FormationRules.hpp`, `FormationValidator`, `FormationResolver`, `FormationCriteria` | ✅ 35/35 |
| **F4** BehaviorCardProcessor + Reactions | `gmActor` | `BehaviorCardProcessor`, `BehaviorReactionSystem` | ✅ 20/20 |
| **F5** 4 Widget PySide6 generici | `gmGui` | `TimelineWidget`, `FormationWidget`, `SequenceStateWidget`, `BehaviorCardWidget` | ✅ sintassi |

#### F1 — SequenceEngine (`gmAlea`)

Macchina a stati per sequenze di carte. Tipi (`CardType`):
`SINGLE` / `SEQ_START` / `SEQ_CONTINUE` / `SEQ_END` / `INSTANT`.
Regole: `SEQ_CONTINUE` è valida **solo** dentro una sequenza aperta;
`SINGLE` e nuovi `SEQ_START` terminano o non si aprono se è già attiva una
sequenza. `INSTANT` è sempre valida (fuori turno, trigger esterno).
Nessuna dipendenza da altre gmXxx.

#### F2 — TimelineMilestoneSystem (`gmFlow`)

Trigger "al tempo T scatta l'evento E" per qualsiasi gioco con timeline continua.
Aggancia `ITimelineAdapter::on_time_advanced()` **senza modificare l'interfaccia**.
Supporta milestone `one_shot` (auto-rimozione dopo il primo fuoco) e persistenti.
API chiave: `add_milestone(threshold, callback, one_shot)`, `advance(old, new, ctx)`,
`remove_milestone(threshold)`, `next_threshold()`.

#### F3 — FormationValidator + FormationResolver (`gmActor`)

Invariante generica `Retroguardia ≤ Prima Linea × fattore` (Descent, Gloomhaven,
Massive Darkness, ecc.). `FormationRules` è una policy POD iniettabile.
`FormationResolver` ordina i candidati da spostare con criteri prioritizzati:
by highest HP → by lowest timeline → random (seed). Stateless.

#### F4 — BehaviorCardProcessor + BehaviorReactionSystem (`gmActor`)

Turno dei gruppi mostri basato su Carte Comportamento. `BehaviorStep` POD:
`effect_type`, `amount`, `value`, `timeline_cost`, `optional`.
`BehaviorCardProcessor` applica `process_group_turn()` / `process_fallback()`
iniettando `CardCatalogLookup` + `StepExecutor` (zero accoppiamento con il gioco).
`BehaviorReactionSystem`: `has_reaction()` + `fire_reaction()` — usa
`BehaviorDeckOp` callback. Zero dipendenze dal dominio del gioco.

---

## 3. Integrazione librerie Python GUI (gmGui)

### 3.1 Filosofia di riuso

- La GUI **riusa i moduli/widget della libreria `pyLib/gmGui`** invece di
  reimplementarli per ogni gioco.
- Nessun colore hardcoded: tutti i valori cromatici passano per `resolve_color()`
  del `ThemeManager`.
- Ogni widget risponde a eventi engine serializzati — non gestisce logica di dominio.

### 3.2 Moduli riusabili già esistenti

| Modulo / Widget | Classe | Evento sorgente engine | Note |
|---|---|---|---|
| Deck Manager | `GmCompDeckModule` | `gmAlea.deck.*` | 6 zone; `set_enforce_action_cost(False)` se il backend valida le azioni |
| Actor panel | `GmActorModule` | `gmActor.snapshot` | Scheda HP, statuses, risorse |
| Map | `GmMapModule` | — | Locazioni, connessioni |
| Flow | `GmFlowModule` | — | Stato flusso |
| Area info | `GmMapAreaInfoModule` | — | Info stanza |
| Timeline | `TimelineWidget` | `gmflow.timeline.actors_updated` | **nuovo F5** |
| Formation | `FormationWidget` | `gmactor.formation.updated` | **nuovo F5** |
| Sequence state | `SequenceStateWidget` | `gmalea.sequence.state_changed` | **nuovo F5** |
| Behavior card | `BehaviorCardWidget` | `gmactor.behavior.card_changed` | **nuovo F5** |

### 3.3 Pattern di integrazione GUI ↔ engine

```
main.py
  └── sys.path.insert(pyLib/)          # bootstrap path senza install
  └── GameMainWindow(QMainWindow)
        ├── GameBridge (TCP server 9200 + client 9201)
        ├── EventRouter → registra handler per ogni typeId
        ├── GmCompDeckModule (riusato)
        ├── GmActorModule / GmMapModule (riusati)
        └── Widget game-specifici (estendono / wrappano i generici)
```

- La GUI apre il **server TCP degli eventi** (9200/9210) e si connette al
  **server comandi** del CoreEngine (9201/9211).
- Protocollo: 4 byte big-endian (lunghezza) + payload JSON UTF-8.
- I moduli gmGui si abbonano ai typeId che interessano con `subscribed_type_ids()`.

### 3.4 Bug critico risolto — build lazy e flag opt-out

`GmCompDeckModule._build_widget()` inizializzava `_enforce_action_cost = True`
ogni volta che il widget veniva costruito in modo lazy (al momento del docking),
**sovrascrivendo** `set_enforce_action_cost(False)` chiamata prima.
**Fix:** `self._enforce_action_cost = getattr(self, "_enforce_action_cost", True)`
→ preserva il valore già impostato. Valido per entrambi gli ordini di chiamata.

---

## 4. gmFlow — Decisioni specifiche

- **Modello timeline-first** (nessun Round classico): chi ha il timeline
  value più basso agisce per primo; tie-break per rank. Applicato a tutti i
  giochi costruiti sopra le gmXxx.
- `TimelineFlowController` resta **generico**: decide quando/chi agisce; la
  logica di dominio (carta, danno, formazione, trigger missione) non va
  inglobata nel controller.
- `TimelineMilestoneSystem` (F2) si aggancia via decoratore/helper su
  `on_time_advanced()` — zero modifica all'interfaccia `ITimelineAdapter`.
- Per Eldhôm: la priorità sequenza attiva (§2.2 regolamento) è gestita in
  `EldhomEngine::next_actor()` — non in `TimelineFlowController`.

---

## 5. gmRules e GRS — Decisioni specifiche

### 5.1 Separazione dei ruoli

| Componente | Responsabilità |
|---|---|
| `.grs` / JSON | **Descrizione** delle regole (dato, non codice) |
| `RuleBookLoader` | **Caricamento** da file / stringa in memoria |
| `RuleBook` | **Registry** delle `RuleDefinition` (strutture dati) |
| `gmRulesEngine` (façade) | **API pubblica** verso il gioco |
| `EffectResolver` | **Esecuzione** degli effetti su un `RuleContext` |
| `RuleGroupRegistry` | **Attivazione** dei gruppi (WHAT) — separato dall'esecuzione (HOW) |

### 5.2 Tabella EffectType

Prima di questa sessione la tabella completa **non esisteva** nella documentazione.
Ora è in [gmRules/gmRules_API.md](../gmRules/gmRules_API.md):
**46 tipi** totali, con semantica di `amount`/`value` per tipo, metodo
`RuleContext` invocato, raggruppati per categoria.
Aggiunti **5 nuovi tipi Dungeon Crawler** (sezione "Advanced"):
tra questi `SET_ACTOR_RESOURCE` (imposta la risorsa a `value` assoluto,
vs. aggiunta relativa) e il nuovo campo `chain_count` in `EffectSpec`.

### 5.3 Lifecycle regole di carta (decisione chiave)

> La regola di una Carta vale **solo quando il Giocatore gioca la Carta**,
> poi in genere finisce lì (`TRANSIENT`). Se la Carta **resta attiva** in
> `PlayArea` (o altra zona dichiarata), **resta attiva anche la sua regola**
> finché la carta è in quella zona.

Modello dati del collegamento Carta → Regole:

```json
{
  "group_id": "rg_village",
  "rule_ids": ["r_add_action_1", "r_add_actions_2"],
  "lifecycle": "TRANSIENT",
  "_active_zones": ["PlayArea"]
}
```

### 5.4 API di gestione runtime delle regole (implementata)

```cpp
// RuleBook + gmRulesEngine
remove_rule(rule_id)   // no-op se non esiste; ritorna bool
replace_rule(def)      // remove + re-add in una chiamata (upsert)
clear_rules()          // hot-reload completo: svuota tutto il RuleBook
```

Le regole sono **immediatamente operative** dopo il caricamento, in qualsiasi
momento durante l'esecuzione (avvio, hot-reload, su richiesta GUI).

---

## 6. Dungeon Crawler generico — Piano di riuso e sviluppo

### 6.1 Analisi di riuso (`~80% già coperto`)

Origine dell'analisi: regolamento de *Le Pergamene di Eldhôm* incrociato
con le gmXxx disponibili.

| Categoria | % riuso | Componente |
|---|---|---|
| Linea Temporale | 100% | `gmFlow::TimelineFlowController` |
| Ordine attori / rank | 100% | `TimelineFlowController` + tie-break rank |
| Scheda PG (HP, limiti, affiliazioni, attrezzatura) | 90% | `gmActor::HeroState` |
| Prima Linea / Retroguardia | 100% | `gmActor::AreaPosition::FRONTLINE/BACKLINE` |
| Mano / Scarti / Memoria / Mazzo | 100% | `gmAlea::GmCompDeck` (6 zone) |
| Gruppi Mostri + mazzo comportamento | 95% | `gmActor::MonsterGroupState` |
| Effetti carte (danno, cura, stato, tag…) | 100% | `gmRules::EffectResolver` (46 tipi) |
| Locazioni + connessioni mappa | 90% | `gmMap` |

**~20% da costruire ex novo** per ogni gioco:
- `SequenceEngine` (F1) — ora in `gmAlea`, generico.
- `FormationValidator/Resolver` (F3) — ora in `gmActor`, generico.
- `BehaviorCardProcessor` (F4) — ora in `gmActor`, generico.
- `TimelineMilestoneSystem` (F2) — ora in `gmFlow`, generico.
- Logic di dominio game-specific: `EldhomEngine`, adapter, loader missioni.

### 6.2 Architettura Eldhôm (prototipo di riferimento per Dungeon Crawler avanzato)

```
PySide6 GUI (GAME/Eldhom/GUI/)
  TimelineWidget  FormationWidget  SequenceWidget  BehaviorCardWidget
  GmCompDeckModule (riusato)  GmActorModule (riusato)  GmMapModule (riusato)
        │ TCP JSON events / commands (porte 9210/9211)
EldhomEngine (GAME/Eldhom/CoreEngine/)
  EldhomSequenceAdapter  →  gmAlea::SequenceEngine   (F1)
  EldhomFormationAdapter →  gmActor::FormationValidator (F3)
  EldhomBehaviorAdapter  →  gmActor::BehaviorCardProcessor (F4)
  MissionEventSystem     →  gmFlow::TimelineMilestoneSystem (F2)
  EldhomRuleAdapter      →  gmRules::RuleBook + EffectResolver
        │
gmXxx Libraries (nessuna modifica, solo wrapper sottili)
  gmFlow / gmActor / gmAlea / gmRules / gmMap / gmDispatch / gmLog
```

### 6.3 Regola delle azioni (stabilita per tutti i giochi)

- `actions` non può mai andare sotto zero.
- Il check corretto: **se `(actions − action_cost) < 0`** → avvisa
  ("Hai finito le Azioni: Continua comunque?").
- Se il risultato è esattamente `0`: azione **concessa senza avviso**.
- Carte a **costo 0** (es. monete) sempre giocabili anche con `actions = 0`.
- **Autorità = CoreEngine.** Il controllo client-side nel deck module è
  disattivabile con `set_enforce_action_cost(False)` (vedi §3.4).

---

## 7. Stato implementato al 2026-06-28

### Librerie C++ (gmXxx)

| Lib | Versione / Feature | Stato |
|---|---|---|
| `gmAlea` | v3.0 + F1 (CardType, SequenceEngine) | ✅ 29/29 test |
| `gmFlow` | v2.0 + F2 (TimelineMilestoneSystem) | ✅ 19/19 test |
| `gmActor` | v0.2 + F3 (Formation) + F4 (Behavior) | ✅ 35+20/55 test |
| `gmRules` | 46 EffectType, remove/replace/clear_rules | ✅ |
| `gmDispatch` | invariato | ✅ |
| `gmMap` | invariato | ✅ |
| `gmLog` | invariato | ✅ |

### Libreria Python (gmGui / pyLib)

| Componente | Stato |
|---|---|
| `GmCompDeckModule` | ✅ + fix lazy-build flag + `set_enforce_action_cost()` |
| `GmActorModule`, `GmMapModule`, `GmFlowModule` | ✅ |
| `TimelineWidget` (F5) | ✅ |
| `FormationWidget` (F5) | ✅ |
| `SequenceStateWidget` (F5) | ✅ |
| `BehaviorCardWidget` (F5) | ✅ |

### Gioco Eldhôm (prototipo)

| Fase | Componenti | Stato |
|---|---|---|
| P2 CardType + SequenceEngine | `gmAlea` F1 | ✅ |
| P3 FormationValidator | `gmActor` F3 | ✅ |
| P4 BehaviorCard + Reactions | `gmActor` F4 | ✅ |
| P8 4 Widget PySide6 | `gmGui` F5 | ✅ base |
| P6 EldhomEngine C++ | `EldhomEngine`, adapters, `MissionLoader` | ✅ 31/31 test |
| P7c GUI finale + comunicazione TCP | `eldhom_main_window.py`, `eldhom_bridge.py` | 🔧 in consolidamento |
| P1 JSON Data Layer | carte e missioni | ⏳ |
| P5 MissionEventSystem completo | — | ⏳ |
| P10 Integration Test | — | ⏳ |

---

## 8. Punti ancora aperti (da riprendere nei prossimi thread)

1. **gmFlow_API.md** — aggiungere sezione "Timeline Milestones (F2)".
2. **gmActor_API.md** — aggiungere sezioni F3 (Formation) e F4 (Behavior).
3. **Eldhôm P7c** — consolidare comunicazione GUI ↔ CoreEngine (connection
   lifecycle, gestione riconnessione).
4. **Eldhôm P1** — completare JSON Data Layer (carte per tutte le etnie,
   missioni complete).
5. **Governance versioning regole** — policy di merge/override quando due
   pacchetti add-on definiscono la stessa `RuleId`; compatibilità savegame.

---

## 9. Riferimenti rapidi

| Documento | Percorso |
|---|---|
| Piano feature generiche (F1–F5) | [plan_gm_lib.md](../plan_gm_lib.md) |
| Piano gioco Eldhôm | [GAME/Eldhom/info/PLAN.md](../GAME/Eldhom/info/PLAN.md) |
| API gmRules (tabella EffectType) | [gmRules/gmRules_API.md](../gmRules/gmRules_API.md) |
| API gmAlea (SequenceEngine) | [gmAlea/gmAlea_API.md](../gmAlea/gmAlea_API.md) |
| API gmGui widget | [pyLib/gmGui/gmGui_API.md](../pyLib/gmGui/gmGui_API.md) |
| Manuale generale librerie | [Game-Lib_readme.md](../Game-Lib_readme.md) |
| Regole di stile e architettura | cartella `.github/` |
| Storia Bug_01 + gmRules/GRS (sessione precedente) | [STORIA_decisioni_gmFlow_gmRules_GRS_DungeonCrawler.md](./STORIA_decisioni_gmFlow_gmRules_GRS_DungeonCrawler.md) |
