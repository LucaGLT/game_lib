# Le Pergamene di Eldhôm — WebApp Development Plan

**Version:** 0.2.0
**Status:** Phase 1 – Completato ✅ (scaffold + smoke test validati, vedi Esito sotto)
**Language:** Python 3.11+ (FastAPI) + TypeScript 6 / React 19 (frontend) — polyglot web layer.
Il CoreEngine C++17 esistente (`eldhom_engine.exe`, vedi `../info/PLAN.md`) è **invariato**.
**Namespace:** N/A (no C++ namespace) — pacchetto Python `eng_serve`, app frontend `webapp_frontend`.
Riusa il wire-contract `eldhom.*` (typeId/payload) invariato.

---

## Risposta diretta

**No**, Eldhôm non è ancora apribile come WebApp: oggi esiste solo la GUI desktop
PySide6 (`GAME/Eldhom/GUI`), che parla TCP diretto con `eldhom_engine.exe` sulle
porte 9210 (eventi) / 9211 (comandi). Non esiste alcun `eng_serve`/`webapp_frontend`
per Eldhôm (a differenza di `GAME/Tic-Tac-Toe/WebApp`, già completo per Phase 1+3).

Questo file pianifica il lavoro mancante, riusando il più possibile ciò che è
già stato costruito per Tic-Tac-Toe (pattern `eng_serve`, libreria condivisa
`webLib/WebGUI_Lib`) e ciò che Eldhôm stesso già condivide con Tris a livello di
infrastruttura (`pyLib/gmGui/engine_bridge`, 5 temi `theme_manager.py`).

**Vincolo esplicito per questa pianificazione:** si mantiene, per ora,
`MONO_USER_MANAGES_ALL_PLAYER` — un solo utente locale controlla tutti i PG
(2-5 eroi) della missione, esattamente come la GUI desktop oggi. Nessun
multi-sessione/autenticazione in questo piano (mirror della Phase 2
"Deliberately deferred" già decisa per Tris).

---

## Goal

Espone "Le Pergamene di Eldhôm" (`eldhom_engine.exe`) anche come **WebApp**, in
aggiunta — non in sostituzione — alla GUI desktop PySide6 esistente
(`GAME/Eldhom/GUI`), riusando **invariato** il wire-contract TCP (4-byte length
prefix + JSON) già parlato oggi dalla GUI. A differenza del pilota Tic-Tac-Toe
(dominio semplicissimo, 3×3, un solo tipo di mossa), Eldhôm è un dungeon crawler
completo: mappa con locazioni/adiacenze, Linea Temporale continua, mano di carte
con sequenze, formazioni Prima Linea/Retroguardia, gruppi mostri con Carte
Comportamento, missioni con soglie/obiettivi, e **3 dialog modali** desktop
(Formazione, Finestra Istantanee, Selezione Missione) che devono ottenere un
equivalente web con **le stesse identiche funzionalità**.

La buona notizia architetturale: il motore C++ tratta *già* le 3 dialog come
scambi **asincroni evento→comando** (non RPC bloccanti) — la GUI desktop le
rende sincrone solo per sua scelta locale (`QDialog.exec()`). Sul web basta
mostrare una modale quando arriva l'evento e inviare il comando alla conferma:
**nessuna modifica al protocollo/engine è necessaria per le dialog.**

### Confronto: backend `eng_serve` — clonare da Tris vs estrarre subito un toolkit condiviso

| Strategia | Pro | Contro | Scelta |
|---|---|---|---|
| Clonare `GAME/Tic-Tac-Toe/WebApp/eng_serve` e adattare (porte, `eldhom_engine.exe`, arg `data_dir`) | Minimo rischio, pattern già provato in Phase 1 Tris | Duplica codice quasi identico (`EngineEventListener`/`EngineProcess` sono già 100% game-agnostic) | ❌ |
| Estrarre ORA `pyLib/gmWebServe/` (`EngineEventListener` + `EngineProcess`, generalizzati) e farlo consumare da entrambi `eng_serve` | Mirror esatto della scelta già fatta per `webLib/WebGUI_Lib` sul frontend; il codice esistente è **già** privo di logica Tris-specifica (verificato leggendo `bridge_client.py`/`engine_process.py` di Tris) — costo di generalizzazione quasi nullo (solo `EngineProcess.start()` deve accettare `extra_args` opzionali per il `data_dir` di Eldhôm) | Un pacchetto Python condiviso in più da mantenere | ✅ |
| Riscrivere `eldhom_engine.exe` per parlare HTTP/WS nativamente in C++ | Nessun processo intermedio | Viola "engine C++ invariato"; alto rischio su un motore stabile (94/94 test) | ❌ |

