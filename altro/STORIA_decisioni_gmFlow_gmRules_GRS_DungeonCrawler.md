# Storia delle decisioni — gmFlow, gmRules/GRS, Regole add-on a runtime, Dungeon Crawler

**Scopo:** preservare le decisioni architetturali e di progetto prese nelle sessioni
di sviluppo, così da fungere da memoria storica per i thread futuri. Questo file
sostituisce il thread di conversazione, che può essere eliminato.

**Periodo coperto:** 2026-06-16 → 2026-06-28  
**Fonti:** transcript di sessione + artefatti reali verificati nel workspace.

---

## 1. GRS Tool (Python CLI)

### 1.1 Architettura generale

Il GRS Tool è uno **strumento Python CLI** per analizzare file `.grs` (linguaggio
di descrizione delle regole del gioco). Struttura a strati:

```
.grs file
    │
    ▼
  Lexer (tokenizzatore a regex)
    │
    ▼
  Parser → AST (Abstract Syntax Tree)
    │       ├─ lint (sintassi)
    │       ├─ validate (semantica)
    │       ├─ yaml (genera YAML canonico)
    │       └─ grapho (genera diagramma Mermaid)
```

Il Parser è il fondamento: tutte le altre funzionalità dipendono da un AST corretto.

### 1.2 Comandi CLI disponibili

```bash
grs lint   file.grs               # validazione sintattica, errori per riga
grs validate file.grs             # validazione semantica (10 regole della spec)
grs yaml   file.grs [-o file.yaml]
grs grapho file.grs [-o file.md] [--rule NomeRegola]
grs check  file.grs               # alias: lint + validate insieme
```

- Entry point: `python -m grs`
- `--format text|json` per lint/validate
- Comportamento tipo `git`/`pip`/`cmake`: `--help` per ogni sottocomando

### 1.3 Fasi di implementazione

| Fase | Contenuto |
|---|---|
| 1 | Struttura progetto + Lexer |
| 2 | Parser + AST |
| 3 | `lint` con diagnostica dettagliata per riga |
| 4 | `validate` semantico (10 regole della spec) |
| 5 | `yaml` — traversal AST → dict Python → serializzazione YAML; mapping `GRS → YAML` dalla spec |
| 6 | `grapho` — traversal AST → grafo Mermaid nello stile `--rule NomeRegola` |
| 7 | CLI e packaging (`cli.py` con argparse subparsers, entry point `python -m grs`) |

Manuale: [tools/GRS_MANUAL.md](../tools/GRS_MANUAL.md)

### 1.4 Warning V-009 — logica stabilita

**Definizione:** effetto con `[stop]` segue un effetto con `[continue/optional]` →
comportamento insolito.

**Concetto:** `[stop]` interrompe la catena se l'effetto fallisce; `[continue]` e
`[optional]` dicono "vai avanti comunque". Se un effetto dice "fallimento ignorato"
e quello successivo dice "fermati se fallisce", la catena è incoerente.

```grs
THEN E_DiscardPlayedCard [optional]   ← fallimento ignorato
 AND THEN E_MarkActionUsed [stop]     ← ← ← V-009 WARNING qui
```

### 1.5 packaging

Problema rilevato: `pip install -e tools/` fallisce perché `tools/` non ha
`pyproject.toml` / `setup.py`. Risolto creando la struttura di packaging corretta
(non `pip install -e` dalla cartella radice).

---

## 2. gmRules e GRS

### 2.1 RuleBook — da JSON Carta a Regola eseguibile

- **`RuleBook`** trasforma il JSON delle Carte in **vere regole** pronte per essere
  triggerate ed eseguire gli Effetti.
- Caricamento da file o da stringa, **senza dipendenze esterne** (parser JSON
  custom): `RuleBookLoader::load_json()`, `gmRulesEngine::load_rules_json()`,
  `gmRulesEngine::load_rules_json_string()`.
- Il motore riceve una **lista unificata** di regole ordinata per `priority` e
  non distingue se una regola proviene da BASE o da una Carta.
- **Accumulo:** più chiamate a `load_rules_json*` aggiungono definizioni senza
  cancellare quelle esistenti; necessario per caricare più file di regole separati.

### 2.2 Carte con Regola integrata (gmAlea + gmRules)

Decisione chiave: le librerie **gmAlea** e **gmRules** devono supportare
**nativamente** il concetto di "Carta (o Token) con Regola integrata".

Ogni Carta/Token può essere **linkata a uno o più gruppi di regole** tramite ID.
Modello dati del link (usato nel sandbox e nei dati reali):

```json
{
  "group_id": "rg_village",
  "rule_ids": ["r_add_action_1", "r_add_actions_2"],
  "lifecycle": "TRANSIENT",
  "_active_zones": ["PlayArea"],
  "_desc": "Attivo quando Village è in PlayArea."
}
```

**Lifecycle delle regole di carta — distinzione fondamentale:**
- `TRANSIENT` — la regola vale solo quando il Giocatore gioca la Carta, poi finisce.
- `PERSISTENT` — regola attiva per tutta la durata della sessione.
- `TRIGGER_BOUND` — attiva solo nella finestra del trigger specifico.
- **Caso particolare:** se la Carta **resta attiva** (es. in `PlayArea`), la sua
  regola resta attiva finché la carta è nelle `_active_zones`.

### 2.3 EffectType — tabella completa (46 tipi)

Prima del thread non esisteva una tabella di riferimento. Creata e documentata in
[gmRules/gmRules_API.md](../gmRules/gmRules_API.md) con:
- Semantica di `amount`/`value` per tipo
- Metodo `RuleContext` chiamato internamente
- Raggruppamento per categoria (Salute / Carte / Status / Tag / Mappa / Escape hatch)

Totale attuale: **41 tipi base + 5 nuovi Dungeon Crawler = 46 tipi**.

**5 nuovi EffectType per Dungeon Crawler** aggiunti al thread (Q2 → implementati):

| EffectType | Semantica |
|---|---|
| `CHAIN_DRAW` | Pesca N carte extra durante una sequenza attiva |
| `SET_ACTOR_RESOURCE` | Imposta la risorsa a `value` esatto (non somma come `MODIFY_RESOURCE`) |
| `PUSH_ACTOR` | Sposta un attore in formazione (Frontline ↔ Backline) |
| `APPLY_CHAIN_STATUS` | Applica uno status solo se la sequenza è attiva |
| `REVEAL_CARD_TOP` | Rivela la carta in cima al mazzo senza pescarne |

Nuovo campo `int chain_count = 0` in `EffectSpec` per supportare `CHAIN_DRAW` e `APPLY_CHAIN_STATUS`.

File toccati per i 5 nuovi tipi: [EffectType.hpp](../gmRules/effect/EffectType.hpp),
[EffectSpec.hpp](../gmRules/effect/EffectSpec.hpp),
[EffectResolver.cpp](../gmRules/effect/EffectResolver.cpp),
[RuleBookLoader.cpp](../gmRules/loader/RuleBookLoader.cpp).

### 2.4 gmRules — fasi di sviluppo

