# Le Pergamene di Eldhôm — WebApp Development Plan

**Version:** 0.11.0
**Status:** Phase 11 – Completato ✅ (Phase 2 saltata/rimandata; Phase 7-8 non ancora iniziate —
Phase 9-11 inserite fuori sequenza su richiesta esplicita utente per correzioni estetiche/UX
critiche — vedi Esito sotto)
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

### Phase 2 — Multi-Session & Auth [⏸️ Deliberatamente rimandata / saltata]

- [ ] (invariato rispetto a Tris — vedi `GAME/Tic-Tac-Toe/WebApp/PLAN.md` Phase 2 per la checklist)

**Notes:**
- **Decisione esplicita dell'utente per questa pianificazione (2026-07-17):** si mantiene
  `MONO_USER_MANAGES_ALL_PLAYER` — un solo utente locale controlla tutti i 2-5 PG della
  missione, esattamente come la GUI desktop oggi. Questa fase riparte da qui solo quando/se
  si formerà l'esigenza multi-utente (stessa logica già applicata a Tris).
- **Confermato di nuovo (stesso turno di Phase 3):** l'utente ha esplicitamente chiesto di
  saltare la Phase 2 e procedere direttamente con la Phase 3 — nessun lavoro svolto qui,
  scelta deliberata confermata due volte.

---

### Phase 3 — Mappa & Linea Temporale [✅ Completato]

- [x] `EldhomMap.tsx`: locazioni + adiacenze da `eldhom.state.full`, token attori per locazione
      (PG/PNG/Gruppo/Boss), stile edge FREE/CLOSED_DOOR/LOCKED_DOOR (solido vs tratteggiato,
      colore rosso condiviso — mirror 1:1 di `map_scene.py`), aggiornamento su `eldhom.pg.moved`,
      `eldhom.monster.moved`, `eldhom.zone_door.opened`
- [x] `TimelineTrack.tsx`: tracciato ordine attivazione da sinistra (più indietro) a destra,
      colore per tipo attore, soglie eventi missione come marcatori — aggiornamento su
      `eldhom.turn.next_actor` (mirror di `timeline_widget.py`/gmGui `TimelineWidget`)
- [x] `engine/contract.ts` + `engine/gameState.ts`: sezioni mappa/timeline del reducer
- [x] Smoke test: `eldhom.state.full` popola mappa e timeline correttamente; una mossa PG
      sposta il token e avanza la timeline nel browser

**Esito Phase 3 (validato):**

- Campi/eventi confermati leggendo direttamente `GAME/Eldhom/CoreEngine/main.cpp`
  (`emit_full_state()`/`forward_engine_event()`), non assunti dal solo elenco `EventType` in
  `EldhomTypes.hpp`: `eldhom.state.full` → shape completa (`locations[].{id,name,adjacent}`,
  `heroes[].{id,name,location,position,hp,max_hp,timeline,...}`,
  `groups[].{id,name,timeline,location,monster_type,instances[].{id,location,position,hp,
  max_hp,alive}}`, `special_objects[].{object_id,type,location_id,locked_adjacency}`,
  `opened_zone_doors`, `next_actor.{actor_id,kind}`); `eldhom.pg.moved`/`eldhom.monster.moved`
  → `{actor_id, payload: <destinationLocationId>}` (stesso schema generico
  `forward_engine_event(type, actor_id, payload)`); `eldhom.zone_door.opened` →
  `{actor_id, payload: {a, b}}`; `eldhom.turn.next_actor` →
  `{actor_id, actor_name, actor_timeline, kind, mission_time}`;
  `eldhom.mission.time_advanced` → `{actor_id, payload: <nuovo timeline dell'attore>}`
  (usato per aggiornare live il chip timeline dell'attore che ha appena agito, mirror esatto
  di `eldhom_main_window.py::_on_time_advanced`).
- Layout mappa: BFS a livelli dalla prima locazione della missione (non il layout
  force-directed del desktop `GmMapModule`/`MapScene` — scelta deliberata per evitare una
  dipendenza di layout/fisica per un deliverable di Phase 3; risultato visivamente diverso
  ma funzionalmente equivalente). Colori edge/token **fissi, non theme-reattivi** (mirror
  esatto di `map_scene.py::_edge_pen()`/`theme_manager.py`, che li documenta esplicitamente
  come "fixed gameplay-state semantics, not theme-dependent"): FREE = `--gm-border` (theme),
  CLOSED_DOOR = `#c03030` solido, LOCKED_DOOR = `#c03030` tratteggiato, token = `#1a237e`/
  bordo turno-attivo `#e03030`. `TimelineTrack` invece resta theme-reattivo (`--gm-*`), perché
  il riferimento desktop (`timeline_widget.py`) è esso stesso QSS/tema-dipendente.