**Nota:** a differenza della libreria frontend (dove `createSession`/tema/ecc.
richiedevano una vera generalizzazione), qui `EngineEventListener` (server TCP
Qt-free, parsing frame) ed `EngineProcess` (spawn/poll subprocess) sono **già**
scritti in modo interamente generico nel codice Tris esistente — solo
`settings.py` contiene valori specifici (percorso eseguibile, porte). Estrarli
ora è quindi a basso rischio e coerente con la scelta già fatta per
`webLib/WebGUI_Lib` ("estrai subito, non rimandare").

### Confronto: le 3 dialog — modale generica in `WebGUI_Lib` vs ad-hoc per Eldhôm

| Strategia | Pro | Contro | Scelta |
|---|---|---|---|
| Ogni dialog implementata come componente React indipendente (overlay/backdrop propri) | Zero impatto su `WebGUI_Lib` | Triplica lo stesso boilerplate (backdrop, focus, chiusura); "modale" è un concetto generico, non di dominio | ❌ |
| Nuovo componente generico `Modal` in `webLib/WebGUI_Lib` (chrome/backdrop/focus) + 3 contenuti Eldhôm-specifici dentro | Coerente col principio già stabilito (generico vs specifico); riusabile da futuri giochi/dialog | Richiede toccare di nuovo `WebGUI_Lib` | ✅ |

---

## Architecture

```text
┌────────────────────────────── Browser (1 utente, tutti i PG) ───────────────────┐
│  React SPA (GAME/Eldhom/WebApp/webapp_frontend)                                 │
│    ├─ EldhomMap / TimelineTrack        (locazioni+adiacenze, Linea Temporale)    │
│    ├─ HandPanel / ActionPanel          (mano+sequenze, 4 azioni semplici)        │
│    ├─ HeroPanel× N / AreaInfoPanel     (2-5 PG, dettaglio locazione)             │
│    ├─ FormationModal / InstantWindowModal / MissionSelectModal (via Modal)       │
│    └─ da @webgui/*: temi, EnvelopeRouter, session client, ErrorBar/EventLog,     │
│       ActorStatusBadges, ThemeSelect, Modal (NUOVO)                             │
└───────────────▲───────────────────────────────────────────────┬─────────────────┘
       eventi    │ WebSocket (stesso typeId/payload eldhom.*)   comandi/query      │ REST (HTTP)
┌───────────────┴───────────────────────────────────────────────▼─────────────────┐
│  eng_serve — facciata HTTP/WS (FastAPI, NUOVO processo Python, mirror Tris)      │
│    ├─ routers/sessions.py : POST crea sessione / comando / GET missioni / WS     │
│    ├─ session_manager.py  : sessione singola "dev-session" (MONO_USER, Phase 2  │
│    │                        multi-sessione resta deliberatamente rimandata)      │
│    └─ (da pyLib/gmWebServe, condiviso con Tris) EngineEventListener+EngineProcess│
└───────────────▲───────────────────────────────────────────────┬─────────────────┘
       eventi    │ TCP 9210 (eng_serve = server, engine = client)  comandi         │ TCP 9211 (engine = server)
                 │        [protocollo INVARIATO — identico alla GUI desktop]       │
┌───────────────┴───────────────────────────────────────────────▼─────────────────┐
│  eldhom_engine.exe — C++17 INVARIATO (94/94 test)                                │
│  EldhomEngine + SequenceEngine/FormationEngine/BehaviorCardResolver/RuleAdapter  │
└───────────────────────────────────────────────────────────────────────────────────┘

Nota: la GUI PySide6 esistente (GAME/Eldhom/GUI) continua a funzionare INVARIATA,
connettendosi direttamente alle stesse porte 9210/9211 — coesistenza permanente,
come già per Tris. Le due GUI (desktop e web) non possono però restare connesse
allo STESSO processo motore contemporaneamente (porte hard-coded, un solo bind
possibile) — stesso limite già documentato e accettato per Tris Phase 1.
```

---

## File Structure

