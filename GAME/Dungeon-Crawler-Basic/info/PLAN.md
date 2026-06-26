# gmDungeonBasic – Development Plan

**Version:** 0.9.0
**Status:** Phase 4 – Attack & Reactive Defense – Planned ⏳
**Language:** C++17 Standard
**Namespace:** `gmDungeonBasic`

---

## Goal

Realizzare Dungeon Crawler Basic come un gioco completo a due processi separati: un CoreEngine in C++17 che usa il più possibile le Game Lib esistenti per regole, attori, mappa, turni, eventi, log e persistenza, e una GUI in Python con PySide6 che gestisce visualizzazione, input e feedback dell'utente. Il dominio iniziale include Hero, Monster semplici, Monster Elite e BossMonster, con stato coerente tra Core e GUI. La scelta di base è una architettura a composizione con facade centrale nel Core, così ogni sottosistema resta isolato in una propria cartella e il contratto tra Core e GUI rimane esplicito, testabile e congelabile. Le tre specifiche in gmRules sono la source of truth per il gameplay, quindi il piano deve rispettarle in modo rigido e chiedere approvazione esplicita se emergono incongruenze o refusi da normalizzare.

### Confronto pattern valutati

| Pattern | Pro | Contro | Scelta |
|---|---|---|---|
| Singleton service | Accesso globale semplice | Stato globale, test difficili | ❌ |
| Monolite unico | Più rapido da prototipare | Accoppiamento alto, GUI e Core non separati | ❌ |
| Facade + composizione separata | Ownership chiara, testabile, riuso massimo delle lib | Più boilerplate iniziale | ✅ |

---

## Architecture

```text
┌──────────────────────────── Processo GUI (PySide6) ────────────────────────────┐
│  DungeonMainWindow                                                             │
│    ├─ Toolbar / comandi partita                                                │
│    ├─ Board / Dungeon view              ← eventi da gmMap + gmRules           │
│    ├─ Actor panel / hero, monster, boss  ← eventi da gmActor + gmFlow         │
│    ├─ Action panel / azioni v1           ← Move, Heal, Equip                  │
│    ├─ Log panel / messaggi              ← eventi da gmLog + gmDispatch       │
│    └─ Error / feedback bar              ← eventi di validazione               │
│  engine_bridge: receiver eventi + sender comandi                             │
└───────────────▲───────────────────────────────────────────────┬────────────────┘
        eventi   │ frame JSON + prefisso lunghezza        comandi   │ frame JSON
┌───────────────┴───────────────────────────────────────────────▼────────────────┐
│  Processo CoreEngine (C++17)                                                     │
│  ┌──────────────────── DungeonEngine (Facade centrale) ──────────────────────┐  │
│  │ start_game()  handle_command()  advance_turn()  emit_events()              │  │
│  └──┬────────────┬────────────┬────────────┬────────────┬────────────┬────────┘  │
│     │            │            │            │            │            │           │
│  world/        actors/      flow/        rules/      actions/     log/   bridge/ │
│  DungeonMap    ActorRoster  TurnFlow     RuleAdapter  ActionV1     GameLog CmdSrv │
│  + JsonLoader  (gmActor)    (gmFlow)     (gmRules)    (gmRules)    (gmLog) (gmDispatch)
│  (gmMap+gmSave)                                                     + persistence gmSave │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## File Structure

```text
GAME/Dungeon-Crawler-Basic/                         ← nuovo gioco separato, source of truth del piano
├── PLAN.md                                         ← questo file
├── info/                                           ← specifiche e documentazione di dominio
│   ├── dungeon-crawler-basic.example_Diagram.md    ← grafo regole di esempio
│   ├── dungeon-crawler-basic.example.grs           ← specifica GRS testuale
│   ├── dungeon-crawler-basic.example.yaml          ← specifica machine-readable
│   └── dungeon-crawler-basic_implementation.md     ← note di integrazione e decisioni
├── CoreEngine/                                     ← processo C++17
│   ├── CMakeLists.txt                               ← target dell'eseguibile Core
│   ├── main.cpp                                     ← entry point CoreEngine
│   ├── engine/                                      ← facade centrale e tipi condivisi
│   │   ├── DungeonTypes.hpp                         ← command id, event id, enum stato
│   │   └── DungeonEngine.hpp/.cpp                  ← orchestrazione dei sottosistemi
│   ├── world/                                       ← mappa, dungeon, posizioni
│   │   ├── DungeonMap.hpp/.cpp                     ← uso di gmMap
│   │   └── DungeonMapLoader.hpp/.cpp               ← lettura JSON mappa via gmSave
│   ├── actors/                                      ← hero, monster, elite, boss e stati
│   │   └── ActorRoster.hpp/.cpp                     ← uso di gmActor
│   ├── flow/                                        ← turni, round, avanzamento partita
│   │   └── TurnFlow.hpp/.cpp                        ← uso di gmFlow
│   ├── rules/                                       ← parsing e adattamento delle regole GRS
│   │   └── DungeonRuleAdapter.hpp/.cpp              ← uso di gmRules
│   ├── actions/                                     ← azioni gameplay fase v1
│   │   └── ActionV1.hpp/.cpp                        ← Move, Heal, Equip
│   ├── log/                                         ← log strutturato e diagnostica partita
│   │   └── GameLog.hpp/.cpp                         ← uso di gmLog
│   └── bridge/                                      ← rete e protocollo verso la GUI
│       ├── GuiBridge.hpp/.cpp                       ← invio eventi verso GUI
│       └── CmdServer.hpp/.cpp                       ← ricezione comandi dalla GUI
└── GUI/                                             ← processo Python / PySide6
    ├── main.py                                       ← entry point applicazione GUI
    ├── app/
    │   ├── dungeon_main_window.py                    ← finestra principale
    │   ├── dungeon_bridge.py                         ← adapter lato GUI per engine_bridge
    │   └── event_router.py                           ← smistamento eventi verso widget
    └── widgets/
        ├── dungeon_board_widget.py                  ← vista mappa / dungeon cliccabile
        ├── hero_panel_widget.py                     ← stato personaggio e risorse
        ├── action_panel_widget.py                   ← azioni disponibili e target (v1)
        ├── log_widget.py                           ← log di gioco
        └── error_bar_widget.py                     ← messaggi di validazione e blocco