| Phase | Contenuto | Stato |
|---|---|---|
| 1 | Interfacce & stub (`RuleContext`, `RuleResult`, `EffectSpec`, ecc.) | ✅ |
| 2 | Core resolvers (`TargetResolver`, `ConditionEvaluator`, `EffectResolver`) | ✅ |
| 3 | Status engine (`StatusDefinition`, `StatusInstance`, `StatusEngine`) | ✅ |
| 4 | Facade + RuleGroup registry (`gmRulesEngine`, `RuleGroupRegistry`) | ✅ |
| 5 | RuleBook + Loader (JSON hand-written, no dipendenze esterne) | ✅ |
| 6 | `RuleContext::modify_resource` + sandbox mock_engine.py | ✅ |
| 7 | CMakeLists.txt + build + CTest C++ per `test_rule_book` | ⏳ PROSSIMO |

**Architettura confermata:**
- `RuleGroupRegistry` → risponde a QUALE set di regole è attivo (WHAT). Non esegue.
- `RuleBook` → mappa `RuleId → RuleDefinition → EffectSpec[]`.
- `gmRulesEngine` → orchestra tutto; espone `load_rules_json()`, `resolve_rule()`, ecc.

### 2.5 Sandbox mock_engine.py — stato Phase 6

File: [GAME/gmGui-Sandbox/mock_engine.py](../GAME/gmGui-Sandbox/mock_engine.py)

- Carica `data/dominion_rules.json` e `data/rule_groups.json`.
- Simula `CardRuleBridge + RuleGroupRegistry`: carta in `PlayArea`/`Memory` → emette `gmRules.rule_group.activated/deactivated`.
- Esegue effetti via `_apply_rule_effects`: gestisce `MODIFY_RESOURCE`, `DRAW_CARDS`, `APPLY_STATUS`.
- `GmActorModule` mostra sezione **Risorse** separata dagli Status; si aggiorna live su `gmActor.actor.resource_changed`.
- Costo automatico: giocata carta da `CardHand` → `PlayArea` → deduce 1 azione (se `action_cost > 0`).
- `DRAW_CARDS` muove carte reali da `MainDeck` a `CardHand` ed emette `gmAlea.deck.card_moved`.
- `APPLY_STATUS` con `target=SELF` → `gmActor.actor.status_added` per il giocatore; con `target=ALL_ENEMIES_IN_LOCATION` → per tutti gli avversari.
- **Porta TCP:** EVENT_PORT = 9000 (GUI in ascolto).

**Carte tesoro Dominion** (Copper/Silver/Gold): `action_cost: 0`, si giocano sempre senza costo azione.

---

## 3. Capacità di creare/eliminare Regole add-on a runtime

Domanda del thread: le regole possono solo aggiungersi, o si possono **eliminare**
e **sostituire** a runtime?

### 3.1 Risposte alle Q3/Q4/Q5 del thread

| Q | Domanda | Risposta implementata |
|---|---|---|
| Q1 | Dove trovo la documentazione degli EffectType? | Creata tabella completa in `gmRules_API.md` |
| Q2 | Nuovi EffectType per Dungeon Crawler? | 8 suggeriti, 5 implementati (vedi §2.3) |
| Q3 | Le regole si possono eliminare a runtime? | **Sì.** `remove_rule(rule_id)` implementato (no-op se non esiste, ritorna `false`) |
| Q4 | Si possono sostituire? | **Sì.** `replace_rule(def)` = shortcut per `remove_rule` + `register_rule`; se non esiste, semplicemente aggiunge |
| Q5 | Hot-reload completo? | **Sì.** `clear_rules()` svuota tutto il RuleBook |

### 3.2 API completa del ciclo di vita delle regole a runtime

| API | Semantica |
|---|---|
| `register_rule(def)` / `load_json()` | Aggiunge regole (accumulo, non reset) |
| `remove_rule(rule_id)` | Rimuove una regola; no-op se non esiste |
| `replace_rule(def)` | Equivale a remove + register |
| `clear_rules()` | Svuota tutto il RuleBook (hot-reload) |

Le regole sono **immediatamente operative** dopo il caricamento.

---

## 4. Game-Lib_readme.md — ristrutturazione

Prima del thread: il README duplicava namespace, versioni e feature da ogni API doc.
Dopo il thread: trasformato in **navigation hub** che punta ai file `*_API.md`.

- **Tabella di navigazione** con link diretto a ogni `*_API.md` (fonte autorevole).
- **Quick Reference table** non duplica dati — rimanda alla fonte.
- **CMake Integration Status** (nuovo, era assente).
- **Principio single source of truth**: chi modifica una lib aggiorna solo il file API;
  il README rimane coerente perché punta al file.

---

## 5. Feature generiche promosse a libreria (plan_gm_lib.md)

Criterio per promuovere una meccanica a libreria:
1. Appare in più tipi di gioco.
2. Non contiene concetti di dominio del gioco.
3. Può essere parametrizzata da dati/policy iniettati dall'esterno.

**~75% della logica di un dungeon crawler/deckbuilder è già coperta dalle gmXxx.**

| Feature | Libreria | Componenti | Test |
|---|---|---|---|
| F1 — CardType + SequenceEngine | `gmAlea` | Macchina a stati SINGLE/SEQ_START/SEQ_CONTINUE/SEQ_END/INSTANT; `can_play()`, `advance()`, `interrupt()`, `reset()` | 29/29 ✅ |
| F2 — TimelineMilestoneSystem | `gmFlow` | Trigger temporali a soglia `(old, new]`; one-shot vs. persistente; `remove_milestone()`, `clear()`, `next_threshold()` | 19/19 ✅ |
| F3 — FormationValidator + FormationResolver | `gmActor` | Regola `RG ≤ PL` (Backline ≤ Frontline); criteri prioritizzati (HP, timeline, dado) iniettabili come lambda; `by_highest_hp()`, `by_lowest_timeline()`, `random(seed)` | 35/35 ✅ |
| F4 — BehaviorCardProcessor + BehaviorReactionSystem | `gmActor` | Loop "per ogni step → per ogni mostro → skip se impossibile → paga costo"; reazione "scarta attiva → pesca nuova"; callback iniettato per effetti | 20/20 ✅ |
| F5 — 4 widget generici PySide6 | `gmGui` | `TimelineWidget`, `FormationWidget`, `SequenceStateWidget`, `BehaviorCardWidget` | Sintassi ✅ |

### 5.1 Principi GUI (F5) — regole di stile applicate

- **Nessun colore hardcoded** — tutto via `resolve_semantic_color()` o proprietà QSS `tone`, `chip`, `text_role`.
- **Spacing**: solo 4, 8, 16, 24, 32 px.
- **State changes**: via `setProperty()` + `style().polish()` — mai `if theme == ...`.
- Tone values in uso: `danger`, `accent`, `muted`, `error`.
- Widget milestone in `TimelineWidget`: linea tratteggiata + label, colore blocchi per `kind` con mappa `_KIND_TONE`.
- `FormationWidget`: badge `tone="danger"` quando `backline > frontline` o cap superato.
- `BehaviorCardWidget`: step con indicatore `✓/▶/○` + badge `⚡` reazione nascosto se card vuota.

---

