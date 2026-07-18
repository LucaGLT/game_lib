# Tic-Tac-Toe WebApp — Development Plan

**Version:** 0.2.0
**Status:** Phase 3 – Complete ✅ (Phase 2 – Deliberately deferred ⏸️, single-user resta come nel pyQt)
**Language:** Python 3.11+ (FastAPI) + TypeScript 5 / React 18 (frontend) — polyglot web layer.
The existing C++17 CoreEngine (`gmTris`, see `../PLAN.md`) is **unchanged**.
**Namespace:** N/A (no C++ namespace) — Python package `eng_serve`, frontend app `webapp_frontend`.
Reuses the `gmTris` wire-contract (typeId/payload) unchanged.

---

## Goal

Espone il gioco Tic-Tac-Toe (`gmTris`) anche come **WebApp**, in aggiunta — non in
sostituzione — alla GUI desktop PySide6 già esistente e stabile
(`GAME/Tic-Tac-Toe/GUI`). Il pilota è stato scelto perché `gmTris` è il modulo più
maturo del repository (4 fasi CoreEngine complete, wire-contract stabile, E2E
testato via CTest), quindi permette di validare l'architettura web a basso rischio
prima di estenderla ad altri giochi.

Decisione architetturale chiave: invece di far tradurre il protocollo TCP/JSON
esistente da un gateway ad-hoc, il CoreEngine si arricchisce di una **facciata di
rete nativa HTTP/WebSocket** (`eng_serve`) pensata come punto di ingresso di rete
"ufficiale" e futuro, mantenendo `tris_engine.exe` totalmente invariato. Il target
d'uso è **multi-utente/remoto**: ogni utente autenticato ottiene una propria
sessione isolata, con un processo `tris_engine.exe` dedicato (process-per-session),
coerente con il principio "engine C++ invariato" già rispettato in tutte le fasi
precedenti di `gmTris`.

### Confronto strategie di comunicazione valutate

| Strategia | Pro | Contro | Scelta |
|---|---|---|---|
| Gateway traduce il protocollo TCP/JSON esistente (nessuna facciata nativa) | Minimo sforzo iniziale | Resta legato a TCP grezzo; nessun percorso di unificazione futura con il desktop | ❌ |
| `eng_serve`: facciata HTTP/WS nativa, `tris_engine.exe` invariato | API di rete moderna e documentata; percorso di unificazione futura desktop+web; zero rischio sul C++ già testato | Richiede un nuovo servizio dedicato da costruire/mantenere | ✅ |
| Riscrivere `tris_engine.exe` per parlare HTTP/WS nativamente in C++ | Nessun processo intermedio | Viola "engine C++ invariato"; alto rischio di regressione su un engine stabile (4/4 CTest PASS) | ❌ |

### Confronto modelli di sessione valutati

| Modello | Pro | Contro | Scelta |
|---|---|---|---|
| Un `tris_engine.exe` per sessione (process-per-session) | Isolamento totale; riusa `CmdServer`/`IpSocketChannel` senza modifiche; coerente con "engine C++ invariato" | Più processi attivi = più RAM/handle da gestire | ✅ |
| Un solo `tris_engine.exe` multi-sessione (engine "session-aware") | Meno processi | Richiede riscrivere `TrisEngine`/`CmdServer` per N stati paralleli — viola "engine C++ invariato" | ❌ |

---

## Architecture

