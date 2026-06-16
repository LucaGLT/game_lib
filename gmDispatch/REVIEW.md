# Code Review — gmDispatch

| Field                | Value                                       |
|----------------------|---------------------------------------------|
| **Reviewed scope**   | `gmDispatch` (entire library, 37 files)     |
| **Date**             | 2026-06-12                                  |
| **Rule-set version** | style-rules.md v1.3                         |
| **Reviewer**         | AI (GitHub Copilot)                         |

---

## Summary

| Category            | Status       | 🔴 | 🟡 | 🔵 | Total |
|---------------------|--------------|----|----|-----|-------|
| CAT-1 Naming        | 🔴 Errors    |  8 |  0 |  0 |     8 |
| CAT-2 Guards        | ✅ Clean     |  0 |  0 |  0 |     0 |
| CAT-3 Formatting    | 🔴 Errors    |  2 |  0 |  0 |     2 |
| CAT-4 Spacing       | ✅ Clean     |  0 |  0 |  0 |     0 |
| CAT-5 Switch/Case   | ✅ Clean     |  0 |  0 |  0 |     0 |
| CAT-6 Signatures    | ✅ Clean     |  0 |  0 |  0 |     0 |
| CAT-7 Includes      | 🔴 Errors    |  1 |  0 |  0 |     1 |
| CAT-8 Preprocessor  | 🔴 Errors    |  1 |  0 |  0 |     1 |
| CAT-9 Documentation | 🔵 Info only |  0 |  0 |  1 |     1 |
| CAT-10 Constraints  | 🔴 Errors    |  1 |  0 |  0 |     1 |
| **TOTALE**          |              | **13** | **0** | **1** | **14** |

---

## Findings

---

### CAT-1 · Naming

---

#### F-01 · NS-1 · 🔴 Error
**File:** tutti i file della libreria (pervasivo — 37 file)
**Location:** ogni `namespace GmDispatch {` / `} // namespace GmDispatch`
**Description:** Il namespace usa `GmDispatch` (G maiuscola) invece del pattern obbligatorio `gm` + PascalCase che produce `gmDispatch`.

```diff
- namespace GmDispatch {
+ namespace gmDispatch {
- } // namespace GmDispatch
+ } // namespace gmDispatch
```

---

#### F-02 · CL-2 · 🔴 Error
**File:** `gmDispatch/Dispatcher.hpp`, `gmDispatch/Dispatcher.cpp`, tutti i consumer
**Location:** `class Dispatcher`
**Description:** La classe principale (façade) deve portare il prefisso `Gm` (regola CL-2). `Dispatcher` deve diventare `GmDispatcher`.

```diff
- class Dispatcher {
+ class GmDispatcher {
```

---

#### F-03 · FN-1 · 🔴 Error
**File:** `channels/EventBusChannel.hpp/.cpp`, `serializers/JsonSerializer.hpp/.cpp`, `routers/PatternRouter.hpp/.cpp`, `channels/IpSocketChannel.hpp/.cpp`, `dispatchers/AsyncDispatcher.hpp/.cpp`
**Location:** metodi pubblici e privati in camelCase
**Description:** Nomi di funzione in camelCase; regola FN-1 impone snake_case.

```diff
// EventBusChannel
- void addHandler(Handler handler);
+ void add_handler(Handler handler);

// JsonSerializer
- static std::string escapeJsonString(const std::string& value);
+ static std::string escape_json_string(const std::string& value);

// PatternRouter (private)
- static bool matchPattern(const std::string& pattern, const std::string& typeId);
+ static bool match_pattern(const std::string& pattern, const std::string& typeId);

// IpSocketChannel (private)
- void closeSocket();
+ void close_socket();

// AsyncDispatcher (private)
- void workerLoop();
+ void worker_loop();
```

---

#### F-04 · FN-2 · 🔴 Error
**File:** `channels/IpSocketChannel.hpp/.cpp`, `routers/PatternRouter.hpp/.cpp`
**Location:** `isConnected()`, `isTargeted()`
**Description:** Boolean query in camelCase. FN-2 impone prefisso `is_` in snake_case.

```diff
- bool isConnected() const;
+ bool is_connected() const;

- static bool isTargeted(const std::shared_ptr<IChannel>& channel, const Envelope& envelope);
+ static bool is_targeted(const std::shared_ptr<IChannel>& channel, const Envelope& envelope);
```

---

#### F-05 · FN-3 · 🔴 Error
**File:** `gmDispatch/DispatcherFactory.hpp/.cpp`
**Location:** tutti i metodi statici factory
**Description:** Metodi factory in camelCase. FN-3 impone prefisso `create_` in snake_case.

```diff
- static Dispatcher createSyncDispatcher(const std::string& name, bool autoTimestamp = true);
+ static GmDispatcher create_sync_dispatcher(const std::string& name, bool auto_timestamp = true);

- static Dispatcher createDebugDispatcher(const std::string& name);
+ static GmDispatcher create_debug_dispatcher(const std::string& name);

- static Dispatcher createAsyncDispatcher(const std::string& name, bool autoTimestamp = true);
+ static GmDispatcher create_async_dispatcher(const std::string& name, bool auto_timestamp = true);

- static Dispatcher createPatternDispatcher(const std::string& name, bool autoTimestamp = true);
+ static GmDispatcher create_pattern_dispatcher(const std::string& name, bool auto_timestamp = true);
```