## 6. Gioco Dungeon Crawler Basic

### 6.1 10 carte con regole integrate (data approvati)

| card_id | Nome | Tipo | Costo | Effetto |
|---|---|---|---|---|
| `colpo_efficace` | Colpo Efficace | ACTION | 1 | 2 danni mischia (distanza 1) |
| `passo_veloce` | Passo Veloce | ACTION | 1 | Muovi 2 locazioni |
| `parata` | Parata | ACTION | 1 | Status `difeso` su SELF |
| `pozione_di_cura` | Pozione di Cura | ITEM | 1 | +3 PV su SELF |
| `pugno_di_ferro` | Pugno di Ferro | ACTION | 2 | 4 danni mischia (distanza 1) |
| `grido_di_guerra` | Grido di Guerra | ACTION | 1 | Status `energico` su SELF (+2 danno prossimo attacco) |
| `tiro_rapido` | Tiro Rapido | ACTION | 1 | 2 danni a distanza (portata 3 locazioni) |
| `veleno` | Veleno | ACTION | 1 | Status `avvelenato` su 1 nemico (-1 PV/turno) |
| `furia_cieca` | Furia Cieca | ACTION | 2 | 2 danni AOE tutti i nemici adiacenti |
| `scudo_antico` | Scudo Antico | ITEM | 1 | Tag `scudo_equipaggiato`; -1 PV/attacco per 2 attacchi |

### 6.2 DungeonRuleAdapter — regola di reach attacco

**Bersaglio valido** se nella **stessa stanza o in stanza adiacente**. Regola
implementata in `GAME/Dungeon-Crawler-Basic/engine/DungeonRuleAdapter.cpp`.

### 6.3 Bug_01 — "Azioni esaurite" popup errato

**Problema:** il popup "Hai finito le Azioni: continua comunque?" appariva anche
con `actions = 0` quando si giocava una carta a costo 0 (es. Copper).

**Causa radice:**
```python
# In gm_comp_deck_module.py — build lazy del widget
def _build_widget(self):
    self._enforce_action_cost = True   # ← sovrascriveva set_enforce_action_cost(False)
```
Il flag veniva inizializzato in `_build_widget()` (lazy), che girava **dopo**
`set_enforce_action_cost(False)` annullandone l'effetto.

**Fix:**
```python
self._enforce_action_cost = getattr(self, "_enforce_action_cost", True)
```

**Regola corretta sul check:**
- `action_cost == 0` → **mai** bloccare, si gioca sempre liberamente.
- `action_cost > 0` e `(actions - action_cost) < 0` → MessageBox "Hai finito le Azioni. Continua comunque?"
- `action_cost > 0` e `(actions - action_cost) >= 0` → si gioca senza popup.
- Le azioni **non possono mai andare sotto zero** (cappate con `max(0, ...)`).

**Autorità sul costo azione = CoreEngine.** Il check client-side in `gm_comp_deck_module`
è disattivabile via `set_enforce_action_cost(False)`.

**Prova di innocenza del CoreEngine:** test di isolamento
[test_coreengine_bug01.py](../GAME/Dungeon-Crawler-Basic/tools/test_coreengine_bug01.py).

---

## 7. gmFlow — estensioni (FlowPhase + bridges) ← da THREAD CORRENTE

### 7.1 PhaseContext e FlowPhase (Phase 5–7)

Aggiunto il sistema di **gerarchie di sessione arbitrariamente profonde** senza
corrompere il contesto radice.

| Componente | Decisione chiave |
|---|---|
| `PhaseContext` | Sottoclasse di `GameContext`; IDs locali (round/turn/phase); GameState/ActorRegistry/EventBus presi a prestito dal genitore — mai duplicati |
| `FlowPhase` | Implementa `IPhase` — dal parent è una fase normale; internamente possiede `IFlowController` + `PhaseContext` |
| `SequentialFlowController` | Solo 3 cambiamenti: `_round_index → protected`; `advance_phase() → virtual`; aggiunto `current_phase() const` |

`scope_prefix` (es. `"epoch_1"`) disponibile in `PhaseContext`; il caller
costruisce l'ID qualificato (`"epoch_1.round_1"`) se necessario.

`ActionQueue` resta globale e unica per sessione. Nessun limite di nesting
imposto dal framework (pratico: 3 livelli).

### 7.2 FlowRulesGateway e ActionGateway (Phase 11–12)

Adapter **opzionali** in `gmFlow/bridges/` che collegano il lifecycle events di
gmFlow a un motore di regole esterno.

**`FlowRulesPayload`** — struct flat, copiabile:
```cpp
struct FlowRulesPayload {
    std::string actor_id, action_id, phase_id, round_id, turn_id;
    std::string scope_prefix;   // non-empty se dentro FlowPhase
    std::string event_type;     // costante EVT_* scatenante
};
```

**`register_flow_rules_gateway(bus, cb×10)`** — registra fino a 10 callback
opzionali (nullptr silenziosamente ignorato) per: `EVT_TURN_STARTED`,
`EVT_TURN_ENDED`, `EVT_ROUND_STARTED`, `EVT_ROUND_ENDED`, `EVT_PHASE_ENTERED`,
`EVT_PHASE_EXITED`, `EVT_WINDOW_OPENED`, `EVT_WINDOW_CLOSED`,
`EVT_ACTION_SUBMITTED`, `EVT_ACTION_COMPLETED`.

**`ActionGateway`** — decorator `IAction`:
- `ActionPreCheck`: in `validate()` dopo inner; blocca con `RULE_VIOLATION`.
- `ActionPostHook`: in `execute()` dopo inner; solo informativo, non blocca.
- Regola R6: solo `validate()` può bloccare un'azione.

**Stato finale gmFlow:** 7/7 test CTest passano (Release).

---

## 8. Gioco Eldhôm — Le Pergamene di Eldhôm

### 8.1 Identità gameplay (immutabile)

- Dungeon crawler card-based con deckbuilding.
- **Timeline continua** (nessun Round classico) — usa `gmFlow::TimelineFlowController`.
- Formazioni Prima Linea / Retroguardia.
- Gruppi Mostri con Carte Comportamento autonome.
- Core C++ (regole/stato/flusso) + GUI Python/PySide6 (visualizzazione/interazione).

### 8.2 Regole di gioco base (da spec AI)

**Status negativi:** Vista Offuscata, Rallentato, Avvelenato (= Vista Offuscata +
Rallentato duraturo), Immobilizzato, Disarmato, Sanguinante (-1 PV a inizio turno),
Svenuto (solo Recupera), Maledetto (TBD).

**Status positivi:** Vista Acuita, Concentrato (prossima azione −2⌛, min 1⌛),
Energizzato (= Vista Acuita + Concentrato duraturo), Invisibile (non bersagliabile),
Resistente N (riduce danni di N), Benedetto (TBD).

**Azioni semplici del PG (SimpleActionType):**
- `MOVE` — sposta fino a 2 Loc, costo 1⌛
- `ATTACK` — infliggi 1 dano su bersaglio vicino, costo 2⌛
- `INTERACT` — usa elemento scena, costo 3⌛
- `RECOVER` — +1 PV, scarta/pesca una carta, costo 3⌛