```text
game_lib/
├── pyLib/
│   └── gmWebServe/                  ← NUOVO: toolkit backend condiviso (mirror di webLib/WebGUI_Lib)
│       ├── engine_listener.py       ← EngineEventListener (da eng_serve/bridge_client.py di Tris, invariato)
│       ├── engine_process.py        ← EngineProcess (id., + parametro extra_args per il data_dir di Eldhôm)
│       └── tests/
├── webLib/WebGUI_Lib/src/
│   └── components/
│       └── Modal.tsx                ← NUOVO: chrome/backdrop/focus generico (nessun contenuto di dominio)
└── GAME/Eldhom/WebApp/               ← NUOVO
    ├── PLAN.md                       ← questo file
    ├── conftest.py                   ← rende eng_serve importabile per pytest (mirror Tris)
    ├── eng_serve/
    │   ├── main.py                   ← FastAPI app factory + lifespan
    │   ├── settings.py               ← eseguibile eldhom_engine.exe, porte 9210/9211, data_dir
    │   ├── session_manager.py        ← sessione singola "dev-session" (MONO_USER_MANAGES_ALL_PLAYER)
    │   ├── missions.py               ← NUOVO: scansione data/mission_*.json → GET /missions (ex MissionSelectDialog)
    │   ├── routers/
    │   │   └── sessions.py           ← REST comandi/query + GET missioni + WS eventi
    │   └── tests/
    │       └── test_session_e2e.py   ← E2E con eldhom_engine.exe reale (start_mission→state.full→simple_action)
    └── webapp_frontend/
        ├── package.json
        ├── tsconfig.app.json          ← alias @webgui/* (stesso schema di Tris, vedi Notes Phase 1)
        ├── vite.config.ts             ← alias @webgui + proxy dev verso eng_serve
        ├── src/
        │   ├── main.tsx / App.tsx     ← orchestratore pagina unica (routing multi-sessione: N/A, deferred)
        │   ├── engine/
        │   │   ├── contract.ts        ← typeId/payload eldhom.* (~40 eventi/comandi, vedi Phase 1 Notes)
        │   │   └── gameState.ts       ← reducer: mappa, eroi, gruppi mostri, timeline, mano, sequenza, formazione
        │   └── components/
        │       ├── EldhomMap.tsx          ← locazioni/adiacenze (incl. CLOSED_DOOR/LOCKED_DOOR), token attori
        │       ├── TimelineTrack.tsx       ← Linea Temporale (ordine attivazione, soglie missione)
        │       ├── HandPanel.tsx           ← mano carte, playability da SequenceEngine
        │       ├── ActionPanel.tsx         ← 4 azioni semplici + finestra reazione TAKE/BLOCK/DODGE (inline, non modale)
        │       ├── HeroPanel.tsx           ← scheda PG (HP/risorse/mano), 2-5 istanze
        │       ├── AreaInfoPanel.tsx       ← dettaglio locazione selezionata
        │       ├── FormationModal.tsx      ← contenuto Eldhôm-specifico dentro <Modal> (ex FormationDialog)
        │       ├── InstantWindowModal.tsx  ← id. (ex InstantWindowDialog, variante proattiva + reattiva)
        │       └── MissionSelectModal.tsx  ← id. (ex MissionSelectDialog, usa GET /missions)
        └── (tests/ dedicati Vitest+RTL: da popolare per ogni componente, vedi Phase 7)
```

---

## Development Phases

### Phase 1 — Interfaces & Stubs [✅ Completato]

- [x] Estrarre `pyLib/gmWebServe/` da `GAME/Tic-Tac-Toe/WebApp/eng_serve/bridge_client.py`
      + `engine_process.py` (vedi tabella comparativa sopra); aggiungere `extra_args: list[str] | None`
      a `EngineProcess.start()` (necessario per passare `data_dir` a `eldhom_engine.exe`, cosa che
      Tris non richiede); far ripuntare `GAME/Tic-Tac-Toe/WebApp/eng_serve` al pacchetto condiviso
      (nessuna regressione attesa: stesso codice, solo spostato) + rieseguire i test Tris esistenti.
- [x] Scaffold `GAME/Eldhom/WebApp/eng_serve/` (FastAPI, `requirements.txt`, `settings.py` con
      executable=`eldhom_engine.exe`, `event_port=9210`, `command_port=9211`, `data_dir` = `GAME/Eldhom/data`)