---

#### F-06 · VAR-1 · 🔴 Error
**File:** `gmDispatch/DispatcherConfig.hpp`
**Location:** `bool autoTimestamp = true;`
**Description:** Il campo pubblico di una struct usa camelCase invece di snake_case (VAR-1).

```diff
- bool autoTimestamp = true;
+ bool auto_timestamp = true;
```

---

#### F-07 · VAR-2 · 🔴 Error
**File:** tutti i file con classi che hanno membri privati (pervasivo)
**Location:** ogni membro privato con suffisso `_`
**Description:** I membri privati usano **suffisso** `_` invece del **prefisso** `_` richiesto da VAR-2.

```diff
// Dispatcher.hpp
- DispatcherConfig             config_;
- std::unique_ptr<IDispatcher> dispatcher_;
+ DispatcherConfig             _config;
+ std::unique_ptr<IDispatcher> _dispatcher;

// SyncDispatcher.hpp
- std::unique_ptr<IRouter>     router_;
- mutable std::recursive_mutex mutex_;
+ std::unique_ptr<IRouter>     _router;
+ mutable std::recursive_mutex _mutex;

// AsyncDispatcher.hpp (tutti i membri)
- queue_, queueMutex_, queueCv_, drainCv_, workerBusy_, stop_, routeMutex_, worker_
+ _queue, _queue_mutex, _queue_cv, _drain_cv, _worker_busy, _stop, _route_mutex, _worker

// EventBusChannel.hpp
- name_, handlers_  →  _name, _handlers

// FileChannel.hpp
- name_, filePath_, file_, serializer_  →  _name, _file_path, _file, _serializer

// StdoutChannel.hpp
- name_, serializer_  →  _name, _serializer

// IpSocketChannel.hpp
- name_, host_, port_, serializer_, socketFd_  →  _name, _host, _port, _serializer, _socket_fd

// SyncRouter.hpp / PatternRouter.hpp
- routes_  →  _routes

// LogDispatchBridge.hpp
- bus_  →  _bus
```

---

#### F-08 · EX-2 · 🔴 Error
**File:** (file mancante)
**Location:** — (classe inesistente)
**Description:** La libreria non definisce una classe base di eccezione. `FileChannel`, `IpSocketChannel` e `IpSocketChannel.cpp` lanciano `std::runtime_error` invece di un'eccezione della libreria. Regola EX-2 impone `EDispatchError`.

```cpp
// Da creare: gmDispatch/GmDispatchError.hpp
namespace gmDispatch {

class EDispatchError : public std::runtime_error
{
public:
    explicit EDispatchError(const std::string& message)
        : std::runtime_error(message)
    {}
};

} // namespace gmDispatch
```

---

### CAT-3 · Formatting

---

#### F-09 · FMT-1 · 🔴 Error
**File:** **tutti i 37 file della libreria** (pervasivo)
**Location:** ogni riga indentata
**Description:** Tutti i file usano **spazi** (4 spazi per livello) per l'indentazione. Regola FMT-1 richiede esclusivamente **caratteri tab reali** (visual width 4). Verificato con analisi byte-a-byte.

```diff
- [4 spazi]void dispatch(const Envelope& envelope) override;
+ [1 tab]void dispatch(const Envelope& envelope) override;
```

---

#### F-10 · FMT-2 · 🔴 Error
**File:** `Dispatcher.cpp`, `AsyncDispatcher.cpp`, `SyncRouter.cpp`, `PatternRouter.cpp`, `FileChannel.cpp`, `StdoutChannel.cpp`, `IpSocketChannel.cpp`, `JsonSerializer.cpp`
**Location:** blocchi `if`, `while`, `for`, `switch` con `{` in coda alla riga
**Description:** Le parentesi graffe di apertura appaiono sulla stessa riga del costrutto (K&R). Regola FMT-2 impone Allman: `{` sempre su riga propria.

```diff
// Dispatcher.cpp
- if (config_.autoTimestamp && envelope.timestamp == ...) {
+ if (_config.auto_timestamp && envelope.timestamp == ...)
+ {

// AsyncDispatcher.cpp
- while (true) {
+ while (true)
+ {

// SyncRouter.cpp
- if (envelope.typeId != "*") {
+ if (envelope.typeId != "*")
+ {

// FileChannel.cpp
- if (!serializer_) {
+ if (!_serializer)
+ {

// JsonSerializer.cpp
- switch (c) {
+ switch (c)
+ {
- if (c < 0x20) {
+ if (c < 0x20)
+ {
```

---

### CAT-7 · Includes

---

#### F-11 · INC-2 · 🔴 Error
**File:** tutti i file nelle sottocartelle `dispatchers/`, `routers/`, `channels/`, `serializers/`, `bridges/`
**Location:** ogni `#include "../..."` e `#include "../../..."`
**Description:** La regola INC-2 vieta il componente `..` nei percorsi di include relativi. Tutte le sottocartelle usano `../` per referenziare gli header root della libreria.