- Zona/porta: `zoneFromLocationId` (strip suffisso numerico finale) replica `_zone_from_loc_id`
  1:1 — con id senza suffisso numerico (es. `"ingresso"`/`"corridoio"` di `missione_01`) ogni
  locazione è la propria zona, quindi ogni arco parte CLOSED_DOOR finché non attraversato
  (comportamento confermato nel browser: `eldhom.zone_door.opened` per ingresso↔corridoio ha
  correttamente commutato quell'arco a FREE, lasciando corridoio↔sala CLOSED_DOOR).
- Etichette token: stessa euristica del desktop (`_monster_label_prefix_from_payload`) —
  prefisso da `monster_type` (prima lettera, +E se elite/boss), suffisso numerico preservato
  se presente nell'id istanza altrimenti contatore progressivo per prefisso. **Collisione di
  etichetta identica al desktop, non un bug**: `brigante_B1` (gruppo "Briganti B") e
  `brigante_A1` (gruppo "Briganti A") generano entrambi il label "B1" (stesso `monster_type`
  "brigante_comune" → prefisso "B", stesso suffisso numerico "1") — verificato che il desktop
  fa esattamente lo stesso, quindi non è stato "corretto" oltre la fedeltà richiesta.
- **Smoke test end-to-end validato nel browser reale** con introspezione DOM diretta (non solo
  screenshot): dopo l'avvio missione, `TimelineTrack` mostra Thael/Velyr ⏳0 e i 2 gruppi
  mostro ⏳4 (ordine corretto); `EldhomMap` mostra 3 nodi in linea (Ingresso/Corridoio/Sala)
  con i token corretti per locazione. Dopo un'azione MOVE (thael→corridoio): il tempo missione
  passa a ⏳3, il chip di Thael in `TimelineTrack` si aggiorna a ⏳3, il token PG1 si sposta sul
  nodo Corridoio, l'arco ingresso↔corridoio passa da classe CSS
  `eldhom-map__edge--closed_door` a `eldhom-map__edge--free` (verificato leggendo
  `getAttribute('class')` sugli elementi `<line>` via `run_playwright_code`), e il token PG2
  (Velyr, prossimo attore) mostra la classe `eldhom-map__token--active` mentre PG1 non la mostra
  più — tutte le transizioni di stato incrementali (senza attendere il successivo
  `eldhom.state.full`) confermate corrette.
- `TimelineTrack` **non** generalizzato in `WebGUI_Lib` in questo turno (nessun secondo
  consumatore concreto ancora) — resta locale a `GAME/Eldhom/WebApp/webapp_frontend`, come da
  nota Phase 3 originale.

**Notes:**
- Valutare in fase di implementazione se `TimelineTrack` sia generalizzabile in `WebGUI_Lib`
  (concetto "traccia ordine attivazione attori" non è intrinsecamente Eldhôm-specifico) — non
  deciso ora, da rivalutare con evidenza concreta di un secondo consumatore (stessa logica
  "estrai quando serve davvero" già seguita per `Modal`/`WebGUI_Lib`).

---

### Phase 4 — Mano, Sequenze & Azioni [✅ Completato]

- [x] `HandPanel.tsx`: carte in mano, evidenziazione giocabilità da stato sequenza
      (`SequenceEngine`/`eldhom.pg.sequence_started|ended|broken`), icone tipo/effetto
      (mirror `hand_widget.py` + logica icone di `eldhom_main_window.py`)
- [x] `ActionPanel.tsx`: 4 azioni semplici (Movimento/Attacco/Interazione/Recupero, costi ⌛
      fissi da `EldhomTypes.hpp`), armamento mossa/attacco stile point-and-click (mirror
      `action_panel_widget.py`), **finestra di reazione TAKE/BLOCK/DODGE inline** (non è una
      dialog modale sul desktop — è già gestita dentro `ActionPanelWidget`, stesso trattamento qui)
- [x] Comandi: `eldhom.play_card`, `eldhom.simple_action`, `eldhom.stop_sequence`,
      `eldhom.declare_attack`, `eldhom.react_defense`
- [x] Smoke test: un turno PG completo (azione semplice + una carta in sequenza) end-to-end
      nel browser, incl. un attacco con scelta di reazione TAKE/BLOCK/DODGE

**Esito Phase 4 (validato):**

- NUOVO endpoint `GET /cards` (`eng_serve/cards.py`, mirror di `missions.py`): scansiona
  `data/cards_*.json` lato server (sostituisce `_load_card_catalog()` locale della GUI
  desktop) — pass-through puro, nessun `response_model` Pydantic rigido (la forma di
  `effects[]` varia troppo per tipo, coerente con la Key Design Decision 6).
- Campi/eventi confermati leggendo direttamente `GAME/Eldhom/CoreEngine/main.cpp`
  (`handle_play_card`/`handle_declare_attack`/`handle_react_defense`/`handle_stop_sequence`/
  `emit_defense_window`/`emit_instant_window`), non assunti dalla sola GUI desktop:
  `eldhom.play_card` → `{hero_id, card_id, destination?, target_id?, discard_ids?}`;
  `eldhom.stop_sequence` → `{hero_id}`; `eldhom.declare_attack` → `{hero_id, target_id}`;
  `eldhom.react_defense` → `{defender_id, reaction}`. `eldhom.reaction.window_opened`/
  `window_closed` e `eldhom.attack.resolved` sono inviati **alla radice del payload**
  (`{attacker_id, defender_id, incoming_damage, reactions}`, ecc.), NON nello schema
  generico `{actor_id, payload}` usato dalla maggior parte degli altri eventi —
  differenza confermata leggendo `emit_defense_window()`/`handle_react_defense()`
  direttamente (non deducibile dalla sola lista `EventType`).
- `engine/cardIcons.ts` (NUOVO): porta 1:1 `_CARD_TYPE_ICONS`/`_EFFECT_ICONS`/
  `_ATTACK_TYPE_ICONS`/`_POSITION_ICONS`/`_effect_summary_line()` da
  `eldhom_main_window.py` (solo il riassunto compatto per riga, non l'intero blocco
  HTML `_card_description()` — non necessario per lo scopo di Phase 4).
- `engine/gameState.ts` esteso: `cards` (catalogo, caricato una volta con
  `applyCardCatalog`, non guidato da envelope), `handByHero`, `sequenceActiveByHero`,
  `pendingReaction`, `nextActorKind`. Regola di giocabilità mano (non nel desktop, che
  usa un solo flag enabled/disabled per l'intera mano — qui è un'estensione UX
  esplicitamente richiesta dal checklist Phase 4, non stretta validazione): nessuna
  sequenza attiva → SINGLE/SEQ_START giocabili; sequenza attiva → solo SEQ_CONTINUE/
  SEQ_END; INSTANT mai giocabile dalla mano (solo via finestre di reazione, Phase 6).
  Il motore resta comunque l'autorità finale (una giocata illegale produce solo
  `action.result:{ok:false}`).
- `components/HandPanel.tsx` + `components/ActionPanel.tsx` (NUOVI): port di
  `hand_widget.py`/`action_panel_widget.py`. Costi ⌛ duplicati come costanti fisse
  lato frontend (`EldhomTypes.hpp::COST_SIMPLE_*`) perché `eng_serve` non espone
  costanti motore sul wire (resta pass-through puro).