```

---

## Development Phases

### Phase 1 — Interfaces & Stubs (FASE A) ✅

- [x] Creare la struttura iniziale `CoreEngine/` e `GUI/` nel nuovo gioco.
- [x] Definire in header tutte le classi e le firme funzione per: `DungeonMap`, `DungeonMapLoader`, `ActorRoster`, `TurnFlow`, `DungeonRuleAdapter`, `ActionV1`, `GameLog`, `GuiBridge`, `CmdServer`.
- [x] Implementare body placeholder senza logica di business con marker obbligatorio `// ToBeImplemented //`.
- [x] Nei body placeholder, restituire `"Tutto ok"` dove il tipo di ritorno è stringa e default neutro coerente negli altri casi.
- [x] Scrivere descrizioni Doxygen complete per classi, metodi pubblici e parametri (input/output/errore atteso).
- [x] Generare il manuale d'uso in Markdown (API + wire contract) in `GAME/Dungeon-Crawler-Basic/info/`.
- [x] Definire e congelare il contratto v1 Core ↔ GUI (command/event IDs, payload minimi, errori standard).
- [x] **Smoke test:** CoreEngine compila con soli stub (`dungeon_engine.exe` generato); GUI shell si avvia in headless senza eccezioni Python.

**Notes:**
- Questa fase produce artefatti stabili per lavorare in parallelo: header, contratti, documentazione e skeleton compilabile.
- Nessuna logica reale deve entrare nei body in questa fase.
- Il manuale .md è parte del deliverable obbligatorio della fase.
- **Porte scelte:** eventi CoreEngine→GUI su **9200**, comandi GUI→CoreEngine su **9201** (evitano conflitti con Tris 9100/9001 e con la porta 9000 occupata da proxy Windows).
- Inizializzazione in DungeonEngine: `_rules` e `_actions` usano initializer list per passare i riferimenti a `_map` e `_actors` (necessario perché le classi hanno costruttori con parametri senza default).
- Smoke test CoreEngine: `dungeon_engine.exe` compilato e linkato senza errori. GUI shell avviata offscreen senza eccezioni Python.