**Linea Temporale:** ogni azione paga immediatamente `soggetto.timeline += costo_⌛`.
Non esiste pagamento cumulativo a fine turno. Agisce sempre chi è più indietro.

### 8.3 EldhomEngine (P6) — struttura C++

File chiave: `GAME/Eldhom/CoreEngine/`

```
engine/
    EldhomTypes.hpp         ← alias, costanti timeline, event string keys, ActionResultCode
    CardData.hpp            ← EldhomCard, EldhomEffect, LocationDef
    EldhomEngine.hpp/.cpp   ← orchestratore missione
sequence/
    EldhomSequenceAdapter   ← thin adapter → gmAlea::SequenceEngine
formation/
    EldhomFormationAdapter  ← thin adapter → gmActor::FormationValidator (§41)
monsters/
    EldhomBehaviorAdapter   ← thin adapter → gmActor::BehaviorCardProcessor
mission/
    MissionDefinition.hpp   ← POD missione (location graph, roster, deck)
    MissionEventSystem      ← vittoria (ALL_MONSTERS_ELIMINATED) / sconfitta
                               (TIME_LIMIT + ALL_PG_KO) + timeline milestones
targeting/
    TargetingFilter         ← §15 Proiezione (PL scherma RG)
rules/
    EldhomRuleAdapter       ← bridge gmRules ↔ EldhomEngine
bridge/
    EldhomGuiBridge         ← TCP client verso GUI (porta 9210); format: 4-byte BE + UTF-8 JSON
    EldhomCmdServer         ← TCP server per comandi GUI (porta 9211)
```

**Turn loop — caller-driven (non autonomo):**
1. `next_actor()` → identifica chi agisce
2. Se `HERO`: `do_simple_action()` o `play_card()` fino a `end_hero_turn()`
3. Se `MONSTER_GROUP`: `resolve_next_group_turn()`
4. Repeat fino a `is_over()`

**Factory:** `EldhomEngine::from_definition(def, card_catalog, behavior_catalog)`

**ActionResultCode:** `OK`, `ERR_NOT_YOUR_TURN`, `ERR_CARD_NOT_IN_HAND`,
`ERR_CARD_NOT_PLAYABLE`, `ERR_NO_SEQUENCE_ACTIVE`, `ERR_UNKNOWN_ACTOR`.

**TargetingFilter (§15 Proiezione):**
- Se esistono attori FRONTLINE vivi nella fazione bersaglio → solo quelli sono
  bersagliabili.
- Se nessun FRONTLINE (Scompaginamento già risolto) → qualsiasi attore nella
  locazione è bersagliabile.

**Data files:** `mission_01.json` (Thael + Velyr vs 3 Briganti, 3 locazioni),
`cards_base.json`, `behavior_brigante_comune.json`.

**Test:** 31/31 PASS iniziali; poi 43/44 (1 fallimento non correlato a Eldhôm).

**Commit message P6:**
```
GAME/Eldhom: P6 — CoreEngine C++ (Turno PG + Turno Gruppo Mostri)
Thin adapters (no reimplementazione logica gmXxx):
- EldhomSequenceAdapter → gmAlea::SequenceEngine
- EldhomFormationAdapter → gmActor::FormationValidator
- EldhomBehaviorAdapter → gmActor::BehaviorCardProcessor
Tests: 31/31 PASS
```

### 8.4 EldhomEngine GUI (P7) — struttura Python