- `components/EldhomMap.tsx` esteso con `onLocationClick`/`onTokenClick` opzionali
  (targeting point-and-click riusato sia per azioni semplici sia per carte con
  effetto MOVE/DAMAGE che richiedono destinazione/bersaglio — mirror di
  `_pre_play_card_hook`/`_try_move`/`_try_play_card_with_target`, MA senza la
  validazione BFS di raggiungibilità lato client per le mosse guidate da carta:
  semplificazione deliberata, il motore valida comunque e rifiuta con
  `action.result:{ok:false}` una mossa non valida).
- `App.tsx`: nuovo stato `targeting` (unione discriminata simple-move/simple-attack/
  card-move/card-attack, quest'ultimo con `destination` opzionale per il caso
  "muovi-poi-attacca" di carte come *Passo e Lama*, mirror del chaining desktop),
  reset automatico del targeting al cambio turno o apertura/chiusura finestra di
  reazione. **Bug trovato e corretto durante il test nel browser**: il pulsante
  "Annulla Attacco"/"Annulla Muovi" disarmava solo il targeting `simple-*`, non
  quello `card-*` (cliccarlo mentre un targeting guidato da carta era armato lo
  trasformava in un targeting semplice invece di annullarlo) — corretto verificando
  entrambe le varianti. **Guardia aggiunta** (mirror `_actor_is_enemy` desktop):
  cliccare il token di un alleato durante il targeting d'attacco mostra un errore
  invece di inviare il comando (il motore ha comunque un fallback di
  auto-selezione del bersaglio valido, documentato in `_pre_play_card_hook`, ma la
  guardia lato client evita di affidarsi silenziosamente a quel fallback).
- **Smoke test end-to-end validato nel browser reale** (missione `missione_01`):
  Thael e Velyr spostati entrambi a "corridoio" (evitando un `eldhom.
  formation.dialog_needed` imprevisto incontrato spostando un solo eroe alla volta
  — vedi nota sotto); mano di Thael renderizzata con icone corrette e playability
  toggle confermato (`Secondo Colpo`/`Colpo di Chiusura` disabilitati senza sequenza
  attiva); giocata la carta `Colpo Secco` (SINGLE, effetto DAMAGE) → targeting
  d'attacco armato correttamente ("⚔ Clicca il nemico...") → `eldhom.attack.declared`
  → `eldhom.instant.window_opened` (finestra istantanea, bypassata con un comando
  diretto `play_instants:{selected:[]}` per continuare il test senza costruire UI
  Phase 6 in anticipo) → `eldhom.reaction.window_opened` → `ActionPanel` mostra
  correttamente la vista DIFESA ("DIFESA: B1 — danno in arrivo 1❌", pulsanti
  Subisci/Para/Schiva) → scelto BLOCK → `eldhom.attack.resolved`
  (`base_damage:1, final_damage:0, reaction:BLOCK`) → `eldhom.reaction.window_closed`
  → `eldhom.turn.next_actor`. Ciclo completo dichiarazione-attacco→reazione→risoluzione
  confermato funzionante.
- **Catena di sequenza completa (SEQ_START→CONTINUE→END) NON testata end-to-end in
  questo turno**: la mano di Thael pescata in questa run non includeva una carta
  SEQ_START (pesca casuale) — la REGOLA di playability (SEQ_CONTINUE/END disabilitati
  senza sequenza attiva) è comunque stata confermata visivamente corretta in una
  sessione precedente. Da riverificare opportunisticamente in Phase 5/7 se una mano
  con SEQ_START ricorre naturalmente durante altri test.
- **Dipendenza Phase 6 incontrata organicamente**: sia `eldhom.formation.
  dialog_needed` sia `eldhom.instant.window_opened` sono emersi spontaneamente
  durante il test (non forzati) — confermano che questi 2 dialog sono frequenti in
  questo motore e Phase 6 (Modal generico + `FormationModal`/`InstantWindowModal`)
  è necessaria per una sessione di gioco reale ininterrotta; per Phase 4 sono stati
  aggirati con un comando diretto (`play_instants:{selected:[]}`) o evitati (doppio
  movimento invece di interact) senza introdurre UI Phase 6 in anticipo.

**Notes:**
- Nessuna dialog qui: la finestra di reazione è deliberatamente inline (mirror esatto del
  comportamento desktop — l'utente ha citato "le Dialog" riferendosi alle 3 vere `QDialog`
  trattate in Phase 6, non a questa).

---

### Phase 5 — Eroi, Info Area & Log [✅ Completato]

- [x] `HeroPanel.tsx` × N (2-5 istanze): HP/risorse/conteggio mano/stato per eroe (mirror
      `hero_panel_widget.py`); valutare riuso di `ActorStatusBadges` (`@webgui/*`) per la sola
      striscia stati, con `HeroPanel` per il resto (HP/risorse) — split generico/specifico
      analogo a Tris
- [x] `AreaInfoPanel.tsx`: dettaglio locazione selezionata (mirror `area_info_widget.py`)
- [x] Formattazione log Eldhôm-specifica (icone/testo, mirror `log_widget.py::_format_event`)
      che alimenta il componente **già generico** `EventLog` da `@webgui/*` (nessuna nuova
      componente di rendering: solo la funzione di formattazione è nuova/specifica)
- [x] Smoke test: danno/cura/KO riflessi nei pannelli eroe corretti; selezione locazione
      sulla mappa aggiorna `AreaInfoPanel`; log coerente con i testi del riferimento desktop

**Esito Phase 5 (validato):**