```diff
// dispatchers/SyncDispatcher.hpp
- #include "../IDispatcher.hpp"
- #include "../IRouter.hpp"
+ #include "IDispatcher.hpp"
+ #include "IRouter.hpp"

// channels/FileChannel.cpp
- #include "../serializers/JsonSerializer.hpp"
+ #include "serializers/JsonSerializer.hpp"

// bridges/LogDispatchBridge.hpp
- #include "../../gmLog/ILogDispatcher.hpp"
+ #include "gmLog/ILogDispatcher.hpp"
```

> Richiede `-I gmDispatch/` e `-I .` nel sistema di build (o configurazione analoga in CMake).

---

### CAT-8 · Preprocessor

---

#### F-12 · PP-1 · 🔴 Error
**File:** `gmDispatch/channels/IpSocketChannel.cpp`
**Location:** blocco `#if defined(_WIN32)`, righe ~24–50
**Description:** Le direttive `#ifndef WIN32_LEAN_AND_MEAN` e i `#define` all'interno del blocco `#if` sono rientrate con 4 spazi invece di essere a colonna 1 (PP-1).

```diff
  #if defined(_WIN32)
-     #ifndef WIN32_LEAN_AND_MEAN
-         #define WIN32_LEAN_AND_MEAN
-     #endif
+#ifndef WIN32_LEAN_AND_MEAN
+#define WIN32_LEAN_AND_MEAN
+#endif
```

---

### CAT-9 · Documentation

---

#### F-13 · DOC-1 · 🔵 Info
**File:** `gmDispatch/channels/IpSocketChannel.hpp`
**Location:** `bool isConnected() const;` (→ `is_connected()` dopo F-04)
**Description:** Il metodo pubblico non-void `is_connected()` ha solo `///< brief` ma manca del tag `@return` obbligatorio per tutti i metodi pubblici.

```diff
- /// @brief Returns @c true if the socket is currently connected.
- bool isConnected() const;
+ /**
+  * @brief Returns @c true if the socket is currently connected.
+  * @return @c true when a TCP connection is established.
+  */
+ bool is_connected() const;
```

---

## Correction Plan

Le correzioni sono ordinate per severità (🔴 prima) e impatto pervasivo (rename globali prima).

### 🔴 Fase 1 — Rename pervasive (impattano tutti i file)

| # | File/Scope | Violazione | Azione |
|---|------------|------------|--------|
| 1 | Tutta la libreria (37 file) | F-01 NS-1 | `namespace GmDispatch` → `gmDispatch` |
| 2 | `Dispatcher.hpp/.cpp` + consumer | F-02 CL-2 | `Dispatcher` → `GmDispatcher` |
| 3 | Tutta la libreria | F-09 FMT-1 | Converti indentazione spazi → tab (batch) |

### 🔴 Fase 2 — Rename locali

| # | File | Violazione | Azione |
|---|------|------------|--------|
| 4 | `DispatcherFactory.hpp/.cpp` | F-05 FN-3 | `createXxx` → `create_xxx` |
| 5 | `DispatcherConfig.hpp` + consumer | F-06 VAR-1 | `autoTimestamp` → `auto_timestamp` |
| 6 | `EventBusChannel.hpp/.cpp` | F-03 FN-1 | `addHandler` → `add_handler` |
| 7 | `JsonSerializer.hpp/.cpp` | F-03 FN-1 | `escapeJsonString` → `escape_json_string` |
| 8 | `PatternRouter.hpp/.cpp` | F-03/F-04 FN-1/2 | `matchPattern`→`match_pattern`, `isTargeted`→`is_targeted` |
| 9 | `IpSocketChannel.hpp/.cpp` | F-03/F-04 FN-1/2 | `closeSocket`→`close_socket`, `isConnected`→`is_connected` |
| 10 | `AsyncDispatcher.hpp/.cpp` | F-03 FN-1 | `workerLoop` → `worker_loop` |
| 11 | Tutti i file con classi | F-07 VAR-2 | Suffisso `_` → prefisso `_` su tutti i membri privati |

### 🔴 Fase 3 — Nuova classe eccezione + INC-2 + PP-1 + FMT-2

| # | File | Violazione | Azione |
|---|------|------------|--------|
| 12 | Nuovo `GmDispatchError.hpp` | F-08 EX-2 | Creare `EDispatchError`; aggiornare FileChannel, IpSocketChannel |
| 13 | Tutte le sottocartelle | F-11 INC-2 | Rimuovere `..` dai percorsi include |
| 14 | `IpSocketChannel.cpp` | F-12 PP-1 | Portare `#define` a colonna 0 nel blocco platform |
| 15 | `.cpp` con if/while/switch K&R | F-10 FMT-2 | Portare `{` su riga propria (Allman) |

### 🔵 Fase 4 — Info

| # | File | Violazione | Azione |
|---|------|------------|--------|
| 16 | `IpSocketChannel.hpp` | F-13 DOC-1 | Aggiungere `@return` a `is_connected()` |

---

*Fine della review — nessuna modifica ai sorgenti applicata in questo documento.*