### Phase 2 — Body Implementation (FASE B) ⏳

- [x] Implementare i body reali di `DungeonMap` e `DungeonMapLoader` con caricamento mappa JSON via `gmSave`.
- [x] Implementare `ActorRoster` con Hero, Monster, Monster Elite, BossMonster su `gmActor`.
- [x] Implementare `TurnFlow` con `gmFlow` e collegare stato partita.
- [x] Implementare `DungeonRuleAdapter` e `ActionV1` su `gmRules` solo per Move, Heal, Equip.
- [x] Implementare `GuiBridge`/`CmdServer` e routing eventi/comandi mantenendo invariato il contratto v1.
- [x] Completare GUI interattiva PySide6 in parallelo al Core, senza introdurre nuovi campi fuori contratto.
- [x] Integrare logging, test unitari, test E2E e registrazione CTest.
- [ ] Pianificare Attack/Defend come estensione successiva solo dopo approvazione contratti v2.
- [ ] **Smoke test:** partita completa v1 (Move, Heal, Equip) con Core + GUI in esecuzione reale e test PASS.

**Notes:**
- La GUI può iniziare sviluppo parallelo subito dopo il freeze della Phase 1.
- Ogni modifica al contratto congelato richiede approvazione esplicita.
- Attack e Defend restano fuori dallo scope di questa fase.
- Loader JSON: supporto risoluzione path primaria + fallback in `.cache/` per file mappa.
- RuleAdapter/ActionV1: validazione/effetti Move-Heal-Equip collegati a gmRules (`ConditionSpec` + `EffectResolver`).
- Bridge v1: `GuiBridge` invia envelope con `source="DungeonCore"` e `headers.data` serializzato; `CmdServer` decodifica frame TCP (4-byte big-endian + JSON UTF-8).
- GUI interattiva: board con stanze cliccabili e segnale move_requested; HeroPanel aggiorna HP/tag/status; ActionPanel abilita Heal/Equip in base ai tag hero.
- Logging: `GameLog` usa `gmLog::GmLogger` (stdout, INFO) via `logger().log(Level, msg)` per evitare risoluzione template lazy-eval.
- Test C++: `test_dungeon_map`, `test_actor_roster`, `test_turn_flow`, `test_dungeon_rules`, `test_map_loader` — tutti PASS.
- Test Python: `test_event_router.py`, `test_widgets.py` — tutti PASS in headless offscreen.
- CTest: tutti i test registrati in `CoreEngine/CMakeLists.txt`; test Python via `find_program(python)`.
- Mappa test: `.cache/maps/dungeon_01.json` (3 stanze, hero + 2 nemici, tag pozione/spada).

---

### Phase 3 — Shared GUI modules & area-info contract ⏳

- [ ] Definire un contratto messaggi **comune e riusabile** (typeId neutri `gmMap.*`/`gmMap.ui.*`) per la selezione area e la richiesta/risposta dei contenuti area, valido per qualunque game.
- [ ] Creare un nuovo modulo GUI comune `gm_map_area_info` in `pyLib/gmGui/modules/` che mostra in un unico widget due liste separate: attori nell'area selezionata e oggetti interagibili nell'area selezionata.
- [ ] Refactor di `gm_map_module`: il click su un'area **non** genera più azioni Actor; aggiorna solo lo stato visuale e invia una **richiesta dati** al Core (`gmMap.area.info.request`).
- [ ] Refactor di `gm_actor_module`: pannello dettagli attore **sempre visibile**; albero/lista attori **collassabile** con Mostra/Nascondi e **default nascosto** (inversione del comportamento attuale).
- [ ] Registrare il nuovo modulo e il routing in `pyLib/gmGui/main_window.py` (docking + tabella typeId) così ogni game ne beneficia senza codice game-specific.
- [ ] Implementare nel CoreEngine l'handler della richiesta area-info che produce `gmMap.area.info.response` aggregando dati da `gmMap` (interagibili/oggetti) e `gmActor` (attori nell'area), **senza side-effect di gameplay**.
- [ ] Test pyLib (layout/toggle attori, click=>request mappa, rendering area-info) e validazione headless.