- [x] `session_manager.py`: sessione singola "dev-session" (mirror 1:1 Tris — nessuna modifica di design)
- [x] Endpoint REST minimo: `POST /sessions` (invia `eldhom.start_mission` con `mission_id`),
      `POST /sessions/{id}/command` (inoltra qualunque `eldhom.*` command 1:1, stesso pass-through
      typeId-agnostico già usato per Tris — **nessuna nuova logica per la ricchezza del contratto**,
      l'eng_serve non interpreta i payload)
- [x] Endpoint WebSocket minimo: `/sessions/{id}/ws`, inoltra 1:1 tutti gli eventi `eldhom.*`
- [x] `missions.py` + `GET /missions`: scansiona `data/mission_*.json` **lato server** (sostituisce
      la scansione locale su filesystem di `MissionSelectDialog`, che nel browser non è possibile)
- [x] Scaffold `webapp_frontend/` (Vite+React+TS) consumando **da subito** `@webgui/*`
      (a differenza di Tris, che lo ha adottato solo in un refactor successivo — qui è la prima
      vera validazione che `WebGUI_Lib` si generalizza a un secondo gioco molto diverso)
- [x] Pagina stub: mostra il JSON grezzo degli eventi ricevuti via WS (stesso approccio Phase 1 Tris)
- [x] Smoke test: sessione singola end-to-end — `eldhom.start_mission` → `eldhom.state.full` →
      un `eldhom.simple_action` (MOVE) → evento risultante (`eldhom.pg.moved` e
      `eldhom.action.result`) visibile nel browser

**Esito Phase 1 (validato):**

- `pyLib/gmWebServe/` estratto (`engine_listener.py`, `engine_process.py` con `extra_args`,
  `__init__.py`); Tris (`GAME/Tic-Tac-Toe/WebApp/eng_serve`) rifattorizzato per usarlo,
  `bridge_client.py`/`engine_process.py` locali rimossi — suite pytest Tris rieseguita:
  2 passed (nessuna regressione).
- `GAME/Eldhom/WebApp/eng_serve/` scaffoldato per intero (`settings.py`, `session_manager.py`,
  `missions.py`, `routers/sessions.py`, `main.py`, `requirements.txt`, `conftest.py`,
  `tests/test_session_e2e.py`) — suite pytest: 3 passed (`test_health`, `test_list_missions`,
  `test_single_session_end_to_end` con `eldhom_engine.exe` reale e `mission_sim_a.json`).
- Campi comando confermati da lettura diretta di `GAME/Eldhom/CoreEngine/main.cpp`:
  `eldhom.start_mission` → `{"mission_id"}`; `eldhom.simple_action` →
  `{"hero_id", "action_type" ("MOVE"/"ATTACK"/"INTERACT"/altro→RECOVER), "destination", "discard_ids"?}`.
  `MissionLoader::load_mission` mappa `mission_id` (campo nel JSON, es. `"missione_01"`) al file
  (`mission_01.json`) con una trasformazione propria — `missions.py` non la reimplementa: legge
  `mission_id`/`title`/`description` dal contenuto di ciascun file, non dal nome del file.
- `GAME/Eldhom/WebApp/webapp_frontend/` scaffoldato (Vite+React+TS, alias `@webgui/*` identico a
  Tris) con pagina stub (`src/App.tsx` + `src/engine/contract.ts`) — `npm run build` e
  `npm run lint` puliti.
- **Smoke test end-to-end validato nel browser reale** (missione `missione_01`, non
  `mission_sim_a` — scelta per comodità del test manuale, copertura equivalente): avviata
  la missione dalla UI, ricevuti via WS `eldhom.deck.reshuffled`/`hand_updated` (x2 eroi),
  `eldhom.state.full` (mappa/eroi/gruppi corretti), `eldhom.turn.next_actor`; inviata
  un'azione MOVE (thael → "corridoio"), ricevuti `eldhom.zone_door.opened`, `eldhom.pg.moved`,
  `eldhom.mission.time_advanced`, `eldhom.action.result` (`{"ok": true}`),
  `eldhom.turn.next_actor` (turno di velyr), nuovo `eldhom.state.full` con thael a "corridoio".
  Pipeline browser ↔ eng_serve ↔ `eldhom_engine.exe` confermata funzionante al 100%.
- `mission_sim_a.json` (scelta dall'utente per il test Phase 1) resta la missione di
  riferimento per la suite pytest automatica; il test manuale nel browser ha usato
  `missione_01` solo per semplicità di selezione nella dropdown — nessuna differenza di
  comportamento attesa o osservata tra le due missioni.

**Notes:**
- **Riuso confermato del wire format**: `EldhomBridge` (GUI desktop, `GAME/Eldhom/GUI/app/eldhom_bridge.py`)
  usa già `pyLib/gmGui/engine_bridge` (stesso framing 4-byte-prefix+JSON di Tris) — nessuna sorpresa
  di protocollo attesa.
- **Polarità porte DIVERSA da Tris ma già gestita da `EngineEventListener`**: in Eldhôm il lato
  "GUI" (qui: `eng_serve`) è il **server TCP per gli eventi** (porta 9210, l'engine si connette come
  client) e l'engine è **server per i comandi** (porta 9211). `EngineEventListener`/`EngineSender`
  esistenti già implementano esattamente questi due ruoli (sono gli stessi usati, con studio
  invertito, anche lato Tris) — nessuna nuova classe di trasporto necessaria, solo configurazione.
- **Porte hard-coded** (`GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp::ports::EVENTS/COMMANDS` =
  9210/9211, `constexpr`) — stesso limite già noto per Tris: `eng_serve` e la GUI desktop non
  possono restare connessi allo stesso processo motore contemporaneamente.
- **`data_dir` come argomento CLI obbligatorio**: a differenza di `tris_engine.exe` (nessun argv),
  `eldhom_engine.exe` richiede il percorso dati come primo argomento (vedi `run_eldhom.bat`) —
  motivo della generalizzazione `extra_args` in `EngineProcess`.
- **Contratto tipizzato molto più ampio di Tris**: ~11 eventi PG/sequenza, ~5 eventi mostro/gruppo,
  2 formazione + 2 dialog formazione, 4 attacco/reazione, 2 finestra istantanee, 2 mazzo/mano,
  3 missione (tempo/vittoria/sconfitta) + 4 oggetti speciali missione, 1 porta di zona, 1 stato
  pieno, 1 turno/prossimo attore, 1 risultato azione — e lato comandi: `start_mission`,
  `play_card`, `simple_action`, `stop_sequence`, `request_state`, `declare_attack`,
  `react_defense`, `play_instants`, `play_reactive_instants`, `resolve_formation`, 4 comandi
  mazzo GM. `engine/contract.ts` deve mappare tutte le stringhe da
  `GAME/Eldhom/CoreEngine/engine/EldhomTypes.hpp` (fonte di verità), non reinventarle.
- `eng_serve` resta **puro pass-through**, esattamente come per Tris: nessuna interpretazione di
  dominio nella facciata, tutta la logica sta nell'engine C++ e (per la sola presentazione) nel
  reducer frontend.

---

### Phase 2 — Multi-Session & Auth [⏸️ Deliberatamente rimandata]

- [ ] (invariato rispetto a Tris — vedi `GAME/Tic-Tac-Toe/WebApp/PLAN.md` Phase 2 per la checklist)

**Notes:**
- **Decisione esplicita dell'utente per questa pianificazione (2026-07-17):** si mantiene
  `MONO_USER_MANAGES_ALL_PLAYER` — un solo utente locale controlla tutti i 2-5 PG della
  missione, esattamente come la GUI desktop oggi. Questa fase riparte da qui solo quando/se
  si formerà l'esigenza multi-utente (stessa logica già applicata a Tris).

---

### Phase 3 — Mappa & Linea Temporale [⏳]

- [ ] `EldhomMap.tsx`: locazioni + adiacenze da `eldhom.state.full`, token attori per locazione
      (PG/PNG/Gruppo/Boss), stile edge FREE/CLOSED_DOOR/LOCKED_DOOR (solido vs tratteggiato,
      colore rosso condiviso — mirror 1:1 di `map_scene.py`), aggiornamento su `eldhom.pg.moved`,
      `eldhom.monster.moved`, `eldhom.zone_door.opened`
- [ ] `TimelineTrack.tsx`: tracciato ordine attivazione da sinistra (più indietro) a destra,
      colore per tipo attore, soglie eventi missione come marcatori — aggiornamento su
      `eldhom.turn.next_actor` (mirror di `timeline_widget.py`/gmGui `TimelineWidget`)
- [ ] `engine/contract.ts` + `engine/gameState.ts`: sezioni mappa/timeline del reducer
- [ ] Smoke test: `eldhom.state.full` popola mappa e timeline correttamente; una mossa PG
      sposta il token e avanza la timeline nel browser

**Notes:**
- Valutare in fase di implementazione se `TimelineTrack` sia generalizzabile in `WebGUI_Lib`
  (concetto "traccia ordine attivazione attori" non è intrinsecamente Eldhôm-specifico) — non
  deciso ora, da rivalutare con evidenza concreta di un secondo consumatore (stessa logica
  "estrai quando serve davvero" già seguita per `Modal`/`WebGUI_Lib`).

---

### Phase 4 — Mano, Sequenze & Azioni [⏳]

- [ ] `HandPanel.tsx`: carte in mano, evidenziazione giocabilità da stato sequenza
      (`SequenceEngine`/`eldhom.pg.sequence_started|ended|broken`), icone tipo/effetto
      (mirror `hand_widget.py` + logica icone di `eldhom_main_window.py`)
- [ ] `ActionPanel.tsx`: 4 azioni semplici (Movimento/Attacco/Interazione/Recupero, costi ⌛
      fissi da `EldhomTypes.hpp`), armamento mossa/attacco stile point-and-click (mirror
      `action_panel_widget.py`), **finestra di reazione TAKE/BLOCK/DODGE inline** (non è una
      dialog modale sul desktop — è già gestita dentro `ActionPanelWidget`, stesso trattamento qui)
- [ ] Comandi: `eldhom.play_card`, `eldhom.simple_action`, `eldhom.stop_sequence`,
      `eldhom.declare_attack`, `eldhom.react_defense`
- [ ] Smoke test: un turno PG completo (azione semplice + una carta in sequenza) end-to-end
      nel browser, incl. un attacco con scelta di reazione TAKE/BLOCK/DODGE

**Notes:**
- Nessuna dialog qui: la finestra di reazione è deliberatamente inline (mirror esatto del
  comportamento desktop — l'utente ha citato "le Dialog" riferendosi alle 3 vere `QDialog`
  trattate in Phase 6, non a questa).

---

### Phase 5 — Eroi, Info Area & Log [⏳]

- [ ] `HeroPanel.tsx` × N (2-5 istanze): HP/risorse/conteggio mano/stato per eroe (mirror
      `hero_panel_widget.py`); valutare riuso di `ActorStatusBadges` (`@webgui/*`) per la sola
      striscia stati, con `HeroPanel` per il resto (HP/risorse) — split generico/specifico
      analogo a Tris
- [ ] `AreaInfoPanel.tsx`: dettaglio locazione selezionata (mirror `area_info_widget.py`)
- [ ] Formattazione log Eldhôm-specifica (icone/testo, mirror `log_widget.py::_format_event`)
      che alimenta il componente **già generico** `EventLog` da `@webgui/*` (nessuna nuova
      componente di rendering: solo la funzione di formattazione è nuova/specifica)
- [ ] Smoke test: danno/cura/KO riflessi nei pannelli eroe corretti; selezione locazione
      sulla mappa aggiorna `AreaInfoPanel`; log coerente con i testi del riferimento desktop

**Notes:**
- Stesso principio Tris: `EventLog` resta puro rendering (props `entries`), tutta la logica di
  formattazione testo/icone vive lato Eldhôm (`engine/logFormat.ts`, nuovo file).

---

### Phase 6 — Dialog Interattive → Modali Web [⏳]

- [ ] `webLib/WebGUI_Lib/src/components/Modal.tsx` (**NUOVO, generico**): backdrop, focus,
      chiusura Esc/click-esterno — nessun contenuto di dominio, nessuna riga Eldhôm-specifica
- [ ] `FormationModal.tsx` (ex `FormationDialog`): checkbox per attore, vincolo
      Retroguardia ≤ Prima Linea (OK disabilitato se violato) — mostrata su
      `eldhom.formation.dialog_needed`, risolta con `eldhom.resolve_formation`
      (`{backline_actor_ids}`)
- [ ] `InstantWindowModal.tsx` (ex `InstantWindowDialog`): checkbox per istantanea giocabile,
      pulsanti "Nessuna"/"Gioca selezionate" — due varianti dello stesso contenuto:
      - proattiva: mostrata su `eldhom.instant.window_opened`, risolta con `eldhom.play_instants`
      - reattiva (Assestarsi/avvicinamento nemico): mostrata su `eldhom.pg.enemy_approach`,
        risolta con `eldhom.play_reactive_instants`
- [ ] `MissionSelectModal.tsx` (ex `MissionSelectDialog`): lista missioni da **`GET /missions`**
      (nuovo endpoint `eng_serve`, non più scansione locale del filesystem) invece di scandire
      `data/mission_*.json` lato browser; selezione avvia `POST /sessions` con `mission_id`
- [ ] Smoke test: risoluzione completa di uno Scompaginamento via modale, di una Finestra
      Istantanee (entrambe le varianti) e selezione/avvio missione dalla lista — tutto senza
      alcuna modifica al wire-contract engine

**Notes:**
- **Nessuna modifica engine necessaria**: le 3 dialog sono già scambi evento→comando asincroni
  nel motore C++ (`EVT_FORMATION_DIALOG`/`CMD_RESOLVE_FORMATION`,
  `EVT_INSTANT_WINDOW_OPEN`/`CMD_PLAY_INSTANTS`, nessun evento per la selezione missione —
  puramente client/server REST). Il `QDialog.exec()` bloccante è una scelta locale della sola
  GUI desktop, non un requisito del protocollo.
- Stato "modale pendente" nel reducer: un solo campo tipo
  `pendingModal: FormationRequest | InstantWindowRequest | null` alla volta è sufficiente per
  Phase 6 (il motore apre una finestra alla volta per lo stesso attore) — da confermare/estendere
  se emergono sovrapposizioni durante l'implementazione.

---

### Phase 7 — Hardening & Test [⏳]

- [ ] Test backend `pytest` su `pyLib/gmWebServe` (unit, condivisi Tris+Eldhôm) +
      `eng_serve/tests/test_session_e2e.py` Eldhôm-specifico con `eldhom_engine.exe` reale
- [ ] Test frontend Vitest+RTL per ogni nuovo componente (`EldhomMap`, `TimelineTrack`,
      `HandPanel`, `ActionPanel`, `HeroPanel`, `AreaInfoPanel`, 3 modali)
- [ ] Gestione riconnessione WebSocket lato frontend
- [ ] Checklist di sicurezza minima: validazione Pydantic su ogni body REST, CORS esplicito
- [ ] Smoke test: suite completa verde (backend + frontend) in esecuzione locale

---

### Phase 8 — Unificazione Desktop (opzionale, futura) [⏳]

- [ ] Stessa valutazione già annotata per Tris — opzionale, non richiesta dalla coesistenza
      permanente desktop/web

---

## Key Design Decisions

1. **Pilota #2 dopo Tris:** Eldhôm è scelto per validare che sia `webLib/WebGUI_Lib` sia il
   pattern `eng_serve` si generalizzino a un dominio molto più ricco (mappa, timeline, mano,
   formazioni, dialog) e non solo al caso semplice di Tris.
2. **Convivenza permanente:** la GUI PySide6 esistente resta invariata e autonoma; la WebApp è
   un canale aggiuntivo, non un rimpiazzo — stesso principio di Tris.
3. **`MONO_USER_MANAGES_ALL_PLAYER`:** un solo utente locale gestisce tutti i PG; nessuna
   sessione multi-utente/autenticazione in questo piano (Phase 2 deliberatamente rimandata).
4. **Le 3 dialog diventano modali web event-driven, non RPC bloccanti:** nessuna modifica al
   wire-contract C++ — solo un nuovo stato React "modale pendente" popolato dagli eventi
   `eldhom.formation.dialog_needed` / `eldhom.instant.window_opened` /
   `eldhom.pg.enemy_approach`, risolto inviando il comando corrispondente.
5. **Estrazione backend condivisa (`pyLib/gmWebServe`) contestuale a questo piano, non rimandata:**
   coerente con la scelta già fatta per `webLib/WebGUI_Lib` — il codice Tris esistente è già
   privo di logica specifica, quindi il costo di estrazione è marginale.
6. **`eng_serve` resta puro pass-through typeId-agnostico:** anche con un contratto ~4× più
   ricco di quello di Tris, la facciata non interpreta alcun payload — tutta la logica di
   dominio resta nell'engine C++ (stato) e nel reducer frontend (presentazione).