**Porte TCP:**
- `9210` — GUI TCP server (evento ricevuti dall'engine C++)
- `9211` — Engine C++ TCP server (comandi inviati dalla GUI)

**Wire format:** 4-byte big-endian length prefix + UTF-8 JSON (uguale a tutti gli
altri bridge gmGui).

**Componenti GUI:**

```
GAME/Eldhom/GUI/
app/
    eldhom_bridge.py       ← wrappa gmGui::EngineReceiver + EngineSender
    eldhom_main_window.py  ← finestra principale
    event_router.py        ← instrada eventi entranti ai widget specifici
    mission_select_dialog.py
widgets/
    map_widget.py
    timeline_widget.py
    hero_panel_widget.py
    hand_widget.py
    action_panel_widget.py
    log_widget.py
```

**Bug critico GUI risolto (comunicazione asimmetrica):**

Causa: `set_on_event()` in `eldhom_bridge.py` faceva:
```python
self._receiver.on_message = handler   # ❌ attributo mai letto
```
`EngineReceiver` è un `QThread` che emette il **Qt Signal `envelope_received`**,
non un attributo. Il segnale veniva emesso ma non era collegato a nessuno slot.
Risultato: la GUI restava su "Non connesso" mentre i comandi GUI→Engine funzionavano.

**Fix:**
```python
self._receiver.envelope_received.connect(handler)   # ✓ Qt Signal (QueuedConnection)
```
Essendo cross-thread, Qt usa automaticamente `QueuedConnection` → thread-safe.

**Problema di startup risolto:** timeout di 1.5s tra avvio GUI e Engine insufficiente.
**Fix:** aumentato a 5s in `run_eldhom.bat`.

**run_eldhom.bat:** lancia GUI (porta 9210) + Engine (porta 9211) in 2 finestre
cmd separate con 5s di attesa tra i due.

---

## 9. Piano integrazioni librerie (grs-integration-implementation-plan.md)

File: [gmRules/specs/grs-integration-implementation-plan.md](../gmRules/specs/grs-integration-implementation-plan.md)

| Capitolo | Libreria | Priorità | Stato |
|---|---|---|---|
| 1 | gmRules (DSL/vocab esteso) | P0 | ⏳ |
| 2 | gmFlow (lifecycle → gmRules) | P1 | ✅ Completato (Phase 13–15) |
| 3 | gmDispatch (bridge eventi) | P2 | ⏳ |
| 4 | gmActor (eventi HP/morte/risorse + effetti) | P3 | ⏳ |
| 5 | gmAlea (pre/post resolve draw/roll) | P4 | ⏳ |
| 6 | gmMap (eventi spatial + effetti barriere) | P5 | ⏳ |

---

## 10. Decision Log sintetico (usabile come prompt per nuovi thread)

| ID | Decisione |
|---|---|
| DLOG-01 | Timeline continua gmFlow (non round classici) per Eldhôm e dungeon crawler. |
| DLOG-02 | gmRules separa attivazione gruppi (`RuleGroupRegistry`) ed esecuzione (`RuleBook`/Engine). |
| DLOG-03 | Regole add-on runtime: load multiplo (accumulo); `remove_rule()`, `replace_rule()`, `clear_rules()` disponibili. |
| DLOG-04 | Core C++ mantiene autorità su stato e regole; GUI Python è consumer event-driven. |
| DLOG-05 | Estensioni a gmXxx ammesse solo additive e retrocompatibili. |
| DLOG-06 | `FlowPhase` è un `IPhase` normale per il parent controller; gerarchia trasparente. |
| DLOG-07 | `ActionGateway`: solo `validate()` blocca; `execute()` sempre chiama inner + post-hook. |
| DLOG-08 | `ActionQueue` globale per sessione; azioni di livelli FlowPhase interni nella stessa coda. |
| DLOG-09 | RuleBookLoader: parser JSON hand-written, no dipendenze esterne. |
| DLOG-10 | Sandbox mock_engine simula CardRuleBridge+RuleGroupRegistry in Python; porta TCP 9000. |
| DLOG-11 | GRS Tool: Lexer → AST → lint/validate/yaml/grapho; V-009 WARNING = stop dopo continue/optional. |
| DLOG-12 | EffectType: 46 tipi totali; campo `chain_count` in EffectSpec per Dungeon Crawler. |
| DLOG-13 | Bug_01: check azioni solo se `(actions - cost) < 0`; `action_cost=0` mai bloccato; fix in `_build_widget()`. |
| DLOG-14 | Eldhôm: porta 9210 GUI server (eventi), 9211 Engine server (comandi); 4-byte BE + JSON. |
| DLOG-15 | Eldhôm bridge fix: `envelope_received.connect(handler)` non `on_message = handler`. |
| DLOG-16 | GUI widgets: nessun colore hardcoded; spacing 4/8/16/24/32px; state via `setProperty()`. |
| DLOG-17 | TargetingFilter §15: PL scherma RG — se FRONTLINE vivo nella fazione bersaglio, solo quelli targettabili. |
| DLOG-18 | `SimpleActionType`: MOVE(1⌛), ATTACK(2⌛), INTERACT(3⌛), RECOVER(3⌛). |

---

## 11. Prossimi passi prioritari

1. **gmRules Phase 7** — `test_rule_book` in CMakeLists.txt + build + CTest PASS.
2. **grs-integration Cap 1** — Estendere vocabolario GRS (`TriggerType`, `EffectType`, parser).
3. **Eldhôm P6 completo** — `EldhomEngine` + GUI finale per test reali.
4. **grs-integration Cap 4** — Adapter gmActor (eventi HP/morte/risorse + effetti).
5. **grs-integration Cap 5** — Adapter gmAlea (pre/post resolve draw/roll).

---

## 12. Riferimenti file

| Argomento | File |
|---|---|
| GRS Tool manuale | [tools/GRS_MANUAL.md](../tools/GRS_MANUAL.md) |
| gmRules API + EffectType | [gmRules/gmRules_API.md](../gmRules/gmRules_API.md) |
| gmRules piano fasi | [gmRules/PLAN.md](../gmRules/PLAN.md) |
| gmFlow API (bridges inclusi) | [gmFlow/gmFlow_API.md](../gmFlow/gmFlow_API.md) |
| gmFlow piano fasi 5–15 | [gmFlow/FlowPhase_PLAN.md](../gmFlow/FlowPhase_PLAN.md) |
| Piano integrazione librerie | [gmRules/specs/grs-integration-implementation-plan.md](../gmRules/specs/grs-integration-implementation-plan.md) |
| Piano feature generiche | [plan_gm_lib.md](../plan_gm_lib.md) |
| Eldhôm regole base spec | [GAME/Eldhom/info/regole_base_eldhom_ai_spec.md](../GAME/Eldhom/info/regole_base_eldhom_ai_spec.md) |
| Eldhôm piano sviluppo | [GAME/Eldhom/info/PLAN.md](../GAME/Eldhom/info/PLAN.md) |
| Sandbox mock engine | [GAME/gmGui-Sandbox/mock_engine.py](../GAME/gmGui-Sandbox/mock_engine.py) |
| Regole Dominion (esempio) | [GAME/gmGui-Sandbox/data/dominion_rules.json](../GAME/gmGui-Sandbox/data/dominion_rules.json) |
| Carte Dungeon Crawler Basic | [GAME/Dungeon-Crawler-Basic/data/cards_dungeon.json](../GAME/Dungeon-Crawler-Basic/data/cards_dungeon.json) |
| gmGui API | [pyLib/gmGui/gmGui_API.md](../pyLib/gmGui/gmGui_API.md) |
| Manuale librerie | [Game-Lib_readme.md](../Game-Lib_readme.md) |
| Style rules C++ | [style-rules.md](../style-rules.md) |


---

## 1. gmFlow

### 1.1 Decisioni prese

- La **Linea Temporale senza Round** è una meccanica **generica di libreria**, non
  specifica di un gioco: chi è più indietro nel tempo ha diritto di iniziare per
  primo il prossimo turno. Già coperta da `gmFlow::TimelineFlowController`
  (tie-break per rank; nessun concetto di Round).
- Confermato come **riuso al 100%** per giochi a timeline continua (Eldhôm,
  dungeon crawler avanzati, ecc.).
- Aggiunto il sistema **TimelineMilestoneSystem** (feature F2 del piano generico):
  trigger "al tempo T scatta l'evento E", pattern universale per qualsiasi gioco
  con timeline continua. Si aggancia a `ITimelineAdapter::on_time_advanced()`
  **senza modificare l'interfaccia esistente**.

### 1.2 Artefatti

| Componente | File | Stato |
|---|---|---|
| Timeline senza Round | [gmFlow/flow/TimelineFlowController.hpp](../gmFlow/flow/TimelineFlowController.hpp) | Produzione |
| Timeline milestones (F2) | [gmFlow/flow/TimelineMilestoneSystem.hpp](../gmFlow/flow/TimelineMilestoneSystem.hpp) | 19/19 test |
| Finestra di reazione fuori turno | `gmFlow::ActionWindow(CompletionPolicy::ANY_SUBMITTED)` | Riuso |
| Azione multi-step | `gmFlow::StepBasedAction` | Riuso |

---

## 2. gmRules e GRS

### 2.1 Tool GRS (file `.grs`)

- Tool **Python CLI** per i file `.grs` (linguaggio di descrizione regole).
  Comandi: `lint` (sintassi), `validate` (semantica), `yaml` (genera YAML canonico).
- Decisione di **documentare il tool GRS** nel manuale generale
  [Game-Lib_readme.md](../Game-Lib_readme.md) e di uniformare namespace e stato
  fase/produzione incrociando automaticamente i file API di ogni libreria
  (principio **single source of truth** per il README).
- Warning semantico stabilito: *"effetto con `[stop]` segue un effetto con
  `[continue/optional]` — comportamento insolito"* (segnala catene di effetti
  potenzialmente incoerenti).

### 2.2 RuleBook — da JSON Carta a Regola eseguibile

- **`RuleBook`** in gmRules trasforma il JSON delle Carte in **vere regole**
  pronte a essere triggerate ed eseguire gli Effetti.
- Caricamento da file o da stringa, **senza dipendenze esterne** (parser JSON
  custom): `RuleBookLoader::load_json()`, `gmRulesEngine::load_rules_json()`,
  `load_rules_json_string()`.
- Il motore riceve una **lista unificata** di regole ordinata per `priority` e
  non distingue se una regola proviene da BASE o da una Carta.

### 2.3 Carta con Regola Integrativa (gmAlea + gmRules)

Decisione chiave (2026-06-25): le librerie **gmAlea** e **gmRules** devono
supportare **nativamente** il concetto di "Carta (o Token) con Regola integrata".

