# Tic-Tac-Toe WebApp — Development Plan

**Version:** 0.4.1
**Status:** Phase 6 – Complete ✅ (Phase 3 – Complete ✅; Phase 2 – Complete ✅, completata
retroattivamente a livello di libreria dopo la Phase 3 — vedi Notes; Phase 4/5 restano ⏳)
**Language:** Python 3.11+ (FastAPI) + TypeScript 5 / React 18 (frontend) — polyglot web layer.
The existing C++17 CoreEngine (`gmTris`, see `../PLAN.md`) gained an OPTIONAL, backward-compatible
CLI port override (`--events-port`/`--commands-port`) in Phase 2 — see that phase's Notes.
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

Albero **effettivo** dopo le Fasi 1, 2 e 3 + refactor "WebGUI_Lib". Phase 2 (Multi-Session & Auth)
è stata costruita **a livello di libreria condivisa** in `pyLib/gmWebServe` (multi-sessione, porte
dinamiche, auth pilot-grade) e `webLib/WebGUI_Lib` (login/token), non solo per Tris:

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
│           │   ├── restClient.ts    ← createSession/listSessions/sendCommand/closeSession (bearer token)
│           │   ├── wsClient.ts      ← connectSessionEvents(token, sessionId, ...) — token via query param
│           │   ├── authClient.ts    ← login/logout/fetchCurrentUser (POST /auth/login, /auth/me) — Phase 2 ✅
│           │   ├── AuthProvider.tsx ← contesto React + localStorage ("stay logged in") — Phase 2 ✅
│           │   └── EnvelopeRouter.ts← pub/sub per typeId (+ wildcard '*') — dorsale dei "moduli"
│           ├── modules/
│           │   ├── GmGuiModule.ts   ← contratto GmGuiModuleDescriptor (equivalente IGmGuiModule)
│           │   └── useGmGuiModule.ts← hook React di sottoscrizione al router
│           ├── components/
│           │   ├── ErrorBar.tsx     ← barra messaggi/errori generica
│           │   ├── EventLog.tsx     ← log auto-scroll generico (ex MatchLog)
│           │   ├── ActorStatusBadges.tsx ← badge stato attore auto-sottoscritti (ex PlayerBadges)
│           │   ├── ThemeSelect.tsx  ← selettore tema generico (estratto da GameToolbar)
│           │   ├── LoginForm.tsx    ← form utente/password generico — Phase 2 ✅
│           │   └── JoinSessionForm.tsx ← form "entra con codice" generico — Phase 6 ✅
│           └── styles.css           ← CSS strutturale `gmgui-*`, guidato da variabili `--gm-*`
├── pyLib/gmWebServe/                  ← libreria condivisa Python (equivalente backend di WebGUI_Lib)
│   ├── engine_listener.py             ← EngineEventListener (Qt-free) + EngineSender (invariati da Phase 1)
│   ├── engine_process.py              ← spawn/kill di un engine (porte via `extra_args`, invariato da Phase 1)
│   ├── port_utils.py                  ← find_free_port() — allocazione dinamica porte — Phase 2 ✅
│   ├── auth.py                        ← hashing PBKDF2, AuthConfig/AuthService/TokenManager — Phase 2 ✅
│   ├── session_registry.py            ← SessionRegistry multi-sessione/multi-utente generico — Phase 2 ✅
│   │                                     (roles/participants/join_code/join_session — Phase 6 ✅)
│   ├── fastapi_deps.py                ← get_current_user/authenticate_ws (dependency condivisa) — Phase 2 ✅
│   ├── auth_router.py                 ← router `/auth` pronto all'uso (login/logout/me) — Phase 2 ✅
│   ├── tools/manage_users.py          ← CLI per creare/aggiornare utenti in auth_config.json — Phase 2 ✅
│   └── tests/                         ← test generici con un fake_engine.py (no build C++ richiesta)
└── GAME/Tic-Tac-Toe/WebApp/
    ├── PLAN.md                         ← questo file (single source of truth)
    ├── conftest.py                     ← rende eng_serve importabile per pytest
    ├── eng_serve/                      ← facciata nativa HTTP/WS (FastAPI)
    │   ├── main.py                     ← FastAPI app factory + lifespan, mount gmWebServe.auth_router + sessions,
    │   │                                 reaper idle-session periodico — Phase 2 ✅
    │   ├── session_manager.py          ← wrapper sottile Tris-specifico su gmWebServe.SessionRegistry — Phase 2 ✅
    │   │                                 (roles X/O, join_session, binding server-side player — Phase 6 ✅)
    │   ├── auth_config.json            ← utenti pilot-grade (hash+salt, mai password in chiaro) — Phase 2 ✅
    │   ├── routers/
    │   │   └── sessions.py             ← REST list/create/get/command/close + WS (tutto auth-gated) — Phase 2 ✅
    │   │                                 (+ POST /sessions/join — Phase 6 ✅)
    │   ├── settings.py                 ← config via pydantic-settings (.env) + auth_config_path
    │   ├── requirements.txt
    │   └── tests/
    │       └── test_session_e2e.py     ← E2E reale: login, isolamento 2 utenti, cap 429 — Phase 2 ✅
    └── webapp_frontend/                ← React + Vite + TypeScript — layer Tris-specifico
        ├── package.json
        ├── tsconfig.app.json            ← `paths`: alias `@webgui/*` + mapping react/react-dom → @types (vedi Notes)
        ├── vite.config.ts               ← dev proxy verso eng_serve (REST + WS + /auth) + alias `@webgui` + Vitest
        ├── src/
        │   ├── main.tsx                 ← avvolge `<App/>` in `AuthProvider` — Phase 2 ✅
        │   ├── App.tsx                  ← login gate + picker sessioni (Riprendi/Chiudi) + board; consuma `@webgui/*`
        │   ├── App.test.tsx             ← Vitest + Testing Library (login form, errore, picker sessioni)
        │   ├── engine/
        │   │   ├── contract.ts          ← typeId/payload gmTris + PLAYER_ACTORS/resolveTrisBadge + linee vincenti
        │   │   └── gameState.ts         ← stato partita + reducer (porta 1:1 gli handler di tris_window.py)
        │   └── components/
        │       ├── TrisBoard.tsx        ← griglia 3×3 cliccabile (Tris-specifico)
        │       ├── TurnHeader.tsx       ← banner turno/vittoria/pareggio (Tris-specifico)
        │       └── GameToolbar.tsx      ← Nuova Partita + modalità (usa `ThemeSelect` da `@webgui/*`)
        └── (routing dedicato `/login`/`/sessions/:id` valutato e NON adottato — vedi Phase 2 Notes)
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