```text
┌────────────────────────────── Browser (N utenti) ───────────────────────────────┐
│  React SPA (webapp_frontend)                                                    │
│    ├─ /login          : form autenticazione (pilot-grade token)                 │
│    ├─ /sessions/:id   : Board 3×3 cliccabile + dashboard (turno/stato/log)       │
│    ├─ api/restClient.ts : fetch verso REST (comandi/query)                      │
│    └─ api/wsClient.ts   : WebSocket verso /sessions/:id/ws (eventi realtime)     │
└───────────────▲───────────────────────────────────────────────┬─────────────────┘
       eventi    │ WebSocket (JSON, stesso typeId/payload di gmTris)  comandi/query │ REST (HTTP)
┌───────────────┴───────────────────────────────────────────────▼─────────────────┐
│  eng_serve — facciata nativa HTTP/WS (FastAPI, NUOVO processo Python)            │
│    ├─ routers/auth.py     : login pilot-grade → token di sessione utente         │
│    ├─ routers/sessions.py : POST crea sessione / GET stato / POST comando        │
│    ├─ session_manager.py  : registro sessioni attive, cleanup/TTL,               │
│    │                        1 utente = 1 sessione = 1 processo motore            │
│    ├─ engine_process.py   : spawn/kill di un tris_engine.exe per sessione,       │
│    │                        alloca porte libere dinamiche                        │
│    └─ bridge_client.py    : riusa pyLib/gmGui/engine_bridge (framing/sender/     │
│                              receiver) — stesso ruolo della GUI PySide6 oggi     │
└───────────────▲───────────────────────────────────────────────┬─────────────────┘
       eventi    │ TCP + frame JSON (porta evento per-sessione)   comandi          │ TCP (porta comando per-sessione)
                 │        [protocollo INVARIATO — identico a quello della GUI]     │
┌───────────────┴───────────────────────────────────────────────▼─────────────────┐
│  Pool di processi tris_engine.exe (uno per sessione attiva) — C++17 INVARIATO    │
│  TrisEngine (Facade+Mediator) su gmMap/gmActor/gmFlow/gmRules/gmAlea/gmDispatch  │
└───────────────────────────────────────────────────────────────────────────────────┘

Nota: la GUI PySide6 esistente (GAME/Tic-Tac-Toe/GUI) continua a funzionare
INVARIATA, connettendosi direttamente a una propria istanza di tris_engine.exe
sulle porte fisse 9100/9001 — coesistenza permanente, nessun impatto.
```

---

## File Structure

Albero **effettivo** dopo le Fasi 1 e 3 + refactor "WebGUI_Lib" (`routers/auth.py`, `routes/`,
porte dinamiche restano scope della Phase 2, non ancora creati):