- Ogni Carta/Token può essere **linkata a uno o più gruppi di regole** tramite ID.
- Modello dati del link (esempio reale usato in sandbox):

  ```json
  {
    "group_id": "rg_village",
    "rule_ids": ["r_add_action_1", "r_add_actions_2"],
    "lifecycle": "TRANSIENT",
    "_active_zones": ["PlayArea"],
    "_desc": "Attivo quando Village è in PlayArea. Fornisce +1 e +2 Azioni."
  }
  ```

- **Lifecycle delle regole di carta** — distinzione fondamentale:
  - La regola di una Carta vale **solo quando il Giocatore gioca la Carta**, poi
    in genere finisce lì (`TRANSIENT`).
  - **Caso particolare:** se la Carta **resta attiva** (es. in `PlayArea`), allora
    resta attiva **anche la sua regola** finché la carta è nelle `_active_zones`.

### 2.4 EffectType

- Creata la **tabella di riferimento completa** degli EffectType in
  [gmRules/gmRules_API.md](../gmRules/gmRules_API.md) (prima non esisteva):
  semantica di `amount`/`value`, metodo `RuleContext` chiamato, raggruppamento per
  categoria (Salute / Carte / Status / Tag / Mappa / Escape hatch).
- Aggiunti **5 nuovi EffectType** specifici per Dungeon Crawler (sezione
  "Advanced / Dungeon Crawler"), portando il totale a **46 tipi**. Tra questi
  `SET_ACTOR_RESOURCE` (imposta la risorsa a `value` invece di sommare) e l'uso
  del nuovo campo `chain_count` in `EffectSpec`.
- File toccati: [EffectType.hpp](../gmRules/effect/EffectType.hpp),
  [EffectSpec.hpp](../gmRules/effect/EffectSpec.hpp),
  [EffectResolver.cpp](../gmRules/effect/EffectResolver.cpp),
  [RuleBookLoader.cpp](../gmRules/loader/RuleBookLoader.cpp).

---

## 3. Capacità di creare/eliminare Regole add-on a runtime

Domanda dell'utente (2026-06-27): le regole possono solo aggiungersi, oppure si
possono anche **eliminare** e **sostituire** a runtime?

### 3.1 Decisione

Implementata la gestione **completa del ciclo di vita** delle regole a runtime,
con queste API su `RuleBook` (e façade gemella su `gmRulesEngine`):

| API | Semantica |
|---|---|
| `register_rule(def)` / `load_json()` | Aggiunge regole (anche da JSON, in qualsiasi momento) |
| `remove_rule(rule_id)` | Rimuove una regola; **no-op** se non esiste (ritorna `false`) |
| `replace_rule(def)` | Sostituisce: equivale a `remove_rule` + `register_rule`; se non esiste, semplicemente aggiunge |
| `clear_rules()` | Svuota tutto il RuleBook — utile per **hot-reload** completo |

- La **sostituzione** richiesta dall'utente (Q4) è coperta da `replace_rule()`
  come shortcut, oltre alla sequenza manuale remove + re-add.
- Le regole sono **immediatamente operative** dopo il caricamento (avvio, dopo un
  hot-reload, o su richiesta della GUI).

### 3.2 Artefatti

- [gmRules/core/RuleBook.hpp](../gmRules/core/RuleBook.hpp) /
  [RuleBook.cpp](../gmRules/core/RuleBook.cpp)
- [gmRules/facade/gmRulesEngine.hpp](../gmRules/facade/gmRulesEngine.hpp) /
  [gmRulesEngine.cpp](../gmRules/facade/gmRulesEngine.cpp)

---

## 4. Decisioni sul Gioco generico Dungeon Crawler

### 4.1 Architettura di base

- **Principio cardine:** *il CoreEngine comanda SEMPRE, la GUI SOLO visualizza.*
  La GUI **non deve mai** prendere decisioni di logica di gioco.
- Architettura **due processi** con comunicazione TCP (es. Dungeon Crawler Basic:
  GUI server eventi su 9200, CoreEngine comandi su 9201; Eldhôm su 9210/9211).
- La GUI **riusa i widget/moduli generici** della libreria Python `gmGui`
  (Deck Manager, Actor, Map, Flow, ecc.) invece di reimplementarli.

### 4.2 Carte con regole additive / Deckbuilding

- Per il Dungeon Crawler si usano **Carte con Rule integrata** (vedi §2.3):
  file JSON descrivono le Carte con le loro azioni specifiche; le Rule vengono
  **generate davvero** dalle Carte e gli effetti **applicati agli Actors**.
- Regola sulle Azioni (decisa e poi raffinata):
  - Le azioni del Giocatore **non possono mai andare sotto zero**.
  - Il check corretto è: **se `(actions - action_cost) < 0`** allora avvisa con
    MessageBox "Hai finito le Azioni: continua comunque?" — **non** quando il
    risultato è esattamente 0.
  - Carte a **costo 0** (es. monete Gold/Silver/Copper: `coins` +3/+2/+1) si
    possono sempre giocare anche con `actions = 0`.
  - **Autorità sul costo azione = CoreEngine.** Il controllo client-side nel deck
    module è disattivabile via `set_enforce_action_cost(False)` (vedi §4.4).

### 4.3 Riuso libreria vs sviluppo ad hoc

Analisi consolidata: **~75% della logica di un dungeon crawler/deckbuilder è già
coperta dalle gmXxx**. Meccaniche promosse a libreria generica (file
[plan_gm_lib.md](../plan_gm_lib.md), feature F1–F5):

| Feature | Libreria | Componente | Stato |
|---|---|---|---|
| F1 — CardType + SequenceEngine | `gmAlea` | macchina a stati sequenze carte (SINGLE/SEQ_START/SEQ_CONTINUE/SEQ_END/INSTANT) | 29/29 test |
| F2 — TimelineMilestoneSystem | `gmFlow` | trigger temporali a soglia | 19/19 test |
| F3 — FormationValidator + Resolver | `gmActor` | regola `RG ≤ PL`, criteri prioritizzati (HP/timeline/dado) | 35/35 test |
| F4 — BehaviorCardProcessor | `gmActor` | turno gruppo mostri, reazioni, fallback | 20/20 test |
| F5 — 4 widget PySide6 generici | `gmGui` | Timeline / Formation / SequenceState / BehaviorCard | sintassi OK |

Criterio per promuovere una meccanica a libreria: (1) appare in più tipi di gioco,
(2) non contiene concetti di dominio, (3) è parametrizzabile da dati/policy esterni.

### 4.4 Bug storico risolto — "Azioni esaurite" (Bug_01)

- **Causa radice finale:** nel modulo condiviso
  [pyLib/gmGui/modules/gm_comp_deck_module.py](../pyLib/gmGui/modules/gm_comp_deck_module.py)
  il flag `_enforce_action_cost` veniva inizializzato in `_build_widget()` (build
  lazy del widget), che girava **dopo** `set_enforce_action_cost(False)` e ne
  annullava l'effetto, facendo ricomparire il pop-up.
- **Fix:** preservare il valore già impostato →
  `self._enforce_action_cost = getattr(self, "_enforce_action_cost", True)`.