### Phase 2 — Multi-Session & Auth [✅]

- [x] `session_manager.py`: registro sessioni attive (session_id → processo + porte + owner)
- [x] Allocazione dinamica di porte libere per ogni nuova sessione (niente più porte fisse)
- [x] Cleanup: kill del processo e rilascio porte su logout/disconnessione/timeout di inattività
- [x] `routers/auth.py`: login pilot-grade (utenti fissi o in-memory) → token di sessione
- [x] Isolamento: un token vede/comanda solo le proprie sessioni
- [x] Smoke test: due sessioni concorrenti isolate (l'utente A non vede eventi/stato dell'utente B)

**Notes:**
- **Costruita a livello di libreria condivisa, non solo per Tris** (richiesta esplicita utente,
  2026-07-18): tutta la logica multi-sessione/auth vive in `pyLib/gmWebServe` (`SessionRegistry`,
  `auth.py`, `fastapi_deps.py`, `auth_router.py`) e in `webLib/WebGUI_Lib` (`authClient.ts`,
  `AuthProvider.tsx`, `LoginForm.tsx`, `restClient.ts`/`wsClient.ts` ora bearer-token-aware).
  Tris è il primo consumatore/pilota; Eldhôm (che ha la stessa sezione Phase 2 "rimandata" nel
  proprio PLAN.md) può adottarla riusando la libreria quasi invariata — non ancora fatto in
  questo turno (scope di questo turno: solo Tris, per decisione esplicita dell'utente).
- **Decisione presa con l'utente (2026-07-18)**: autenticazione "utenti fissi + password con
  hash/salt + token opachi" (non solo username); limiti sessione **N=2 sessioni concorrenti per
  utente, timeout di inattività 10 minuti**, entrambi configurabili via file (non hardcoded);
  login UI reale (non solo token in localStorage); modifica minima e retrocompatibile al C++
  del CoreEngine per abilitare porte dinamiche.
- **Modello sessione INVARIATO rispetto al piano originale**: resta "un processo per sessione"
  (process-per-session), ora davvero concorrente per N sessioni/utente — non viola "engine C++
  invariato" nel senso forte (nessuna riscrittura di TrisEngine/CmdServer), ma richiede la
  modifica minima descritta sotto.
- **Blocco delle porte fisse risolto**: `GAME/Tic-Tac-Toe/CoreEngine/main.cpp` accetta ora
  argomenti CLI opzionali `--events-port <port>` / `--commands-port <port>` (parsing manuale,
  nessuna dipendenza esterna), con fallback a `gmTris::ports::EVENTS`/`COMMANDS` se assenti —
  **eseguire l'exe senza argomenti (come fa oggi la GUI desktop) resta bit-per-bit invariato**.
  `TrisEngine` guadagna un parametro opzionale `events_port` (default `ports::EVENTS`) per
  poter passare la porta risolta a `GuiBridge`. Verificato: build pulita + `ctest -R "tris_"`
  4/4 PASS (incluso `tris_e2e`, che lancia l'exe SENZA argomenti — prova diretta che il
  comportamento di default non è cambiato).
- **Allocazione porte**: `gmWebServe.port_utils.find_free_port()` (bind su porta 0, legge la
  porta assegnata dal SO, richiude) — tecnica standard "trova porta libera", piccola race
  intrinseca accettata per uso pilot-grade locale (documentata nel docstring).
- **Auth pilot-grade ma OWASP-consapevole**: password mai in chiaro (PBKDF2-HMAC-SHA256,
  600.000 iterazioni, salt casuale 16 byte per utente, confronto a tempo costante via
  `hmac.compare_digest`); hash "fantasma" calcolato anche per username sconosciuti per ridurre
  (non eliminare) il segnale di timing su username-enumeration; token opachi
  `secrets.token_urlsafe(32)` con scadenza (default 8h); `SessionNotFoundError` unica sia per
  "non esiste" sia per "esiste ma di un altro utente" (mai confermare l'esistenza altrui, evita
  IDOR/information disclosure — OWASP A01). Fuori scope dichiarato: registrazione, reset
  password, refresh token, rate-limiting sui login — da rivalutare oltre il prototipo.
- **Config utenti/limiti in file, non hardcoded** (richiesta esplicita utente): nuovo
  `eng_serve/auth_config.json` (JSON con `users[]`, `max_sessions_per_user`,
  `session_idle_timeout_seconds`), generato/aggiornato con la nuova CLI
  `python -m gmWebServe.tools.manage_users --config ... --username ... --password ...`
  (mai scrive la password in chiaro, solo hash+salt). Due utenti pilota creati: `demo` e
  `demo2` (necessari per validare l'isolamento cross-utente).
- **WebSocket + token**: gli handshake WS da browser non possono impostare header custom, quindi
  il token viaggia come query param `?token=...` (`gmWebServe.fastapi_deps.authenticate_ws`,
  `wsClient.ts` aggiornato) — limite noto/accettato (il token può finire nei log di accesso),
  documentato nel codice.
- **Reaper idle-timeout**: task asyncio in background nel lifespan di `main.py`
  (`_reap_idle_sessions_periodically`, ogni 60s) chiama `SessionRegistry.reap_idle_sessions()`
  in un worker thread (`run_in_executor`) per non bloccare l'event loop durante lo stop di un
  eventuale subprocess. L'"inattività" è definita come "nessun comando inviato dall'utente" (non
  conta il traffico eventi in arrivo dal motore) — scelta deliberata per rispecchiare
  l'inattività dell'UTENTE, non quella del motore.
- **Routing `react-router-dom` valutato e NON adottato**: si è preferito estendere la singola
  pagina `App.tsx` con rendering condizionale (login → picker sessioni → board) invece di
  introdurre `/login`+`/sessions/:id`, per coerenza con la Phase 3 (nessuna logica di dominio
  duplicata, un solo componente orchestratore) e perché con un tetto di 2 sessioni/utente un
  semplice picker inline è sufficiente — `react-router-dom` resta installato ma inutilizzato,
  rivalutare se servirà deep-linking a una sessione specifica.
- **Test**: `pyLib/gmWebServe/tests/` (NUOVO, 18 test) valida `SessionRegistry`/`auth` in modo
  totalmente game-agnostic contro un `fake_engine.py` (stesso wire-format dei motori reali, zero
  build C++ richiesta) — isolamento proprietario, cap per-utente, reap idle, hashing/token/login.
  `eng_serve/tests/test_session_e2e.py` riscritto per Phase 2 (6 test) contro `tris_engine.exe`
  **reale**: login, 401 senza auth, isolamento tra `demo`/`demo2` con 2 processi motore reali
  concorrenti, cap 429 al terzo tentativo. Frontend: `App.test.tsx` riscritto (3 test Vitest,
  `fetch` mockato) per il nuovo login gate/picker. `npm run build`/`lint`/`test` puliti.
- **Smoke test end-to-end nel browser reale** (non solo automatico): login `demo` → «Sessioni
  attive: nessuna» → «Nuova Partita» → board reale con `tris_engine.exe` → mossa X in (1,1) →
  turno passato a O (round-trip completo browser↔eng_serve↔engine) → logout → login `demo2` →
  confermato **«Nessuna sessione attiva»** (demo2 NON vede la partita di demo, isolamento reale
  confermato a schermo, non solo via API).

### Phase 3 — Frontend Functional Parity [✅]

- [x] `TrisBoard.tsx`: griglia 3×3 cliccabile, invio comando `gmTris.move` via REST
- [x] `SessionDashboard` (in `App.tsx` + `TurnHeader`/`PlayerBadges`/`MatchLog`/`ErrorBar`): turno attivo / stato partita / log — equivalenti read-only ai widget `TurnHeaderWidget`/`TurnFooterWidget`/`LogWidget`/`ErrorBarWidget` della GUI desktop
- [x] Tema: `src/theme/themes.ts` genera CSS custom properties dagli stessi 5 temi (scroll/stone/dark_moon/blood/techno) + selettore tema live in toolbar
- [x] Routing `react-router-dom`: `/login`, `/sessions/:id` — valutato in Phase 2 e **deliberatamente non adottato** (login gate + picker sessioni via rendering condizionato nello stesso `App.tsx`; vedi Phase 2 Notes per il confronto)
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

### Phase 6 — Shared Multiplayer (Join Code) & Font Theming Backport [✅]

- [x] `pyLib/gmWebServe/session_registry.py`: sessioni multi-partecipante (`roles`, `participants`, `join_code`), `join_session()`, `SessionFullError`
- [x] `eng_serve/session_manager.py`: ruoli Tris (`X`/`O`), `join_session()`, binding server-side del campo `player` al ruolo reale del chiamante (anti-spoofing)
- [x] `eng_serve/routers/sessions.py`: endpoint `POST /sessions/join`, `SessionInfo` esteso (`join_code`/`roles`/`your_role`)
- [x] Frontend: `JoinSessionForm` (nuovo componente condiviso in `webLib/WebGUI_Lib`), picker sessioni con codice/ruoli, banner di ruolo in partita, polling "attesa avversario"
- [x] Gating turni lato frontend: la board si disabilita se non è il turno del ruolo dell'utente corrente
- [x] Font theming: `themeToCssVars()` imposta anche `fontFamily` reale (non solo custom property), regola globale `h1..h6` nello `styles.css` condiviso
- [x] Smoke test: 2 utenti reali (browser separati) creano/joinano/giocano la STESSA partita end-to-end

**Notes:**
- **Origine della richiesta**: l'utente ha chiarito che "Multi-User" non significa sessioni isolate
  per utente (già ottenuto in Phase 2), ma **due utenti che pilotano lo stesso match condiviso**
  tramite un codice di invito ("User Demo avvia una partita... darà un codice GameABCnnn a Demo2").
  Costruito di nuovo a livello di libreria (`pyLib/gmWebServe` + `webLib/WebGUI_Lib`), non solo
  per Tris, coerentemente col principio già seguito in Phase 2.
- **Modello dati**: `GameSession` sostituisce il concetto di singolo `owner` con `roles: tuple[str,...]`
  (es. `("X","O")`) e `participants: dict[str, str|None]` (ruolo → username o `None` se libero).
  `owner` resta per audit/back-compat ma i controlli di accesso usano `is_participant(username)`.
- **Join code**: 6 caratteri, alfabeto `ABCDEFGHJKLMNPQRSTUVWXYZ23456789` (esclude I/O/0/1
  ambigui), generato per sessione e indicizzato in `_session_id_by_code` per lookup O(1).
  `join_session()` è idempotente per chi è già partecipante, alza `SessionFullError` (→409) se
  tutti i ruoli sono occupati, e conta comunque contro il **cap sessioni del joiner**
  (`SessionLimitExceededError`→429) — scelta deliberata per coerenza con Phase 2.
- **Fix di sicurezza (OWASP A01, Broken Access Control)**: `SessionManager.send_command()` di
  Tris ora **sovrascrive sempre** il campo `player` di `gmTris.move` con il ruolo reale
  server-side del chiamante (`session.role_of(username)`), ignorando qualsiasi valore inviato
  dal client — impedisce a "O" di forgiare mosse come "X" (o viceversa). Validato con un test E2E
  che invia deliberatamente un `player` forgiato e verifica che il motore riceva il ruolo corretto.
- **UX "attesa avversario"**: il motore C++ non emette un evento nativo "qualcuno si è unito"
  (il join è un concetto puro di `eng_serve`/registry, invisibile al motore). Il frontend fa
  polling (`GET /sessions/{id}`, ogni 3s) finché un ruolo resta libero, poi si ferma da solo.
  Verificato dal vivo: il banner "in attesa dell'avversario" sparisce entro ~3s dall'ingresso
  del secondo utente, senza reload di pagina.
- **Font theming — bug reale trovato**: `themeToCssVars()` impostava SOLO le custom property
  `--gm-font-*`, mai una vera proprietà CSS `fontFamily` — il tema Eldhôm "sembrava" più curato
  solo perché il CSS *locale* di Eldhôm referenziava esplicitamente `var(--gm-font-body)`, mentre
  quello di Tris no. Fix nella libreria condivisa (non nel gioco): `themeToCssVars()` ora
  restituisce anche `fontFamily: theme.bodyFont`, ereditato via CSS da tutti i discendenti senza
  bisogno di CSS per-gioco; `h1..h6` e la nuova classe opt-in `.gmgui-display-font` usano il
  display font tramite una regola globale in `webLib/WebGUI_Lib/src/styles.css`.
- **Board 1-indicizzata**: gotcha di test (non di prodotto) riscoperto scrivendo il nuovo test
  E2E a 2 utenti — vedi memoria di repository `tris-webapp-plan.md` per i dettagli.
- **Validato**: 25 test `pyLib/gmWebServe/tests` (17 su `SessionRegistry`, incl. 7 nuovi su
  join/ruoli) + 8 test `eng_serve/tests/test_session_e2e.py` (incl. 2 nuovi, contro
  `tris_engine.exe` reale) + 3 test Vitest frontend, tutti verdi. `npm run build` pulito.
- **Smoke test manuale nel browser reale, 2 sessioni indipendenti** (non solo automatico):
  login `demo` → Nuova Partita (ruolo X, codice mostrato) → login `demo2` in una pagina separata →
  «Entra in una partita» col codice → ruolo O assegnato, board condivisa confermata a schermo →
  mossa X in (1,1) vista **live** (via WebSocket) sulla pagina di demo2 senza refresh → mossa O
  in (2,2) vista live sulla pagina di demo → turni alternati e board disabilitata/abilitata
  correttamente per ciascun ruolo → chiusura sessione da un partecipante propagata correttamente
  (verificata risposta server `204`, poi `404` sul secondo tentativo di chiusura, gestito senza
  errori lato UI).
- **Fuori scope, segnalato ma non risolto in questo turno**: `GAME/Eldhom/WebApp/webapp_frontend`
  ha una build TypeScript rotta **preesistente** (4 errori — `App.tsx` chiama ancora le vecchie
  firme pre-Phase-2 di `createSession`/`connectSessionEvents`/`sendCommand` senza token), NON
  causata da questo turno (confermato via `git status` vuoto su `GAME/Eldhom`) — Eldhôm Phase 2
  è deliberatamente rimandata (vedi `eldhom-webapp-plan.md`); da portare quando richiesto.

**Addendum "Sfondo tematico di pagina + contrasto testi" (2026-07-19), stesso giorno:**
richiesta esplicita dell'utente — lo sfondo/le sfumature per-tema esistevano SOLO in Eldhôm
(`App.css` locale, mai promosse a libreria), esattamente lo stesso pattern di "libreria non
davvero condivisa" già corretto per i font. Estratto in `webLib/WebGUI_Lib/src/styles.css`
un nuovo blocco opt-in `.gmgui-theme-backdrop[data-theme='...']` (5 regole, gradient portati
1:1 dallo sfondo pagina di Eldhôm Phase 13/14; Dark Moon usa la variante "Crepuscolo" come
default condiviso — il selettore-varianti resta un arricchimento locale di Eldhôm). Tris
aggiunge `data-theme={themeId}` + la classe `gmgui-theme-backdrop` sulla propria `<div
className="app">` (in tutti e 3 gli stati: restoring/login/gioco) per ereditarlo gratis.
- **Bug di contrasto trovato e corretto durante la verifica visiva**: applicare lo sfondo
  scuro a `.app` ha rotto il contrasto di testi che poggiano DIRETTAMENTE su di esso (non
  dentro un pannello opaco `--gm-panel`) per i temi "chiari" Scroll/Stone, il cui `--gm-text`
  è pensato per contrastare `--gm-background` (chiaro), non il nuovo sfondo scuro. Fix: nuova
  custom property `--gm-backdrop-text` (= `var(--gm-accent)`) definita dentro ciascuna delle 5
  regole `.gmgui-theme-backdrop[data-theme='...']` — mirror esatto del pattern già validato in
  Eldhôm (`.app-header h1 { color: var(--gm-accent); text-shadow: ...; }`), ma promosso a
  variabile di libreria riusabile invece di essere ripetuto come valore letterale in ogni gioco.
  Applicato in Tris a `.app-header h1`, `.app-header__account` (username) e `.tris-role-banner`
  (gli unici 3 punti che non vivono dentro un pannello `--gm-panel`); tutto il resto (h2
  "Sessioni attive", "Turno di: Player X", "Tic-Tac-Toe — Accedi") viveva già dentro un
  pannello opaco (`.gmgui-session-picker`/`.turn-header`/`.gmgui-login-form`, tutti
  `background-color: var(--gm-panel)`) quindi il solo `--gm-text` bastava.
- **Causa radice separata e più grave trovata durante l'indagine**: `webapp_frontend/src/
  index.css` conteneva ~110 righe di CSS morto ereditate da UN ALTRO template Vite (variabili
  `--text`/`--heading`/`--accent`/`#social`/`.counter`, mai referenziate da nessun componente
  Tris — verificato via grep prima di rimuovere, stesso approccio già usato per la pulizia di
  `App.css` in Phase 3) con una regola `h1, h2 { color: var(--text-h); }` che sovrascriveva
  SEMPRE il colore di OGNI h1/h2 della pagina con un valore statico legato al
  `prefers-color-scheme` del sistema operativo, non al tema `gmGui` selezionato — la causa
  principale per cui "nessun tema" mostrava colori giusti sui titoli. Ridotto a
  `body { margin: 0; background: <gradiente scuro neutro>; min-height: 100vh; }` (stesso valore
  di fallback già usato da Eldhôm in `index.css`, così l'area attorno a `.app` non mostra più
  uno sfondo bianco su schermi più alti del contenuto).
