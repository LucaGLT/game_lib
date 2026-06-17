# Tic-Tac-Toe – Development Plan

**Version:** 0.2.0
**Status:** Phase 1 – Interfaces & Stubs ✅ · Phase 2 – Domain logic ✅ (Phase 3 next ⏳)
**Language:** C++17 Standard (CoreEngine) + Python 3 / PySide6 (GUI)
**Namespace:** `gmTris` (C++) / `gmtris_gui` (Python)

---

## Goal

Implementare il gioco Tic-Tac-Toe (Tris) come **due processi separati** che
comunicano via TCP: un **CoreEngine** in C++17 che riusa le librerie `gmXxx`
esistenti per tutta la logica di dominio, e una **GUI** PySide6 che presenta il
tabellone 3×3, lo stato dei turni e il log della partita. La scelta progettuale
principale è un pattern **Facade + Mediator**: una classe `TrisEngine` possiede
e coordina sottosistemi sottili (uno per libreria), evitando singleton e
mantenendo ogni libreria isolata in una propria cartella. La comunicazione
Engine↔GUI riusa il protocollo già implementato in `pyLib/gmGui/engine_bridge`
(frame `uint32` big-endian + payload JSON UTF-8), con eventi Engine→GUI sulla
porta 9100 e comandi GUI→Engine sulla porta 9001.

### Confronto pattern valutati

| Pattern | Pro | Contro | Scelta |
|---|---|---|---|
| Singleton service | Accesso globale semplice | Stato globale, test difficili | ❌ |
| DI container | Massima flessibilità | Over-engineering per un gioco piccolo | ❌ |
| Facade + Mediator (composizione) | Ownership chiara, testabile, libs isolate | Un po' di boilerplate wrapper | ✅ |

---

## Architecture

```text
┌──────────────────────────── Processo GUI (PySide6) ────────────────────────────┐
│  TrisWindow (layout da figura)                                                  │
│    ├─ Header   : Salvataggio / Reload                                           │
│    ├─ Body_Hdr : Stato del Giocatore di Turno   ← eventi gmActor/gmFlow         │
│    ├─ Body     : Board 3×3 (click cella)        → comando gmTris.move           │
│    ├─ R_Panel  : Log della partita              ← eventi gmLog/gmTris           │
│    ├─ Body_Ftr : Stato dei Turni                ← eventi gmFlow                 │
│    └─ Footer   : Messaggi di errore             ← eventi gmTris.invalid_move    │
│  engine_bridge: EngineReceiver(:9100)  EngineSender(:9001)   [riuso esistente]  │
└───────────────▲───────────────────────────────────────────────┬────────────────┘
        eventi   │ JSON frame (Engine→GUI :9100)        comandi   │ (GUI→Engine :9001)
┌───────────────┴───────────────────────────────────────────────▼────────────────┐
│  Processo CoreEngine (C++17)                                                     │
│  ┌──────────────────────────── TrisEngine (Facade + Mediator) ───────────────┐  │
│  │   start_game()    handle_command(typeId, json)    emit events             │  │
│  └──┬───────────┬──────────┬──────────┬──────────┬──────────┬────────────────┘  │
│     │           │          │          │          │          │                   │
│  board/      players/   flow/      rules/     alea/      logging/   bridge/      │
│  Board       Players    TurnFlow   WinRules   Starter    GameLog    GuiBridge    │
│  (gmMap)     (gmActor)  (gmFlow)   (gmRules)  (gmAlea)   (gmLog)    (gmDispatch) │
│                                                                     + CmdServer  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## File Structure

```text
GAME/Tic-Tac-Toe/
├── PLAN.md                          ← questo file (single source of truth)
├── info/                            ← spec di dominio, regole GRS, doc
│   ├── TicTacToe_Implementation.md
│   ├── tictactoe-tris.example.grs
│   ├── tictactoe-tris.example.yaml
│   └── tictactoe-tris.rules.graph.md
├── CoreEngine/                      ← processo C++17
│   ├── CMakeLists.txt               ← build target tris_engine, linka gmXxx
│   ├── main.cpp                     ← entry point: crea TrisEngine, loop
│   ├── engine/                      ← facade/mediator + tipi comuni
│   │   ├── TrisTypes.hpp            ← Mark, Cell, Phase, CommandIds, EventIds
│   │   ├── TrisEngine.hpp/.cpp      ← coordina i sottosistemi
│   ├── board/                       ← uso di gmMap (griglia 3×3)
│   │   └── Board.hpp/.cpp
│   ├── players/                     ← uso di gmActor (X / O + status)
│   │   └── Players.hpp/.cpp
│   ├── flow/                        ← uso di gmFlow (fasi turno)
│   │   └── TurnFlow.hpp/.cpp
│   ├── rules/                       ← uso di gmRules (win/draw)
│   │   └── WinRules.hpp/.cpp
│   ├── alea/                        ← uso di gmAlea (dado starter 1d2)
│   │   └── Starter.hpp/.cpp
│   ├── logging/                     ← uso di gmLog (log mosse/esito)
│   │   └── GameLog.hpp/.cpp
│   └── bridge/                      ← uso di gmDispatch (eventi + comandi)
│   │   ├── GuiBridge.hpp/.cpp       ← invio eventi (IpSocketChannel :9100)
│       └── CmdServer.hpp/.cpp       ← ricezione comandi (TCP server :9001)
└── GUI/                             ← processo Python / PySide6
    ├── main.py                      ← entry point QApplication
    ├── app/
    │   ├── tris_window.py           ← QMainWindow con layout da figura
    │   └── tris_bridge.py           ← wrapper su engine_bridge (riuso)
    └── widgets/
        ├── board_widget.py          ← griglia 3×3 di celle cliccabili
        ├── turn_state_widget.py     ← stato giocatore di turno
        ├── log_widget.py            ← log partita
        └── error_bar_widget.py      ← messaggi di errore