- **Prova di innocenza del CoreEngine:** test di isolamento
  [test_coreengine_bug01.py](../GAME/Dungeon-Crawler-Basic/tools/test_coreengine_bug01.py)
  dimostra che il motore accetta una carta costo-2 con 2/2 azioni
  (`attack.declared` + `defense.window.opened`, nessun `action.rejected`).
- **Regola di reach attacco** (DungeonRuleAdapter): bersaglio valido se nella
  **stessa stanza o in stanza adiacente**.

---

## 5. gmGui (PySide6) — Decisioni operative emerse nel thread

Questa sezione raccoglie le decisioni più pratiche prese durante il refactor
UI/bridge della libreria `gmGui`, utili perché hanno impatto diretto sul modo
in cui si sviluppano sandbox, test e giochi reali.

### 5.1 Principi architetturali confermati

- La libreria `gmGui` resta **headless-safe**: test in offscreen (`QT_QPA_PLATFORM=offscreen`)
  sono obbligatori, ma non sostituiscono la validazione visuale reale.
- Tema grafico centralizzato via `ThemeManager`: nessun hardcode locale di
  colori/font nelle view dei moduli.
- Bridge GUI/Core: protocollo TCP frame length-prefixed condiviso (`4-byte big-endian + UTF-8 JSON`).

### 5.2 Sandbox gmGui — decisioni su porte e robustezza bridge

#### Problema reale riscontrato

- Stato `Engine: Disconnesso` con mock apparentemente avviato.
- Root cause: conflitto sulla porta `9000` già occupata da processo esterno.

#### Decisioni/fix

- La sandbox usa porte dedicate:
  - eventi: `19000`
  - comandi: `19001`
- `MainWindow` legge porte bridge da env var:
  - `GMGUI_EVENT_PORT`
  - `GMGUI_COMMAND_PORT`
- Script sandbox aggiornato per passare entrambe le porte.
- Mock engine in modalità manuale per i turni (niente avanzamento automatico).
- Fix cruciale: il timeout del canale comandi nel mock **non** va trattato come
  disconnessione; è idle normale.

#### Artefatti Flow

- [pyLib/gmGui/main_window.py](../pyLib/gmGui/main_window.py)
- [GAME/gmGui-Sandbox/run_gmgui_sandbox.bat](../GAME/gmGui-Sandbox/run_gmgui_sandbox.bat)
- [GAME/gmGui-Sandbox/mock_engine.py](../GAME/gmGui-Sandbox/mock_engine.py)

### 5.3 Modulo Flow/Timeline — compattazione e controllo turno

#### Decisioni UI

- Obiettivo: dock Flow molto compatto, senza overlap tra badge e timeline.
- Geometrie ridotte e bilanciate (badge + timeline + spacing) per evitare
  intersezioni reali tra widget.
- Toggle log eventi icon-only, coerente con UI minimal.

#### Decisioni funzionali

- Aggiunto bottone `Passa Turno` in `Flow / Timeline`.
- Semantica sandbox: il turno avanza **solo** su comando `gmFlow.turn.pass`.

#### Artefatti Deck

- [pyLib/gmGui/modules/gm_flow_module.py](../pyLib/gmGui/modules/gm_flow_module.py)

### 5.4 Modulo Deck Manager — schema visuale e regole di movimento

#### Decisioni di layout

- Layout aderente al mock funzionale richiesto:
  - aree principali: `Dettaglio Carta`, `Giocate`, `Memoria`, `Scarti`,
    `Mazzo`, `Mano`, `Eliminate`, `Non in Uso`.
- Pulsanti collocati dentro i box zona (sotto il contatore carte).
- Rimosse etichette helper ridondanti (`Stati persistenti`,
  `Usa "Gestisci Mazzo" ...`) su richiesta.

#### Decisioni funzionali su pulsanti

- Regola chiave: i pulsanti devono rappresentare **spostamenti reali di zona**.
- Bottoni agganciati:
  - Mazzo → Mano:
    - `Pesca la Prima` (nascosto)
    - `Scegli` (osservato)
  - Scarti → Mano:
    - bottone unico dinamico:
      - `Prendi la Prima` (nascosto)
      - `Scegli` (osservato)
    - `Rimescola`: Scarti → Mazzo (shuffle)
  - Mano → `Scarti`, `Giocate`, `Memoria`, `Eliminate`
  - Giocate → `Scarti` (`Scarta`) o `Mano` (`Riprendi`)
  - Memoria → `Mano` (`Riprendi`)
- `Gestisci Mazzo` resta separato/secondario (non parte della logica base di
  spostamento carta nel thread corrente).

#### Decisioni su drag-and-drop (matrice regole)

- Da `MainDeck` drag consentito solo verso `CardHand`.
  - nascosto: solo prima carta
  - osservato: qualsiasi carta
- Da `DiscardPile` drag consentito solo verso `CardHand` o `MainDeck`.
  - nascosto: solo prima carta
  - osservato: qualsiasi carta
- Da `CardHand` drag consentito solo verso target equivalenti ai pulsanti Mano:
  `DiscardPile`, `PlayArea`, `Memory`, `BanishZone`.
- `DiscardPile`: prima carta sempre visibile anche in modalità nascosta.
- Inserimento in ogni zona: sempre **on top** (stack semantics), non in coda.

#### Artefatti

- [pyLib/gmGui/modules/gm_comp_deck_module.py](../pyLib/gmGui/modules/gm_comp_deck_module.py)
- [pyLib/gmGui/tests/test_gm_comp_deck.py](../pyLib/gmGui/tests/test_gm_comp_deck.py)

### 5.5 Mock engine sandbox — estensioni deck per test realistico UI

- Il mock non gestisce solo flow/dice: ora gestisce anche comandi deck
  (`move_card`, `draw`, `recycle_discard`) e invia eventi coerenti lato GUI.
- Decisione importante: anche nel mock i movimenti usano semantica a pila
  (inserimento top) per allinearsi al comportamento atteso in UI.

#### Artefatto

- [GAME/gmGui-Sandbox/mock_engine.py](../GAME/gmGui-Sandbox/mock_engine.py)

### 5.6 Lezioni consolidate dal thread

- I test offscreen passati non garantiscono correttezza visuale: serve sempre un
  giro su sandbox reale quando si lavora su geometrie/layout.
- Per bug bridge prima verificare le porte reali occupate (`netstat`) prima di
  toccare logica di protocollo.
- In GUI stateful con widget lazy-build, inizializzazioni in `_build_widget()`
  possono sovrascrivere configurazioni già impostate: usare pattern difensivi
  (`getattr(..., default)`) sui flag sensibili.

---

## 6. Eldhôm — integrazione reale CoreEngine/GUI (decisioni operative)

Questa parte non è teorica: riassume il debug effettivo del bridge Eldhôm in
Windows, con sessioni live su terminali separati.

- Pattern definitivo a 2 canali TCP:
  - **9210**: GUI server eventi (`EngineReceiver`), CoreEngine client eventi.
  - **9211**: CoreEngine server comandi (`EldhomCmdServer`), GUI client comandi.
- Wire contract confermato e congelato:
  - frame = prefisso lunghezza 4 byte big-endian + payload JSON UTF-8.