- **Validato visivamente nel browser reale su tutti e 5 i temi + schermata di login**
  (screenshot per ciascuno): Scroll/Stone/Dark Moon/Blood/Techno tutti con titoli, username,
  banner di ruolo e pannelli leggibili; `npm run build`/`npm run test` (3/3) verdi dopo ogni
  round di modifica.

---

## Key Design Decisions


1. **Pilota:** `gmTris` (Tic-Tac-Toe) scelto per maturità (4 fasi CoreEngine complete, wire-contract stabile, E2E testato) e semplicità di dominio, per validare l'architettura web a basso rischio.
2. **Convivenza permanente:** la GUI PySide6 esistente resta invariata e autonoma; la WebApp è un canale aggiuntivo, non un rimpiazzo.
3. **Modello utenza:** multi-utente/remoto → richiede isolamento per sessione e autenticazione (pilot-grade in questa fase).
4. **Protocollo:** `eng_serve` è una facciata nativa HTTP/WebSocket, non un semplice traduttore ad-hoc — pensata come passo verso una futura unificazione del trasporto desktop+web, lasciando `tris_engine.exe` totalmente invariato.
5. **Isolamento sessione:** un processo `tris_engine.exe` per sessione (non un engine multi-sessione), per non violare "engine C++ invariato" già rispettato in tutte le fasi precedenti.