**Notes:**
- Le modifiche sono nelle **librerie GUI comuni** (`pyLib/gmGui`) e nel contratto messaggi: nessuna logica è specifica del Dungeon e si riflette su tutti i game che riusano `MainWindow`/moduli.
- Contratto area-info (riusabile):
  - `gmMap.area.info.request` (GUI → Core): `{ "area_id": str, "request_id"?: str }`
  - `gmMap.area.info.response` (Core → GUI): `{ "area_id": str, "actors": [...], "interactables": [...], "request_id"?: str }`
  - `gmMap.ui.area_selected` (evento GUI interno opzionale): `{ "area_id": str }`
- Principio: ogni interazione utente sulla mappa cambia solo visualizzazioni (proprio widget o altri widget), mai azioni Actor dirette.
- Migrazione compatibile: i typeId esistenti restano; si aggiungono solo nuovi messaggi.

---

### Phase 4 — Attack & Reactive Defense (Core combat) ⏳

> Modella l'azione Attacco (base + carte) e la **finestra reattiva di Difesa** del bersaglio.
> Il mattone reattivo è `gmFlow::ActionWindow` con `CompletionPolicy::ANY_SUBMITTED`
> (documentata come *Reaction window*). Poiché Core e GUI sono due processi, la
> ActionWindow vive nel Core come **macchina a stati di reazione** e viene proiettata
> sulla GUI tramite nuovi messaggi del wire contract.

- [ ] Estendere `ActorInfo` (e il loader `dungeon_*.json` + snapshot attori) con la statistica base `attack` (e `defense` opzionale), così il danno = `attack` attaccante + modificatore della carta.
- [ ] Aggiungere a `DungeonTypes` i nuovi command id (`dungeon.attack`, `dungeon.defend`, `dungeon.defend.pass`) e gli event id (`dungeon.attack.declared`, `dungeon.defense.window.opened`, `dungeon.defense.window.closed`, `dungeon.attack.resolved`).
- [ ] Implementare nel Core la **macchina a stati di reazione** `AWAITING_DEFENSE` in `DungeonEngine`: memorizza il contesto attacco pendente (attaccante, difensore, danno base, modificatore carta, sorgente) e blocca altre azioni finché la difesa non è risolta o passata.
- [ ] Aggiungere `ActionV1::execute_attack` (azione base) e integrare le carte Attacco già presenti (`colpo_efficace`, `pugno_di_ferro`, …) come sorgente di danno tramite `DungeonRuleAdapter::execute_card`.
- [ ] Estendere `DungeonRuleAdapter` con `can_attack` e la **risoluzione difesa**: carta di difesa reattiva (`parata` → stato `difeso`), carta/risorsa permanente (scudo `scudo_antico` con cariche), consumabile (pozione), con due esiti generali: **riduzione danni** o **annullamento totale**.
- [ ] Implementare il calcolo finale del danno (`danno_finale = max(0, base + mod_carta − riduzione_difesa)`; `0` se annullato), applicare HP e decrementare cariche/consumare risorse e status `difeso`.
- [ ] Emettere gli eventi del flusso reattivo: `attack.declared` → `defense.window.opened` (con `defender_id`, opzioni disponibili) → attesa `dungeon.defend`/`dungeon.defend.pass` → `attack.resolved` + `dungeon.actor.hp_changed` + `defense.window.closed`.
- [ ] Gestire il **pass esplicito** (`dungeon.defend.pass`): anche senza opzioni di difesa il danno pieno si applica solo dopo conferma dalla GUI; nessun timeout (gmFlow V1 non lo prevede).
- [ ] Loggare attacco, finestra di difesa, scelta del difensore ed esito via `GameLog`.
- [ ] Documentare i nuovi messaggi in `info/wire-contract-v1.md` (additivo, senza rompere il contratto esistente).
- [ ] **Smoke test (C++):** unit test combat — attacco con riduzione (HP cala di `base+mod−riduzione`), annullamento totale (0 danni), cariche scudo che decrementano, `pass` = danno pieno; tutti PASS.