```

---

## Development Phases

### Phase 1 — Interfaces & Stubs ✅

- [x] Scaffold cartelle `CoreEngine/` e `GUI/` con struttura per-libreria.
- [x] `engine/TrisTypes.hpp`: enum `Mark`, `Phase`, costanti command/event id.
- [x] `bridge/GuiBridge`: invio eventi JSON via `IpSocketChannel` (:9100).
- [x] `bridge/CmdServer`: server TCP (:9001) con frame `uint32+UTF-8`, callback.
- [x] `engine/TrisEngine`: facade con membri stub dei sottosistemi.
- [x] Wrapper lib stub: `Board`, `Players`, `TurnFlow`, `WinRules`, `Starter`,
      `GameLog` (firme pubbliche, corpi minimi).
- [x] `CoreEngine/CMakeLists.txt`: target `tris_engine` che linka le gmXxx.
- [x] `main.cpp`: istanzia engine, avvia CmdServer, attende.
- [x] GUI shell: `TrisWindow` con i pannelli del layout (figura), bridge wired.
- [x] **Smoke test:** CoreEngine compila e linka; GUI si avvia (offscreen);
      loop E2E Engine↔GUI verificato (new_game → snapshot → mosse → win).

**Notes:**
- Pattern scelto: **Facade + Mediator** con composizione dentro `TrisEngine`,
  istanziato in `main.cpp` (no singleton).
- Porte: eventi Engine→GUI su **9100** (GUI è server), comandi GUI→Engine su
  **9001** (Engine è server). La porta eventi è stata spostata da 9000 a 9100
  perché 9000 risultava occupata da software proxy/agent locale su Windows
  (vedi Decisione 6).
- `CmdServer` rispecchia il wire-format di `IpSocketChannel.cpp`
  (4 byte big-endian + payload), per riuso del protocollo.
- Target CMake delle lib disponibili: `gmDispatch`, `gmActor`, `gmFlow`,
  `gmRules`, `gmLog`, `gmAlea`, `gmSave`. `gmMap` è header-only template
  (dipende da `gmSave`). `gmActor` richiede `gmSave` + `gmFlow`.
- Build: `CoreEngine/CMakeLists.txt` è aggiunto dal root via
  `add_subdirectory(GAME/Tic-Tac-Toe/CoreEngine)`; il target `tris_engine`
  linka `gmDispatch`/`gmLog`/`gmAlea` (+ `ws2_32` su Windows) e include la
  root per gli header `gmXxx`/`gmSave`.

### Phase 2 — Domain logic (board, players, flow, rules) ✅

- [x] `Board` su `gmMap`: 9 location, metadata `mark` per cella, adiacenze ortogonali.
- [x] `Players` su `gmActor`: `ActorStore` con due eroi X/O, status `ACTIVE_TURN`/`WINNER`/`DRAW`.
- [x] `TurnFlow` su `gmFlow`: `ActorRegistry` + `Turn`/`Round` per fase, fasi `PLAYER1_TURN`/`PLAYER2_TURN`/`GAME_OVER`.
- [x] `WinRules` su `gmRules`: 8 linee come `ALL_OF` di `LOCATION_HAS_TAG` valutate da `gmRulesEngine` su `TrisRuleContext`, + draw.
- [x] `Starter` su `gmAlea`: 1d2 per scelta primo giocatore (invariato dalla Phase 1).
- [x] `TrisEngine.handle_command`: `gmTris.move` / `gmTris.new_game` (contratto eventi invariato).
- [x] Emissione eventi: snapshot iniziali + update incrementali (wire-contract identico).
- [x] **Smoke test:** partita completa via comandi simulati → win (`row_1`) e draw verificati E2E.

### Phase 3 — GUI interattiva ⏳

- [ ] `board_widget`: render X/O, click → `gmTris.move`.
- [ ] `turn_state_widget` / `log_widget` / `error_bar_widget` da eventi.
- [ ] Blocco input in `GAME_OVER`, pulsante nuova partita.
- [ ] **Smoke test:** partita giocabile end-to-end Engine↔GUI.

### Phase 4 — Test & robustezza ⏳

- [ ] Unit test C++ regole mossa/win/draw.
- [ ] Test bridge: frame validi e malformati, disconnessione GUI.
- [ ] Test integrazione E2E.
- [ ] **Smoke test:** suite completa verde.

---

## Key Design Decisions

1. **Facade + Mediator** (`TrisEngine`) invece di singleton: ownership chiara,
   testabilità, ogni libreria isolata in una cartella dedicata.
2. **Composizione** dei sottosistemi dentro `TrisEngine`, costruiti in `main.cpp`.
3. **Riuso del protocollo** `engine_bridge` esistente (porte 9000/9001, frame
   `uint32`+JSON) per non duplicare il transport.
4. **GUI game-specific** (layout da figura) anziché i moduli generici `gmXxx`,
   ma riusando l'infrastruttura `engine_bridge` (framing/receiver/sender).
5. **CMake self-contained**: `CoreEngine/CMakeLists.txt` aggiunge come
   subdirectory solo le librerie necessarie, senza modificare il build root.
6. **Porta eventi 9100** (anziché 9000): nell'ambiente di sviluppo la 9000 era
   occupata da software proxy/agent locale (listener `0.0.0.0:9000`), che
   "rubava" le connessioni di loopback all'engine. Spostando il canale eventi su
   9100 il loop Engine→GUI funziona; i comandi restano su 9001.
7. **Wiring librerie Phase 2** (senza toccare il build root): `gmFlow` e
   `gmRules` sono già subdirectory del root e vengono linkati; le sorgenti core
   di `gmActor` (no serializzazione/adapter gmFlow) sono compilate direttamente
   nell'eseguibile; `gmMap` e `gmSave` sono header-only via include path. Flag
   MSVC `/Zc:__cplusplus` necessaria per lo `static_assert` di `gmSave`.
8. **Contratto eventi invariato**: la migrazione ai sottosistemi reali mantiene
   identici `typeId` e payload degli eventi verso la GUI; l'`ActorStore` e il
   `TurnFlow` gmFlow restano fonti di verità interne, rispecchiate (non
   sostituite) dagli eventi già emessi nella Phase 1.