- Autorità dati confermata:
  - CoreEngine produce eventi (`eldhom.state.full`, `eldhom.turn.next_actor`, ...).
  - GUI invia comandi (`eldhom.start_mission`, `eldhom.play_card`, ...).
- Scelta pratica per troubleshooting su Windows:
  - launcher batch con finestre separate visibili (no esecuzione silenziosa).

## 7. Incident log Eldhôm — problemi reali emersi e decisioni finali

### 6.1 Incidente A — GUI "Non connesso" dopo scelta missione

- Sintomo: la GUI restava in attesa infinita dopo selezione missione.
- Prima causa trovata: race di startup (server GUI non sempre pronto quando il
  CoreEngine tentava la connessione eventi).
- Decisione: aumento attesa nel launcher (`run_eldhom.bat`) a 5 secondi.

### 6.2 Incidente B — comunicazione asimmetrica (comandi ok, eventi no)

- Sintomo osservato: GUI -> Engine funzionava, Engine -> GUI no.
- Evidenza raccolta:
  - Engine riceveva `eldhom.start_mission`.
  - Engine costruiva e inviava `eldhom.state.full` senza eccezioni di send.
  - GUI non stampava alcun evento ricevuto.
- Root cause finale:
  - in `GAME/Eldhom/GUI/app/eldhom_bridge.py`, `set_on_event()` impostava
    `self._receiver.on_message = handler`, ma `EngineReceiver` usa il segnale Qt
    `envelope_received` e non legge `on_message`.
- Fix definitivo:
  - connessione segnale-slot corretta:
    `self._receiver.envelope_received.connect(handler)`.
- Risultato verificato:
  - GUI riceve `eldhom.state.full` e `eldhom.turn.next_actor`.

### 6.3 Incidente C — crash immediato GUI / warning QThread

- Sintomo: `QThread: Destroyed while thread '' is still running`.
- Causa reale nel thread:
  - durante una modifica temporanea era stato rimosso dal `main.py` il
    `window.show()` + `return app.exec()`, facendo terminare il processo mentre
    il receiver thread era ancora attivo.
- Decisione correttiva:
  - ripristinare il ciclo Qt standard nel main.
  - mantenere shutdown esplicito del receiver in `closeEvent()`.

### 6.4 Incidente D — encoding console cp1252 durante test automation

- Sintomo: `UnicodeEncodeError` su print con emoji in output rediretto.
- Decisione:
  - per test rediretti su file in Windows usare `PYTHONIOENCODING=utf-8`.
  - evitare dipendenza da emoji nei log di diagnostica critica.

## 8. Contratti e convenzioni stabilizzati (da non rompere)

- Contratto evento/comando Eldhôm:
  - `typeId` string + payload in `headers["data"]` (JSON serializzato) lato
    envelope C++.
  - lato GUI il receiver normalizza su `msg["data"]`.
- Separazione responsabilità:
  - CoreEngine non dipende da widget o stato GUI.
  - GUI non implementa regole di dominio del combattimento/flow.
- Compatibilità del bridge:
  - framing identico tra C++ `IpSocketChannel` e Python `framing.py`.

## 9. Cronologia decisionale estesa (2026-06-16 -> 2026-06-28)

### 8.1 Fase 1 — Consolidamento librerie generiche

- Confermato pacchetto di meccaniche riusabili (F1-F5) per ridurre codice ad hoc.
- Stabilito criterio di promozione a libreria: meccanica multi-gioco,
  non domain-specific, parametrizzabile da dati/policy.

### 8.2 Fase 2 — gmRules/GRS da descrizione a esecuzione runtime

- Chiusa la separazione `RuleGroupRegistry` (attivazione) vs `RuleBook`
  (risoluzione/esecuzione).
- Introdotta gestione completa ciclo vita regole add-on runtime
  (add/remove/replace/clear).

### 8.3 Fase 3 — Integrazione Dungeon Crawler + bug economy azioni

- Formalizzata la regola: consentire gioco carta se risultato azioni è zero,
  bloccare/avvisare solo quando andrebbe sotto zero.
- Dimostrata innocenza CoreEngine con test isolato; fix applicato sul modulo GUI
  condiviso (ordine init flag build lazy).

### 8.4 Fase 4 — Integrazione Eldhôm cross-process reale

- Definiti e verificati i due canali TCP separati.
- Risolta race startup, poi risolto wiring eventi Qt, poi ripristinato event loop
  GUI completo.
- Stabilizzati i contratti di comunicazione e il metodo di debug operativo.

## 10. Decisioni "da tramandare" (alta priorità)

- DP-01: Timeline continua senza round è il modello di riferimento per i giochi
  tactical/deckbuilding su `game_lib`.
- DP-02: Rule engine orientato a dati; il codice C++ non deve essere il punto di
  authoring principale delle regole di gioco.
- DP-03: Regole add-on runtime sono feature core, non eccezione.
- DP-04: CoreEngine è unica source of truth per stato e validazioni.
- DP-05: GUI deve restare event-driven e non duplicare logica dominio.
- DP-06: In debug networking, prima verificare wiring e lifecycle thread/process,
  poi indagare protocollo.

## 11. Rischi residui e policy consigliate

### 10.1 Rischi residui

- Collisione `RuleId` in hot-reload multipacchetto add-on.
- Drift tra capability GRS dichiarate e capability effettive del runtime adapter.
- Regressioni su startup timing in ambienti lenti/CI Windows.

### 10.2 Policy consigliate

- Definire naming/versioning policy per RuleId add-on (`namespace.modulo.rule`).
- Aggiungere smoke test automatico end-to-end GUI<->Engine su almeno:
  `start_mission`, `state.full`, `turn.next_actor`.
- Tenere nel launcher una finestra di startup conservativa configurabile.

## 12. Checklist di ripartenza per un nuovo thread

Quando si apre un nuovo thread su questi temi, partire da questa checklist:

1. Confermare obiettivo: libreria generica vs dominio gioco.
2. Se si toccano regole: indicare se è change di authoring, loader o runtime.
3. Se si tocca bridge GUI/Core: indicare subito i due canali (porta/ruolo).
4. Verificare che le modifiche restino additive/backward-compatible.
5. Allegare sempre evidenza minima di verifica (test o log marker).

---

## 13. Stato aperto / prossimi passi

- gmFlow_API.md: aggiungere la sezione "Timeline Milestones (F2)".
- gmGui: 4/6 widget generici pronti (mancano 2 dei previsti in F5).
- Eldhôm (gioco di prova reale che valida le feature generiche): CoreEngine C++
  P6/P7 completato; GUI in consolidamento con focus su stabilità E2E.

---

## 14. Riferimenti rapidi

- Piano feature generiche: [plan_gm_lib.md](../plan_gm_lib.md)
- Manuale generale librerie: [Game-Lib_readme.md](../Game-Lib_readme.md)
- API regole + tabella EffectType: [gmRules/gmRules_API.md](../gmRules/gmRules_API.md)
- Piano Eldhôm: [GAME/Eldhom/info/PLAN.md](../GAME/Eldhom/info/PLAN.md)
- Regole di stile/architettura: cartella `.github`