**Notes:**
- Sorgenti di difesa (da risposte utente): carta di difesa **reattiva o permanente** + risorsa **permanente (scudo)** o **consumabile (pozione)**; il difensore può ridurre o annullare.
- Sorgente danno: **statistica base attaccante + modificatore carta**.
- Tutti i bersagli (anche i Mostri) aprono la finestra interattiva: la scelta è del GM/utente, **nessuna AI** di difesa in questa fase.
- La `ActionWindow(ANY_SUBMITTED)` è il modello concettuale; nel Core a 2 processi è realizzata come stato `AWAITING_DEFENSE` + messaggi wire, perché la decisione arriva in modo asincrono dalla GUI.
- Le carte Attacco/Difesa e le relative regole gmRules **esistono già** in `data/cards_dungeon.json`; questa fase aggiunge solo il flusso reattivo e la statistica `attack`.

---

### Phase 5 — Reactive Defense GUI integration ⏳

> Riusa i pannelli esistenti: durante la reazione l'Actor **selezionato** diventa il
> **Difensore**, quindi `ActionPanel` + `DeckManagerPanel` mostrano le sue carte, le
> sue risorse e le sue azioni di difesa. Nessun nuovo widget dedicato.

- [ ] Aggiungere alla GUI i sender dei nuovi comandi: `dungeon.attack`, `dungeon.defend`, `dungeon.defend.pass`.
- [ ] Registrare nel router GUI gli handler dei nuovi eventi (`attack.declared`, `defense.window.opened/closed`, `attack.resolved`).
- [ ] Implementare la **modalità difesa**: alla ricezione di `defense.window.opened` la GUI imposta l'Actor selezionato = `defender_id`, mostra in `ActionPanel`/`DeckManagerPanel` le opzioni di difesa (carta/risorsa) + il pulsante **Pass**, e blocca le altre azioni finché la finestra è aperta.
- [ ] Ripristinare lo stato normale alla ricezione di `defense.window.closed`/`attack.resolved` (riseleziona l'Actor di turno, sblocca azioni) e mostrare il feedback di esito (HP aggiornati, log).
- [ ] **Smoke test (Python + E2E):** test router/ widget per la modalità difesa (selezione difensore, opzioni mostrate, pass funzionante) e partita E2E reale `attacco → finestra difesa → difendi/pass → risolto` con Core+GUI; tutti i test PASS.

**Notes:**
- Riuso totale di `ActionPanel` e `gm_comp_deck_module`/`DeckManagerPanel`: il cambio di "Actor selezionato" è l'unico meccanismo che fa apparire carte/risorse/azioni del difensore.
- La finestra reattiva è **bloccante a livello UX** ma non modale a livello tecnico: si basa sullo stato `AWAITING_DEFENSE` del Core, unica source of truth.
- Compatibilità: i typeId v1 restano invariati; la GUI ignora i nuovi eventi se non in modalità difesa.

---

## Key Design Decisions

1. **Due processi separati** per CoreEngine e GUI, così il gameplay resta testabile anche senza interfaccia grafica.
2. **Facade centrale nel Core** per coordinare i sottosistemi e mantenere i moduli delle Game Lib indipendenti.
3. **Source of truth rigida** sulle specifiche GRS del Dungeon Crawler Basic, con escalation manuale se servono normalizzazioni.
4. **Mappa da JSON fin dall'inizio**: `gmMap` alimentata tramite loader basato su `gmSave` già nelle fondazioni Core.
5. **Scope funzionale v1**: solo Move, Heal, Equip; Attack e Defend in backlog futuro.
6. **Split di delivery in due fasi**: FASE A (header/stub/Doxygen/manuale/contratto), FASE B (implementazione completa body).
7. **Parallelizzazione controllata**: GUI e Core lavorano in parallelo dalla FASE B dopo freeze del contratto v1 in FASE A.
8. **Difesa reattiva modellata su `gmFlow::ActionWindow(ANY_SUBMITTED)`**: nel Core a 2 processi diventa lo stato `AWAITING_DEFENSE` proiettato sulla GUI via wire contract; unica source of truth nel Core.
9. **Combattimento componibile**: danno = statistica base attaccante + modificatore carta; difesa = carta (reattiva/permanente) + risorsa (scudo/pozione), con esito riduzione **o** annullamento; tutti gli Actor (anche Mostri) difesi interattivamente dal GM, senza AI.