```text
game_lib/
├── webLib/
│   └── WebGUI_Lib/                  ← libreria condivisa generica (equivalente web di pyLib/gmGui)
│       ├── package.json             ← identità/manifest (consumata come sorgente TS, no build/publish)
│       └── src/
│           ├── index.ts             ← barrel export (superficie pubblica documentata)
│           ├── theme/
│           │   └── themes.ts        ← 5 temi (CSS custom properties, fedeli a theme_manager.py)
│           ├── session/
│           │   ├── types.ts         ← EngineEnvelope/EnvelopeHandler/SessionInfo generici
│           │   ├── restClient.ts    ← createSession(payload)/sendCommand generici (no starterMode Tris-specifico)
│           │   ├── wsClient.ts      ← connectSessionEvents generico
│           │   └── EnvelopeRouter.ts← pub/sub per typeId (+ wildcard '*') — dorsale dei "moduli"
│           ├── modules/
│           │   ├── GmGuiModule.ts   ← contratto GmGuiModuleDescriptor (equivalente IGmGuiModule)
│           │   └── useGmGuiModule.ts← hook React di sottoscrizione al router
│           ├── components/
│           │   ├── ErrorBar.tsx     ← barra messaggi/errori generica
│           │   ├── EventLog.tsx     ← log auto-scroll generico (ex MatchLog)
│           │   ├── ActorStatusBadges.tsx ← badge stato attore auto-sottoscritti (ex PlayerBadges)
│           │   └── ThemeSelect.tsx  ← selettore tema generico (estratto da GameToolbar)
│           └── styles.css           ← CSS strutturale `gmgui-*`, guidato da variabili `--gm-*`
└── GAME/Tic-Tac-Toe/WebApp/
    ├── PLAN.md                         ← questo file (single source of truth)
    ├── conftest.py                     ← rende eng_serve importabile per pytest
    ├── eng_serve/                      ← facciata nativa HTTP/WS (FastAPI)
    │   ├── main.py                     ← FastAPI app factory + lifespan, mount router
    │   ├── session_manager.py          ← sessione singola "dev-session" (Phase 2: registro multi-sessione)
    │   ├── engine_process.py           ← spawn/kill di UN tris_engine.exe (porte fisse 9100/9001)
    │   ├── bridge_client.py            ← EngineEventListener (Qt-free) + EngineSender (riusati da pyLib/gmGui/engine_bridge)
    │   ├── routers/
    │   │   └── sessions.py             ← REST comandi/query + endpoint WebSocket eventi (auth.py: Phase 2)
    │   ├── settings.py                 ← config via pydantic-settings (.env)
    │   ├── requirements.txt
    │   └── tests/
    │       └── test_session_e2e.py     ← E2E con tris_engine.exe reale (new_game→snapshot→move→cell_changed)
    └── webapp_frontend/                ← React + Vite + TypeScript — layer Tris-specifico
        ├── package.json
        ├── tsconfig.app.json            ← `paths`: alias `@webgui/*` + mapping react/react-dom → @types (vedi Notes)
        ├── vite.config.ts               ← dev proxy verso eng_serve (REST + WS) + alias `@webgui` + config Vitest
        ├── src/
        │   ├── main.tsx
        │   ├── App.tsx                  ← pagina unica (routing multi-sessione: Phase 2); consuma `@webgui/*`
        │   ├── App.test.tsx             ← Vitest + Testing Library
        │   ├── engine/
        │   │   ├── contract.ts          ← typeId/payload gmTris + PLAYER_ACTORS/resolveTrisBadge + linee vincenti
        │   │   └── gameState.ts         ← stato partita + reducer (porta 1:1 gli handler di tris_window.py)
        │   └── components/
        │       ├── TrisBoard.tsx        ← griglia 3×3 cliccabile (Tris-specifico)
        │       ├── TurnHeader.tsx       ← banner turno/vittoria/pareggio (Tris-specifico)
        │       └── GameToolbar.tsx      ← Nuova Partita + modalità (usa `ThemeSelect` da `@webgui/*`)
        └── (routes/, tests/ dedicati: da introdurre in Phase 2 con multi-sessione)
```

---


## Development Phases

### Phase 1 — Interfaces & Stubs [✅]

- [x] Scaffold `eng_serve/` (FastAPI app, health-check endpoint, `requirements.txt`, `pydantic-settings`)
- [x] `engine_process.py`: spawn di UN SINGOLO `tris_engine.exe` su porte fisse di sviluppo, stessa lazy-connect logic della GUI
- [x] `bridge_client.py`: riuso diretto di `pyLib/gmGui/engine_bridge` (framing/sender) senza duplicare il wire format
- [x] Endpoint REST minimo: `POST /sessions` (sessione singola, no auth) + `POST /sessions/{id}/command` (inoltra `gmTris.new_game` / `gmTris.move`)
- [x] Endpoint WebSocket minimo: `/sessions/{id}/ws`, inoltra 1:1 gli eventi ricevuti dal motore (stesso typeId/payload)
- [x] Scaffold `webapp_frontend/` (Vite + React + TS): una pagina che mostra il JSON grezzo degli eventi ricevuti via WS
- [x] Smoke test: sessione singola end-to-end — `new_game` → snapshot → una mossa → evento `cell_changed` visibile nel browser

**Notes:**
- Nessuna modifica a `tris_engine.exe`: `eng_serve` si comporta esattamente come la GUI PySide6 oggi (client TCP sulle stesse due porte), solo che le ri-espone come WS/REST.
- Nessuna autenticazione in questa fase: lo scope di Phase 1 è validare la traduzione di protocollo, non la sicurezza.
- **Scoperta importante:** le porte evento/comando (9100/9001) sono `constexpr` hardcoded in `CoreEngine/engine/TrisTypes.hpp`, non configurabili a runtime. `eng_serve/settings.py` usa questi valori fissi come default. **Implicazione per Phase 2:** l'allocazione dinamica di porte per-sessione richiede una modifica C++ a `TrisTypes.hpp` (o un'analoga strategia, es. wrapper/parametro CLI) — da pianificare esplicitamente all'inizio della Phase 2.
- `engine_bridge.receiver.EngineReceiver` (GUI) è un `PySide6.QtCore.QThread` e non è riusabile in un processo senza Qt: `bridge_client.py` introduce `EngineEventListener`, equivalente Qt-free su `threading.Thread`, che replica la stessa logica di normalizzazione payload (`headers.data` JSON string → `msg["data"]` dict). `EngineSender` invece è puro socket ed è stato riusato invariato.
- Test automatico `eng_serve/tests/test_session_e2e.py` verde contro l'eseguibile reale (`2 passed`): copre l'esatto criterio di accettazione (`new_game` → `gmMap.snapshot` → mossa → `gmMap.cell_changed`).
- Verifica manuale end-to-end aggiuntiva (backend `uvicorn` + `npm run dev` avviati insieme): sessione creata e mossa inviata tramite il **proxy dev di Vite** (`/sessions`, `/sessions/{id}/ws` con `ws: true` in `vite.config.ts`), confermando che REST e WebSocket attraversano correttamente il proxy fino al motore reale.
- Struttura frontend effettiva più semplice dell'albero aspirazionale in "File Structure": Phase 1 usa un unico `src/App.tsx` (pagina stub con log JSON grezzo) + `src/api/{restClient,wsClient}.ts` + `src/App.test.tsx` (Vitest/RTL). Lo split in `routes/`, `components/`, `theme/` è rimandato alla Phase 3, quando servirà una board reale e temi multipli.
- Frontend stack effettivamente installato (versioni correnti al momento dello scaffold, superiori a quelle indicative nella tabella di riferimento iniziale): React 19, Vite 8, Vitest 4, `react-router-dom` 7 — nessun impatto sul piano, `react-router-dom` non ancora usato (una sola pagina in Phase 1).
- `npm run build` e `npm run test` (Vitest) verdi.

### Phase 2 — Multi-Session & Auth [⏸️ Deliberatamente rimandata]

- [ ] `session_manager.py`: registro sessioni attive (session_id → processo + porte + owner)
- [ ] Allocazione dinamica di porte libere per ogni nuova sessione (niente più porte fisse)
- [ ] Cleanup: kill del processo e rilascio porte su logout/disconnessione/timeout di inattività
- [ ] `routers/auth.py`: login pilot-grade (utenti fissi o in-memory) → token di sessione
- [ ] Isolamento: un token vede/comanda solo le proprie sessioni
- [ ] Smoke test: due sessioni concorrenti isolate (l'utente A non vede eventi/stato dell'utente B)

**Notes:**
- **Decisione esplicita dell'utente (2026-07-17):** prima di affrontare il multi-utente, si resta
  con un solo utente locale che controlla entrambi i giocatori — esattamente come la GUI PySide6
  oggi. La Fase 3 (resa estetica) è stata sviluppata PRIMA della Fase 2, invertendo l'ordine
  originale del piano. Questa fase riparte da qui quando si formerà l'esigenza multi-utente.
- L'autenticazione di questa fase sarà "pilot-grade" (adeguata a validare l'isolamento, non per
  produzione) — da rivalutare se si va oltre il prototipo.
- Il modello "un processo per sessione" resta scelto esplicitamente per non violare
  "engine C++ invariato" (vedi Phase 1 Notes per il vincolo sulle porte hardcoded).

### Phase 3 — Frontend Functional Parity [✅]

- [x] `TrisBoard.tsx`: griglia 3×3 cliccabile, invio comando `gmTris.move` via REST
- [x] `SessionDashboard` (in `App.tsx` + `TurnHeader`/`PlayerBadges`/`MatchLog`/`ErrorBar`): turno attivo / stato partita / log — equivalenti read-only ai widget `TurnHeaderWidget`/`TurnFooterWidget`/`LogWidget`/`ErrorBarWidget` della GUI desktop
- [x] Tema: `src/theme/themes.ts` genera CSS custom properties dagli stessi 5 temi (scroll/stone/dark_moon/blood/techno) + selettore tema live in toolbar
- [ ] Routing `react-router-dom`: `/login`, `/sessions/:id` — **non necessario finché resta un solo utente/sessione** (Phase 2 deferred); rivalutare quando si introduce multi-sessione
- [x] Smoke test: partita completa giocabile nel browser (vittoria e pareggio) con cambio tema funzionante — verificato manualmente (due partite complete, vittorie X e O, cambio tema Scroll→Techno dal vivo)

**Notes:**
- Nessuna logica di dominio nel frontend: React consuma solo eventi/JSON (parsing tipizzato in
  `src/engine/contract.ts` + `src/engine/gameState.ts`), stessa filosofia "data-driven rendering"
  già seguita da `gmGui` lato desktop.
- **Contratto eventi ed etichette IT** riprodotti 1:1 dal riferimento desktop più completo
  (`GAME/Tic-Tac-Toe/GUI/app/tris_window.py`, non il più recente `TrisMainWindow`/`GmTrisBoardModule`
  che usa i moduli generici `gmGui` non ancora ispezionati in dettaglio): stessi testi
  ("Turno di: Player X (X)", "🏆 Ha vinto Player X!", "Player {mark} gioca in (row, col).", ecc.),
  stessa mappa `_LINE_CELLS` per l'evidenziazione della riga vincente.
- **Interazione utente semplificata rispetto allo stub di Phase 1**: rimosso il form manuale
  player/row/col ("Invia Mossa") — le mosse partono SOLO dal click sulla cella, esattamente come
  la GUI desktop (il giocatore attivo è determinato dall'evento engine `ACTIVE_TURN`, non scelto
  manualmente). Il pulsante "Nuova Partita" riusa la sessione già aperta (comando
  `gmTris.new_game`) invece di ricrearla, evitando l'errore 409 "already running" dal secondo
  avvio in poi — mirror esatto del bottone "Reload" desktop.
- **Discrepanza nota tra spec e runtime**: i colori usati (`src/theme/themes.ts`) sono presi dal
  file Python **realmente eseguito** (`pyLib/gmGui/theme_manager.py._THEMES`), che differisce
  leggermente dai valori dichiarativi in `.github/specs/gui-theme.yml` (es. scroll.background
  `#F3E9D2` runtime vs `#E8DFC8` spec). Scelta deliberata per fedeltà visiva 1:1 con ciò che gira
  oggi nella GUI desktop; da segnalare/armonizzare se la spec verrà aggiornata in futuro.
- Verifica manuale end-to-end nel browser reale (non solo Vitest): creata sessione, giocate due
  partite complete (vittoria O su colonna 3, vittoria X su colonna 1) con evidenziazione riga
  vincente corretta, badge giocatori aggiornati correttamente turno per turno, log partita
  coerente con i testi del riferimento desktop, cambio tema Scroll→Techno applicato
  istantaneamente senza reload di pagina (funzionalità che la GUI desktop non espone nemmeno
  via UI oggi — miglioramento reale rispetto al pyQt).
- `npm run build` e `npm run test` (Vitest+RTL, 3 test su App.tsx) verdi.

**Addendum "WebGUI_Lib" (2026-07-17), successivo al completamento di questa fase:**
richiesta esplicita dell'utente di estrarre SUBITO (non in futuro) le parti generiche/
game-agnostic del frontend in una libreria condivisa `webLib/WebGUI_Lib`, equivalente web
di `pyLib/gmGui`, con scope completo incluso un contratto "modulo" generico (mirror di
`IGmGuiModule`/`BaseModule` desktop). Vedi `webLib/WebGUI_Lib/` in "File Structure" sopra.

- Estratti in `webLib/WebGUI_Lib/src`: `theme/themes.ts`, `session/{types,restClient,wsClient}.ts`
  (generalizzati: `createSession` ora accetta un `Record<string, unknown>` arbitrario invece di
  un `starterMode: string` Tris-specifico), nuovo `session/EnvelopeRouter.ts` (pub/sub per typeId
  con supporto wildcard `'*'`), nuovo `modules/{GmGuiModule.ts,useGmGuiModule.ts}` (contratto +
  hook React), componenti generici `ErrorBar.tsx`/`EventLog.tsx` (ex `MatchLog.tsx`)/
  `ActorStatusBadges.tsx` (ex `PlayerBadges.tsx`, ora auto-sottoscritta al router invece di
  ricevere props)/`ThemeSelect.tsx` (estratto da `GameToolbar.tsx`), `styles.css` (classi
  `gmgui-*`, guidate da variabili `--gm-*`).
- Restano Tris-specifici in `webapp_frontend/src`: `App.tsx` (orchestratore), `engine/contract.ts`
  (typeId/payload + `PLAYER_ACTORS`/`resolveTrisBadge`), `engine/gameState.ts` (reducer, ora senza
  il campo `playerStatuses` — gestito autonomamente da `ActorStatusBadges`), `TrisBoard.tsx`,
  `TurnHeader.tsx`, `GameToolbar.tsx`.
- Import via alias `@webgui/*` (mappato sia in `tsconfig.app.json` `paths` sia in
  `vite.config.ts` `resolve.alias`), consumato come **sorgente TS grezzo**, non come pacchetto
  npm — nessun workspace/build/publish, scelta pragmatica per singolo consumatore.
- **Insidia tecnica risolta**: `webLib/WebGUI_Lib/src` non ha un `node_modules` proprio come
  antenato (vive fuori dall'albero di `webapp_frontend`), quindi `react`/`react-dom` non si
  risolvono per i file lì fisicamente residenti. Fix: `tsconfig.app.json` mappa esplicitamente
  `react`/`react/jsx-runtime`/`react-dom`/`react-dom/client` ai file `.d.ts` di
  `node_modules/@types/react(-dom)/...` di `webapp_frontend` (NON alla cartella del pacchetto
  runtime `node_modules/react` — quella non contiene tipi propri, causa altrimenti una
  regressione `TS7016`/`TS7026` su TUTTO il progetto, non solo su WebGUI_Lib, perché i `paths`
  di TypeScript sono globali alla compilazione). `vite.config.ts` mappa invece `react`/`react-dom`
  al pacchetto runtime reale (i due sistemi di risoluzione — `tsc` e Vite/esbuild — sono
  indipendenti: `paths` serve SOLO al type-checking, `resolve.alias` SOLO al bundling).
- `npm run build`, `npm run test` (Vitest, 3/3) e `npm run lint` (oxlint) verdi dopo il refactor.
  Verifica visiva nel browser condiviso: rendering corretto (temi, badge, log, error bar) tutti
  serviti da `@webgui/*`; il round-trip WebSocket/`EnvelopeRouter` live non è stato ri-verificato
  in questo turno perché il backend Phase 3 (porta 8000) era rimasto attivo dal turno precedente
  con la sessione singola `"dev-session"` già occupata (409 su una nuova `createSession` dopo
  reload pagina — limite noto del modello a sessione singola di Phase 1, non una regressione).

### Phase 4 — Hardening & Tests [⏳]

- [ ] Test backend `pytest` (unit su `session_manager` / `engine_process`)
- [ ] Test di integrazione backend con `tris_engine.exe` reale come subprocess (mirror di `tris_e2e` in CTest)
- [ ] Test frontend `Vitest` + `@testing-library/react` sui componenti principali
- [ ] Gestione riconnessione WebSocket lato frontend
- [ ] Guardia anti-leak: reaper periodico per sessioni/processi orfani
- [ ] Checklist di sicurezza minima: validazione Pydantic su ogni body REST, CORS esplicito, scadenza token
- [ ] Smoke test: suite completa verde (backend + frontend) in esecuzione locale

**Notes:**
- Stesso principio già usato per `tris_e2e`: i test di integrazione lanciano l'eseguibile reale, non un mock, per garantire fedeltà end-to-end.

### Phase 5 — Unificazione Desktop (opzionale, futura) [⏳]

- [ ] Valutare la migrazione del bridge PySide6 (`engine_bridge`) per parlare con `eng_serve` invece che con TCP grezzo diretto
- [ ] Procedere solo se i benefici di manutenzione superano il rischio di toccare una GUI desktop già stabile e testata
- [ ] Smoke test: GUI desktop invariata nel comportamento dopo l'eventuale migrazione

**Notes:**
- Fase esplicitamente opzionale/futura: la decisione "coesistenza permanente" non richiede questa unificazione per essere soddisfatta oggi.

---

## Key Design Decisions

1. **Pilota:** `gmTris` (Tic-Tac-Toe) scelto per maturità (4 fasi CoreEngine complete, wire-contract stabile, E2E testato) e semplicità di dominio, per validare l'architettura web a basso rischio.
2. **Convivenza permanente:** la GUI PySide6 esistente resta invariata e autonoma; la WebApp è un canale aggiuntivo, non un rimpiazzo.
3. **Modello utenza:** multi-utente/remoto → richiede isolamento per sessione e autenticazione (pilot-grade in questa fase).
4. **Protocollo:** `eng_serve` è una facciata nativa HTTP/WebSocket, non un semplice traduttore ad-hoc — pensata come passo verso una futura unificazione del trasporto desktop+web, lasciando `tris_engine.exe` totalmente invariato.
5. **Isolamento sessione:** un processo `tris_engine.exe` per sessione (non un engine multi-sessione), per non violare "engine C++ invariato" già rispettato in tutte le fasi precedenti.