- `components/HeroPanel.tsx` (NUOVO): port di `hero_panel_widget.py`. **`ActorStatusBadges`
  valutato e SCARTATO** (checklist chiedeva esplicitamente di valutarne il riuso): si
  sottoscrive a eventi generici `gmActor.*` (`gmActor.snapshot`/`status_added`/
  `status_removed`) che il wire contract `eldhom.*` non produce affatto (quella traduzione
  esiste solo lato Python nell'`EldhomActorAdapter` desktop, non in `eng_serve`, che resta
  pass-through); inoltre l'informazione "di chi è il turno" che mostrerebbe è già coperta da
  `ActionPanel` ("TURNO: X"), dal bordo attivo di `HeroPanel` stesso e dall'evidenziazione
  del token attivo in `EldhomMap` — riusarlo sarebbe stato ridondante, non funzionalità
  mancante. Nessuna modifica a `webLib/WebGUI_Lib` per Phase 5.
- `components/AreaInfoPanel.tsx` (NUOVO): port di `area_info_widget.py`, puramente
  presentazionale (nessun networking proprio, dati derivati da `eldhomState` nel genitore).
- `engine/logFormat.ts` (NUOVO): porta 1:1 `_EVENT_TEMPLATES`/`_format_event`/`on_any_event`/
  `on_action_result`/`on_mission_victory`/`on_mission_defeat`/`on_next_actor` da
  `log_widget.py` — solo icone/testo, i colori per riga sono stati deliberatamente omessi
  (coerente con la nota Phase 5 originale: `EventLog` resta il componente generico esistente
  con `entries: string[]` semplici, nessuna nuova prop di colore aggiunta). Lo stato privato
  `lastDisplayedTime` (per il riempimento "Tempo N: Nessun Attore" tra un attore e l'altro)
  è tenuto come variabile di modulo, non nel reducer `gameState.ts` — è bookkeeping di
  presentazione del log, non stato di dominio (stessa separazione già stabilita per la mappa
  in Phase 3).
- `engine/gameState.ts` esteso: `heroesById: Record<string, HeroWire>` (popolato da
  `wire.heroes` su ogni `state.full`), `MapLocation.adjacent` (necessario per
  `AreaInfoPanel`), `ActorToken.position` (necessario per mostrare Prima Linea/Retroguardia
  nella lista attori dell'area).
- `App.tsx`: nuovo stato `selectedLocationId`; `handleLocationClick`/`handleTokenClick` ora
  gestiti in DOPPIO percorso — se un targeting è armato, risolvono l'azione (come in Phase 4);
  altrimenti impostano semplicemente `selectedLocationId` per popolare `AreaInfoPanel` (mirror
  di `_on_area_selected`/`_on_actor_selected` del desktop, che fanno la stessa distinzione).
  `EldhomMap` ora riceve SEMPRE gli handler di click (non più condizionati da
  `targetingMode`), dato che servono comunque per la sola selezione informativa; il cursore
  CSS è stato cambiato da `crosshair` a `pointer` di conseguenza (uso ora duale, non solo
  targeting). Aggiunto un secondo `<EventLog>` per il log narrativo, accanto a quello JSON
  grezzo di Phase 1 (mantenuto per debug).
- **Smoke test end-to-end validato nel browser reale** (missione `missione_01`): avviata la
  missione, confermati 2 `HeroPanel` con dati corretti (Thael ❤6/6, Attivo, mazzo:5/5, ⌛0,
  ingresso/Primo piano; Velyr ❤5/5, ingresso/Retro); log narrativo confermato con le righe
  attese ("thael: mazzo rimescolato", "velyr: mazzo rimescolato",
  "⌛ Tempo 0: ▶ Gioca Thael"); click sul nodo Corridoio → `AreaInfoPanel` mostra
  correttamente "Area: Corridoio", "Adiacenti: Ingresso, Sala" (nomi risolti, non id grezzi),
  e i 2 mostri presenti con HP/posizione corretti; dopo un'azione MOVE di Thael verso
  corridoio, `HeroPanel` di Thael si è aggiornato correttamente (⌛3, 📍 corridoio/Primo
  piano) confermando che la stessa fonte dati (`state.full` → `heroesById`) alimenta
  correttamente sia i token mappa (già validati in Phase 3) sia i pannelli eroe.

**Notes:**
- Stesso principio Tris: `EventLog` resta puro rendering (props `entries`), tutta la logica di
  formattazione testo/icone vive lato Eldhôm (`engine/logFormat.ts`, nuovo file).

---

### Phase 6 — Dialog Interattive → Modali Web [✅ Completato]

- [x] `webLib/WebGUI_Lib/src/components/Modal.tsx` (**NUOVO, generico**): backdrop, focus,
      chiusura Esc/click-esterno — nessun contenuto di dominio, nessuna riga Eldhôm-specifica
- [x] `FormationModal.tsx` (ex `FormationDialog`): checkbox per attore, vincolo
      Retroguardia ≤ Prima Linea (OK disabilitato se violato) — mostrata su
      `eldhom.formation.dialog_needed`, risolta con `eldhom.resolve_formation`
      (`{backline_actor_ids}`)
- [x] `InstantWindowModal.tsx` (ex `InstantWindowDialog`): checkbox per istantanea giocabile,
      pulsanti "Nessuna"/"Gioca selezionate" — due varianti dello stesso contenuto:
      - proattiva: mostrata su `eldhom.instant.window_opened`, risolta con `eldhom.play_instants`
      - reattiva (Assestarsi/avvicinamento nemico): mostrata su `eldhom.pg.enemy_approach`,
        risolta con `eldhom.play_reactive_instants`
- [x] `MissionSelectModal.tsx` (ex `MissionSelectDialog`): lista missioni da **`GET /missions`**
      (nuovo endpoint `eng_serve`, non più scansione locale del filesystem) invece di scandire
      `data/mission_*.json` lato browser; selezione avvia `POST /sessions` con `mission_id`
- [x] Smoke test: risoluzione completa di uno Scompaginamento via modale, di una Finestra
      Istantanee (entrambe le varianti) e selezione/avvio missione dalla lista — tutto senza
      alcuna modifica al wire-contract engine

**Esito Phase 6 (validato):**

- `webLib/WebGUI_Lib/src/components/Modal.tsx` (NUOVO): backdrop `position:fixed` +
  chrome (titolo, chiusura opzionale), Esc gestito via `useEffect`+listener, click-esterno
  gestito via `onClick` sul backdrop con `stopPropagation()` sul contenuto. Nessuna riga
  Eldhôm-specifica. Esportato dal barrel `index.ts`. **Nessuna regressione in Tris**
  confermata rieseguendo `npm run build`/`npm run lint` sul suo frontend dopo la modifica
  alla libreria condivisa.
- Scoperta chiave leggendo `GAME/Eldhom/CoreEngine/main.cpp` direttamente: la variante
  REATTIVA della finestra istantanee (Assestarsi/`eldhom.pg.enemy_approach`) NON ha un
  evento/contenuto separato — il motore chiama la STESSA `emit_instant_window()` usata per
  la variante proattiva (confermato al rigo dove `advance_auto()` rileva
  `has_pending_reactive_window()` e richiama `emit_instant_window(pending_reactive_window
  ().trigger)`). Questo significa che **un solo `InstantWindowModal` serve entrambe le
  varianti**: il chiamante distingue solo leggendo `InstantWindowWire.trigger`
  (`"eldhom.pg.enemy_approach"` → reattiva → `eldhom.play_reactive_instants`; altrimenti
  proattiva → `eldhom.play_instants`) — nessuna duplicazione di componente necessaria.
- Campi comando confermati da `main.cpp`: `eldhom.resolve_formation` →
  `{faction_id, location_id, backline: string[]}` (NON `{backline_actor_ids}` come una
  lettura superficiale del solo `EldhomTypes.hpp` avrebbe potuto suggerire — il nome campo
  esatto va sempre confermato in `handle_resolve_formation()`); `eldhom.play_instants`/
  `eldhom.play_reactive_instants` → `{selected: [{actor_id, card_id}]}`.
  `eldhom.formation.dialog_needed`/`eldhom.instant.window_opened` sono inviati **alla radice
  del payload** (come già osservato in Phase 4 per la finestra di reazione), non nello schema
  generico `{actor_id, payload}`.
- Stato "modale pendente" nel reducer: DUE campi separati (`pendingFormation`,
  `pendingInstantWindow`), non un singolo campo unione come ipotizzato nella nota Phase 6
  originale — più semplice da gestire dato che i due tipi di finestra hanno forme diverse e
  non si escludono a vicenda nel tempo (anche se il motore ne apre una alla volta). Pulizia
  ottimistica lato client (`pendingFormation`/`pendingInstantWindow` azzerati subito
  all'invio del comando di risoluzione): se il motore ha ANCORA una formazione/finestra
  pendente dopo la risoluzione, invia immediatamente un NUOVO evento `dialog_needed`/
  `window_opened` che sovrascrive lo stato locale — nessuna logica di concatenamento lato
  client necessaria (confermato nel test: una DODGE ha scompaginato la formazione nemica,
  facendo apparire correttamente una SECONDA `FormationModal` per la fazione `BRIGANTI`
  subito dopo aver risolto la prima per `HEROES`).
- **Bug trovato e corretto durante il test nel browser**: `MissionSelectModal` calcolava la
  selezione di default con l'inizializzatore di `useState(() => missions[0]?.mission_id ??
  '')`, che si esegue UNA SOLA VOLTA al primo render — dato che `missions` arriva sempre in
  modo asincrono (`GET /missions`) DOPO il primo render, la selezione restava bloccata a
  stringa vuota per sempre (pulsante OK permanentemente disabilitato). Corretto con un
  `useEffect` che sincronizza `selectedId` al primo elenco valido ricevuto. **Lezione
  generale**: un valore iniziale derivato da una prop che arriva in modo asincrono va sempre
  sincronizzato con un `useEffect`, mai calcolato solo nell'inizializzatore di `useState`.
- Sezione mission-select inline (Phase 1–5, `<select>`+pulsante "Avvia missione" dentro
  `.controls`) rimossa da `App.tsx` e sostituita interamente da `MissionSelectModal`, mostrata
  automaticamente quando `sessionId === null` — come da checklist ("ex MissionSelectDialog").
- **Smoke test end-to-end validato nel browser reale** (missione `missione_01`): avviata la
  missione dalla nuova `MissionSelectModal`; scatenato uno Scompaginamento (`HEROES @
  ingresso`, Velyr rimasto solo) — `FormationModal` mostrata correttamente con validazione
  (OK disabilitato con Retroguardia > Prima Linea, riabilitato deselezionando), risolta con
  successo; scatenata una Finestra Istantanee proattiva giocando `Colpo Secco` contro un
  nemico (`eldhom.instant.window_opened`, trigger `eldhom.monster.damaged`) —
  `InstantWindowModal` mostrata con "Velyr — Supporto Tattico" (nome eroe risolto
  correttamente, non l'id grezzo), istantanea selezionata e giocata con successo (mano di
  Velyr passata da 5 a 4 carte), flusso proseguito correttamente alla finestra di reazione
  (DIFESA, già Phase 4); scelto DODGE → ha scompaginato la formazione nemica → SECONDA
  `FormationModal` apparsa automaticamente per `BRIGANTI @ corridoio`, risolta con successo.
  Tutti i comandi verificati anche lato server (log `eng_serve`): `resolve_formation` ×2 con
  payload corretti, `play_instants` con payload corretto.
- **Variante REATTIVA (`eldhom.pg.enemy_approach`) non osservata/innescata naturalmente in
  questo turno di test** (richiede una risoluzione di comportamento mostro molto specifica) —
  ma essendo condivisa al 100% con la variante proattiva già validata (stesso componente,
  stesso evento motore, unica differenza il comando di risposta scelto in base al campo
  `trigger`), la fiducia nella sua correttezza è alta by construction. Da confermare
  opportunisticamente in Phase 7 se si ripresenta durante altri test.

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

### Phase 9 — Correzioni Estetiche e UX critiche (richiesta esplicita utente) [✅ Completato]

Fase inserita fuori sequenza (dopo Phase 6, prima di Phase 7/8) su richiesta esplicita
dell'utente dopo aver valutato Phase 1-6 come regressione rispetto alla GUI desktop
PySide6 su 4 punti concreti: estetica poco accattivante, mappa troppo piccola/senza
zoom-pan, zone carte mancanti/senza drag&drop, log in JSON grezzo illeggibile.

- [x] Mappa: zoom (rotellina, non-passive listener) + pan (drag pointer) + pulsante
      "Adatta alla vista", dimensioni adattive invece di un riquadro fisso piccolo
- [x] Log narrativo: colore per tipo di evento (porting 1:1 della palette esadecimale di
      `log_widget.py`), invece del solo JSON grezzo; JSON grezzo conservato ma relegato a
      un `<details>` collassato "solo debug"
- [x] Tavolo carte: 6 zone (Mazzo/Mano/Giocate/Memoria/Scarti/Bandite) con drag&drop HTML5
      nativo TRA le zone realmente supportate dal motore Eldhôm, più pulsanti alternativi
      (Pesca/Riprendi/Rimescola/🗑 Scarta) per ogni azione — sostituisce il precedente
      `HandPanel` a lista semplice
- [x] Overhaul estetico generale (CSS): direzione "tavolo fantasy/pergamena potenziato"
      (non un pivot completo a stile "modern card game") — profondità via ombre/gradienti,
      layout più ampio, stessa identità a 5 temi esistente, zero nuove dipendenze/asset
- [x] Smoke test end-to-end nel browser reale con entrambe le missioni di test

**Esito Phase 9 (validato):**

- `components/EldhomMap.tsx` **riscritto**: stato `ViewBox {x,y,w,h}` con
  `fitViewBox(layout)` ricalcolato ad ogni cambio di `layout` (nuova missione o
  sblocco di un passaggio non lascia la visuale bloccata su un'area ormai diversa).
  Zoom via listener `wheel` nativo non-passivo montato con `useEffect` (l'`onWheel` di
  React è passivo da React 17+ e ignorerebbe silenziosamente `preventDefault()`), centrato
  sul cursore, clampato `MIN_ZOOM=0.4`/`MAX_ZOOM=3.5`. Pan via `onPointerDown/Move/Up/Leave`
  con `setPointerCapture` (attivo solo su drag iniziato sullo sfondo vuoto, non su
  nodi/token, verificando `event.target === event.currentTarget`). Toolbar `+`/`−`/`⤢`
  sovrapposta alla mappa.
- **Bug scoperto e corretto durante il test nel browser**: con `preserveAspectRatio`
  di default (`xMidYMid meet`), un layout molto largo e poco alto (es. una missione a
  3 stanze in fila, viewBox ~690×182) veniva centrato **verticalmente** in un riquadro
  alto — spingendo il contenuto reale fuori dallo schermo iniziale (serviva scroll per
  vederlo), pur essendo tecnicamente presente nel DOM. Corretto con
  `preserveAspectRatio="xMidYMin meet"` (allineamento in alto, non al centro) — il
  contenuto è ora sempre visibile immediatamente sotto la toolbar, indipendentemente
  dal rapporto d'aspetto del layout.
- **Raffinamento successivo**: altezza del riquadro mappa resa adattiva
  (`aspect-ratio` inline calcolato da `layout.width/layout.height`, clampato tra 0.9 e 3,
  con `min-height: 320px`/`max-height: 620px` come guardrail CSS) invece di un'altezza
  fissa — evita grandi aree vuote sotto contenuti "larghi e bassi" (missioni lineari)
  mantenendo comunque un riquadro ampio per layout complessi/ramificati. Verificato
  visivamente con entrambe le missioni di test (`missione_01`: 3 nodi in fila;
  `mission_sim_a`: 13 nodi multi-riga) — nessuna delle due produce più spazio morto
  eccessivo né nodi illeggibili a schermo intero.
- `engine/logFormat.ts` **riscritto**: `EVENT_TEMPLATES` ora
  `Record<string, {template, color}>`; `formatEvent()` ritorna `EventLogEntry[]`
  (`{text, color}`) invece di `string[]`. Tutti i colori esadecimali portati 1:1 da
  `log_widget.py::_EVENT_TEMPLATES` e dai rami speciali (attacco arricchito,
  `zone_door.opened`, `action.result`, `mission.victory/defeat`, `formatNextActor`).
  Verificato via introspezione DOM diretta (`el.style.color`) nel browser reale che
  ogni riga di log produce esattamente il colore RGB atteso (es.
  `eldhom.deck.reshuffled` → `rgb(112,144,160)` = `#7090a0`).
- `webLib/WebGUI_Lib/src/components/EventLog.tsx`: nuovo tipo esportato
  `EventLogEntry = string | {text; color?}`; retrocompatibile — Tris continua a passare
  `string[]` senza modifiche (riverificato `npm run build` sul suo frontend).
- `components/DeckTable.tsx` (**NUOVO**, sostituisce `HandPanel.tsx` rimosso): 6 zone
  con drag&drop nativo (`draggable` + `onDragStart`/`onDragOver`/`onDrop`,
  `DataTransfer` con payload `{cardId, from}`) SOLO per i movimenti realmente
  supportati dal motore (mirror di `_DeckProxy` desktop): Mano→Giocate/Memoria
  (entrambe giocano la carta), Mano→Scarti (scarto GM), Mazzo→Mano (pesca),
  Scarti→Mano (riprendi, sempre la carta in cima). Ogni azione ha anche un'alternativa
  a pulsante (Pesca/Riprendi/Rimescola/🗑 per-carta) per chi non usa drag&drop.
  Memoria/Bandite mostrate ma etichettate come non-funzionali/aliasate (onestà
  funzionale, non finte zone).
- `App.tsx`: 4 nuovi handler (`handleDrawCard`/`handleDiscardCard`/`handleTakeDiscard`/
  `handleReshuffleDiscard`) che inviano i comandi GM-override
  (`eldhom.deck.draw/discard/take_discard/reshuffle`, tutti con solo `{hero_id}` tranne
  discard che aggiunge `{card_id}`); `<HandPanel>` sostituito da `<DeckTable>`; log JSON
  grezzo spostato dentro un `<details><summary>🛠 Log eventi grezzi (JSON) — solo
  debug</summary>` collassato di default.
- **Smoke test end-to-end validato nel browser reale** con entrambe le missioni:
  drag&drop di una carta MOVIMENTO da Mano a Giocate (simulato via `DragEvent`
  sintetico con `DataTransfer` reale, dato che le API Playwright ad alto livello
  `dragTo`/`click` restano bloccate in questo ambiente — vedi nota sotto) → targeting
  di movimento armato correttamente → click su nodo mappa adiacente → comando
  `eldhom.play_card` inviato → stato aggiornato (eroe spostato, turno passato,
  log narrativo colorato popolato correttamente); pulsanti Scarta/Riprendi/Pesca
  testati singolarmente con effetto corretto su conteggi Mano/Mazzo/Scarti; zoom
  (rotellina sintetica), pan (drag pointer sintetico) e "Adatta alla vista" testati
  con verifica diretta del `viewBox` SVG.
- **Gotcha riconfermato**: sia `click_element` sia `locator.dragTo()` di Playwright
  restano bloccati/in timeout in questo ambiente anche su elementi semplici — stessa
  causa già nota (vedi memoria utente tooling-lessons.md). Per il drag&drop HTML5
  nativo, il fallback che funziona è costruire un `DataTransfer` reale via
  `page.evaluate()` e disparare manualmente `dragstart`/`dragover`/`drop` con
  `bubbles:true` sugli elementi sorgente/destinazione — bypassa interamente la
  simulazione mouse di Playwright.
- **Gotcha operativo backend**: `eng_serve` usa un `session_id` fisso `"dev-session"`
  — un `POST /sessions` mentre una sessione precedente è ancora viva (anche solo da un
  precedente ciclo di test, sopravvive a un semplice reload di pagina) fallisce con
  `409 Conflict`. `Stop-Process`/`taskkill` sul processo uvicorn hanno fallito con
  "Accesso negato"; la tecnica WMI/CIM (`Get-CimInstance ... | Invoke-CimMethod
  -MethodName Terminate`) ha funzionato in modo affidabile. Vedi memoria repo
  `eldhom-webapp-plan.md` per la procedura completa.

**Notes:**
- Nessuna modifica al wire-contract C++/motore in questa fase — puramente
  frontend (React/CSS) + un piccolo ampliamento del tipo `EventLogEntry` condiviso.
- Fase inserita fuori sequenza numerica pianificata (tra Phase 6 e Phase 7) perché la
  richiesta utente era esplicitamente prioritaria rispetto a hardening/test formali —
  Phase 7 e 8 restano pianificate e non ancora iniziate.

---

### Phase 10 — Tipografia Tematica per Tema (richiesta esplicita utente) [✅ Completato]

Richiesta esplicita utente: il cambio TEMA deve cambiare anche i FONT, non solo i colori,
con un font più tematico in base al nome di ciascuno dei 5 temi condivisi (`themes.ts`).

- [x] `ThemeTokens` (`webLib/WebGUI_Lib/src/theme/themes.ts`) esteso con `displayFont`/
      `bodyFont` per ciascuno dei 5 temi; `themeToCssVars()` emette
      `--gm-font-display`/`--gm-font-body`
- [x] Coppia di font Google Fonts scelta per ogni tema in base al nome/atmosfera:
      Scroll → scrittura su pergamena/manoscritto; Stone → iscrizione romana/rune scolpite;
      Dark Moon → fantasy noir gotico (lupi/pipistrelli); Blood → gotico-vampiresco/
      draculiano; Techno → sci-fi/HUD/monitor
- [x] Import Google Fonts centralizzato in `webLib/WebGUI_Lib/src/styles.css` (un solo
      `@import`, tutte le famiglie/pesi necessari, con fallback serif/monospace di sistema
      se il font web non si carica)
- [x] `App.css` (Eldhôm): font di base (`.app`) collegato a `--gm-font-body`; font di ogni
      elemento titolo/nome (h1, titoli pannello/zona, nomi eroi/carte/nodi mappa) collegato
      a `--gm-font-display` — pulsanti e testo denso restano sul font body per leggibilità
- [x] Smoke test nel browser reale: tutti e 5 i temi verificati visivamente

**Esito Phase 10 (validato):**

- Coppie font finali (tutte da Google Fonts, con fallback serif/monospace di sistema):
  - **Scroll**: `IM Fell English` (titoli) + `EB Garamond` (corpo) — stampa/manoscritto antico
  - **Stone**: `Cinzel` (titoli) + `Crimson Text` (corpo) — iscrizione romana su pietra
  - **Dark Moon**: `Pirata One` (titoli) + `Spectral` (corpo) — fantasy noir gotico
  - **Blood**: `Metamorphous` (titoli) + `Cormorant` (corpo) — gotico/vampiresco leggibile
    (preferito a font "horror drip" come Nosifer, illeggibili sotto i ~16px usati per i
    titoli di pannello/zona in questa UI)
  - **Techno**: `Orbitron` (titoli) + `Share Tech Mono` (corpo) — sci-fi/HUD/terminale
- Font applicato SOLO a elementi titolo/nome (h1 app, titoli pannello/zona, nomi eroi,
  nomi carte, nomi nodi mappa, nome chip timeline) — pulsanti, input e testo denso
  (descrizioni carte, log) restano sul font body per non sacrificare la leggibilità a
  dimensioni piccole (11-13px), mirror del concetto di ruoli tipografici title/body/
  secondary già presente in `.github/specs/gui-theme.yml` per il desktop.
- **Nessuna modifica al desktop PySide6** in questa fase: `theme_manager.py` usa un solo
  font QSS fisso per tutti i temi (righe ~210-211) — fuori scope, la richiesta utente e
  tutto il contesto recente riguardavano esclusivamente la WebApp.
- **Retrocompatibile con Tris**: `themeToCssVars()` aggiunge solo 2 chiavi in più
  all'oggetto CSS-in-JS già esistente; Tris non referenzia le nuove variabili nel proprio
  CSS quindi resta visivamente invariato (nessuna regressione, `npm run build` riverificato).
- **Smoke test end-to-end validato nel browser reale**: tutti e 5 i temi selezionati dal
  dropdown "Tema" e verificati visivamente (screenshot) — h1, titoli pannello, nomi eroi
  (Thael/Velyr), nomi carte (Passo Cauto/Fendente Pesante/Colpo Secco), nomi nodi mappa
  (Ingresso/Corridoio/Sala) e titolo pannello Area Info cambiano font coerentemente ad
  ogni cambio tema; letto anche `getComputedStyle(...).getPropertyValue('--gm-font-*')`
  per confermare il valore CSS esatto dietro ogni rendering visivo.

**Notes:**
- Scelta di usare Google Fonts (via `@import` unico, non un pacchetto npm) invece di font
  di sistema: garantisce lo stesso identico rendering su qualunque OS (a differenza di font
  Windows-only come Chiller/Copperplate Gothic), a costo di richiedere rete al primo
  caricamento — fallback serif/monospace di sistema se offline, nessuna rottura funzionale.

---

### Phase 11 — Riordino Tavolo Carte e Anteprima "Carta Selezionata" (richiesta esplicita utente) [✅ Completato]

Richiesta esplicita utente: Scarti deve stare subito a sinistra di Mazzo; a destra di Mano va
aggiunto uno spazio "Carta Selezionata" con la descrizione estesa (titolo, icone, ecc.) come
se fosse una vera carta da gioco vista di fronte; Bandite per ora può essere solo un pulsante
che in futuro aprirà una pagina dedicata di gestione.

- [x] `DeckTable.tsx`: riordino zone in `Scarti, Mazzo, Mano, Carta Selezionata, Giocate,
      Memoria, Bandite` (Scarti e Mazzo adiacenti, stesso mazzo fisico visto dai due lati)
- [x] Nuovo pannello "Carta Selezionata": promuove il vecchio hint testuale in intestazione
      (`hoveredCardId`/dettaglio a una riga) a un riquadro dedicato che rende la carta come
      una vera carta da gioco — icona tipo, nome, badge costo (⏳), origine, riga effetti,
      descrizione estesa, tag (Prima Linea/Reazione/Condizione); alimentato dallo stesso stato
      (rinominato `selectedCardId`/`selectedCard`) già aggiornato da hover/focus su carte in
      Mano/Giocate/Scarti — nessun nuovo meccanismo di selezione, solo nuova presentazione
- [x] `Bandite` non è più una drop-zone (rimossi `onDragOver`/`onDrop`/dashed border): è ora
      un singolo pulsante `🚫 Bandite` in una cella centrata; il click invoca `onOpenBanish`
      (nuova prop) che per ora mostra solo un avviso placeholder tramite l'`ErrorBar`
      condiviso ("pagina dedicata non ancora disponibile") — nessuna nuova pagina/route creata
      in questa fase, come esplicitamente richiesto ("per ora")
- [x] CSS: griglia zone estesa da 6 a 8 colonne (`Carta Selezionata` occupa 2 colonne per
      leggibilità della descrizione estesa); rimosse le regole CSS morte `.eldhom-deck-table__
      detail*`; nuova famiglia di stili `.eldhom-deck-table__preview-card*` con bordo
      superiore colorato per `card_type` (`--gm-warning` INSTANT, `--gm-success` SEQ_START,
      `--gm-accent` SEQ_CONTINUE, `--gm-danger` SEQ_END) riusando i token tema esistenti
- [x] Smoke test nel browser reale: ordine zone verificato, hover reale (via
      `page.mouse.move`/`locator.hover`, NON `dispatchEvent` sintetico — vedi Notes) su una
      carta in mano mostra correttamente la carta nel pannello con tutti i dettagli; click su
      Bandite mostra l'avviso placeholder nell'ErrorBar

**Esito Phase 11 (validato):**

- Layout finale confermato via screenshot: Scarti e Mazzo adiacenti; "Carta Selezionata"
  renderizza un riquadro con bordo superiore colorato (verde per Colpo d'Apertura, SEQ_START),
  icona, nome, badge costo arrotondato, etichetta origine ("BASE"), riga effetti tra due
  separatori, descrizione estesa leggibile; Bandite è un singolo pulsante che mostra
  "🚫 Gestione carte bandite: pagina dedicata non ancora disponibile." nell'ErrorBar al click.
- Nessuna modifica al wire-contract o ai comandi GM override esistenti: la selezione carta
  resta puramente un effetto di presentazione client-side (stesso stato hover/focus di prima,
  solo rinominato per chiarezza semantica e promosso a riquadro dedicato).
- `npm run build` pulito (`tsc -b && vite build`).

**Notes:**
- Scoperta di automazione: simulare hover con `element.dispatchEvent(new MouseEvent
  ('mouseenter', {bubbles:true}))` non è affidabile con la delega eventi di React 19 (può
  risultare in nessun aggiornamento, o in un target diverso da quello atteso) — il modo
  affidabile per simulare hover in test/automazione è un movimento mouse reale:
  `page.mouse.move(x, y)` calcolato da `getBoundingClientRect()`, oppure `locator.hover()`
  (che però può attraversare/hover-are elementi intermedi lungo il tragitto, quindi preferire
  `page.mouse.move` diretto quando la precisione sull'elemento esatto conta).
- Bandite resta volutamente senza una vera pagina di gestione in questa fase (richiesta
  esplicita "per ora"): `onOpenBanish` è già il punto di aggancio pronto per una futura Phase
  dedicata (nuova route/pagina + comandi engine, se/quando il gioco userà mai bandire carte).

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
