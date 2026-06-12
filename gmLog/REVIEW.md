# Code Review — gmLog

| Field              | Value                                       |
|--------------------|---------------------------------------------|
| **Reviewed scope** | `gmLog` (entire library)                    |
| **Date**           | 2026-06-12                                  |
| **Rule-set version** | style-rules.md v1.3                       |
| **Reviewer**       | AI (GitHub Copilot)                         |

---

## Summary

| Category            | Status       | 🔴 | 🟡 | 🔵 | Total |
|---------------------|--------------|----|----|-----|-------|
| CAT-1 Naming        | 🔴 Errors    |  8 |  0 |  0 |     8 |
| CAT-2 Guards        | ✅ Clean     |  0 |  0 |  0 |     0 |
| CAT-3 Formatting    | 🔴 Errors    |  2 |  0 |  1 |     3 |
| CAT-4 Spacing       | 🔵 Info only |  0 |  0 |  1 |     1 |
| CAT-5 Switch/Case   | ✅ Clean     |  0 |  0 |  0 |     0 |
| CAT-6 Signatures    | 🟡 Warnings  |  0 |  1 |  0 |     1 |
| CAT-7 Includes      | 🔴 Errors    |  3 |  0 |  0 |     3 |
| CAT-8 Preprocessor  | 🟡 Warnings  |  0 |  2 |  0 |     2 |
| CAT-9 Documentation | 🔵 Info only |  0 |  0 |  1 |     1 |
| CAT-10 Constraints  | 🔴 Errors    |  1 |  0 |  0 |     1 |
| **TOTALE**          |              | **14** | **3** | **3** | **20** |

---

## Findings

---

### CAT-1 · Naming

---

#### F-01 · NS-1 · 🔴 Error
**File:** tutti i file della libreria (pervasivo)
**Location:** ogni `namespace GmLog {` / `} // namespace GmLog`
**Description:** Il namespace usa `GmLog` (G maiuscola) invece del pattern obbligatorio `gm` + PascalCase che produce `gmLog`.

```diff
- namespace GmLog {
+ namespace gmLog {

- } // namespace GmLog
+ } // namespace gmLog
```

---

#### F-02 · CL-2 · 🔴 Error
**File:** `gmLog/Logger.hpp`, `gmLog/Logger.cpp`
**Location:** `class Logger`, tutte le occorrenze di `Logger`
**Description:** La classe principale (façade) della libreria deve portare il prefisso `Gm` (regola CL-2). `Logger` deve diventare `GmLogger`.

```diff
- class Logger {
+ class GmLogger {
```

> **Nota:** questa rename impatta anche `LoggerFactory` (tipo restituito e parametri), tutti i `.cpp` e i file di test.

---

#### F-03 · FN-1 · 🔴 Error
**File:** `gmLog/LogLevel.hpp`, `gmLog/LogLevel.cpp`
**Location:** `levelToString()`, `levelFromString()`
**Description:** Nomi di funzione in camelCase; la regola FN-1 richiede snake_case.

```diff
- const char* levelToString(LogLevel level);
+ const char* level_to_string(LogLevel level);

- bool levelFromString(const std::string& str, LogLevel& out);
+ bool level_from_string(const std::string& str, LogLevel& out);
```

---

#### F-04 · FN-1 / FN-2 · 🔴 Error
**File:** `gmLog/Logger.hpp`, `gmLog/Logger.cpp`
**Location:** `minLevel()`, `setLevel()`, `isEnabled()`
**Description:** Nomi di metodo in camelCase (FN-1). `isEnabled` viola anche FN-2 (boolean query deve usare prefisso `is_` in snake_case, non camelCase).

```diff
- LogLevel minLevel() const;
+ LogLevel min_level() const;

- void setLevel(LogLevel level);
+ void set_level(LogLevel level);

- bool isEnabled(LogLevel level) const;
+ bool is_enabled(LogLevel level) const;
```

---

#### F-05 · FN-1 / FN-3 · 🔴 Error
**File:** `gmLog/LoggerFactory.hpp`, `gmLog/LoggerFactory.cpp`
**Location:** `createStdoutLogger()`, `createFileLogger()`
**Description:** Nomi di factory in camelCase (FN-1). Le factory devono usare prefisso `create_` in snake_case (FN-3).

```diff
- static Logger createStdoutLogger(...);
+ static GmLogger create_stdout_logger(...);

- static Logger createFileLogger(...);
+ static GmLogger create_file_logger(...);
```

---

#### F-06 · FN-1 · 🔴 Error
**File:** `gmLog/formatters/JsonFormatter.hpp`, `gmLog/formatters/JsonFormatter.cpp`
**Location:** `escapeJsonString()`, `formatTimestamp()`
**Description:** Nomi in camelCase invece di snake_case (FN-1).

```diff
- static std::string escapeJsonString(const std::string& value);
+ static std::string escape_json_string(const std::string& value);

- static std::string formatTimestamp(const std::chrono::system_clock::time_point& tp);
+ static std::string format_timestamp(const std::chrono::system_clock::time_point& tp);
```

---

#### F-07 · VAR-2 · 🔴 Error
**File:** `gmLog/Logger.hpp`, `gmLog/dispatchers/SyncDispatcher.hpp`, `gmLog/sinks/FileSink.hpp`
**Location:** tutti i membri privati
**Description:** I membri privati usano il **suffisso** underscore (`config_`, `dispatcher_`, `mutex_`, `sink_`, `formatter_`, `filePath_`, `file_`) invece del **prefisso** underscore richiesto da VAR-2.

```diff
- LoggerConfig                    config_;
- std::unique_ptr<ILogDispatcher> dispatcher_;
+ LoggerConfig                    _config;
+ std::unique_ptr<ILogDispatcher> _dispatcher;

// SyncDispatcher.hpp
- std::mutex                    mutex_;
- std::unique_ptr<ILogSink>      sink_;
- std::unique_ptr<ILogFormatter> formatter_;
+ std::mutex                    _mutex;
+ std::unique_ptr<ILogSink>      _sink;
+ std::unique_ptr<ILogFormatter> _formatter;

// FileSink.hpp
- std::string   filePath_;
- std::ofstream file_;
+ std::string   _file_path;
+ std::ofstream _file;
```

---

#### F-08 · CONST-2 · 🔴 Error
**File:** `gmLog/LogLevel.hpp`, tutti i file che usano `LogLevel::`
**Location:** `enum class LogLevel : int { Debug, Info, Warning, Error, Critical, Off }`
**Description:** Gli enumeratori di `enum class` usano PascalCase invece di SCREAMING_SNAKE_CASE (regola CONST-2).

```diff
  enum class LogLevel : int {
-     Debug    = 0,
-     Info     = 1,
-     Warning  = 2,
-     Error    = 3,
-     Critical = 4,
-     Off      = 5
+     DEBUG    = 0,
+     INFO     = 1,
+     WARNING  = 2,
+     ERROR    = 3,
+     CRITICAL = 4,
+     OFF      = 5
  };
```

> **Nota:** questa rename è pervasiva — impatta ogni `LogLevel::Debug` / `LogLevel::Info` ecc. in tutta la libreria e nei file di test.

---

#### F-09 · EX-2 · 🔴 Error
**File:** `gmLog/` (file mancante)
**Location:** — (classe inesistente)
**Description:** La libreria non definisce una classe base di eccezione. La regola EX-2 impone che ogni libreria abbia una propria gerarchia di eccezioni con base `E<Topic>Error`. `FileSink` lancia `std::runtime_error` invece di un'eccezione della libreria.

```cpp
// Da aggiungere in un nuovo header gmLog/GmLogError.hpp:
namespace gmLog {

/**
 * @brief Base exception class for all gmLog errors.
 */
class ELogError : public std::runtime_error {
public:
    explicit ELogError(const std::string& message)
        : std::runtime_error(message)
    {}
};

} // namespace gmLog
```

```diff
// sinks/FileSink.cpp
- throw std::runtime_error("FileSink: cannot open log file: " + filePath);
+ throw ELogError("FileSink: cannot open log file: " + filePath);
```

---

### CAT-3 · Formatting

---

#### F-10 · FMT-1 · 🔴 Error
**File:** **tutta la libreria** (pervasivo — tutti i `.cpp` e `.hpp`)
**Location:** ogni linea indentata
**Description:** Tutti i file usano **spazi** (4 spazi per livello) per l'indentazione. La regola FMT-1 richiede esclusivamente **caratteri tab reali** (visual width 4). Verificato con analisi byte-a-byte: nessun file contiene il carattere `0x09`.

```diff
- [4 spazi]void flush() override;
+ [1 tab]void flush() override;
```

> Correzione meccanica applicabile con un'espansione inversa (`unexpand --first-only -t 4` su Linux, oppure la funzione "Convert Indentation to Tabs" dell'editor).

---

#### F-11 · FMT-2 · 🔴 Error
**File:** `gmLog/LogLevel.cpp`, `gmLog/Logger.cpp`, `gmLog/sinks/FileSink.cpp`, `gmLog/formatters/JsonFormatter.cpp`
**Location:** vari blocchi `if`, `switch`, `for`
**Description:** Le parentesi graffe di apertura appaiono in coda alla stessa riga dell'istruzione di controllo (stile K&R). La regola FMT-2 impone lo stile Allman: la `{` deve essere sempre su una riga propria.

```diff
// LogLevel.cpp — switch
- switch (level) {
-     case LogLevel::Debug: return "DEBUG";
+ switch (level)
+ {
+     case LogLevel::DEBUG: return "DEBUG";

// LogLevel.cpp — if con corpo inline
- if (lower == "debug")    { out = LogLevel::Debug;    return true; }
+ if (lower == "debug")
+ {
+     out = LogLevel::DEBUG;
+     return true;
+ }

// Logger.cpp
- if (config_.enableSourceLocation) {
+ if (_config.enableSourceLocation)
+ {

// FileSink.cpp
- if (!file_.is_open()) {
+ if (!_file.is_open())
+ {

// JsonFormatter.cpp
- if (record.file && record.line > 0) {
+ if (record.file && record.line > 0)
+ {
```

---

#### F-12 · FMT-2 · 🔵 Info
**File:** `gmLog/Logger.cpp`
**Location:** `Logger::~Logger()`, riga `if (dispatcher_)`
**Description:** Singola istruzione senza `{}` in un distruttore. Formalmente FMT-5 ammette i casi "truly trivial", ma un guard su un `unique_ptr` con side effect è borderline. Preferibile aggiungere le graffe.

```diff
- if (dispatcher_)
-     dispatcher_->flush();
+ if (_dispatcher)
+ {
+     _dispatcher->flush();
+ }
```

---

### CAT-4 · Spacing

---

#### F-13 · SPC-1 · 🔵 Info
**File:** `gmLog/Logger.cpp`
**Location:** funzione `log()`, blocco di assegnazione del record, righe 49–52
**Description:** Gli operatori `=` sono preceduti da multipli spazi di allineamento. La regola SPC-1 prescrive esattamente uno spazio attorno agli operatori binari. L'allineamento colonnare è solo una convenzione visiva, non prevista dalle regole.

```diff
-     record.level      = level;
-     record.loggerName = config_.name;
-     record.message    = message;
-     record.timestamp  = std::chrono::system_clock::now();
+     record.level = level;
+     record.logger_name = _config.name;
+     record.message = message;
+     record.timestamp = std::chrono::system_clock::now();
```

---

### CAT-6 · Signatures

---

#### F-14 · SIG-3 · 🟡 Warning
**File:** `gmLog/Logger.cpp`, `gmLog/LoggerFactory.cpp`, `gmLog/dispatchers/SyncDispatcher.cpp`
**Location:** definizioni multi-linea di `Logger::log()`, `createStdoutLogger()`, `createFileLogger()`, `SyncDispatcher::SyncDispatcher()`
**Description:** Nelle firme multi-linea che superano 100 colonne, la `)` chiudente deve trovarsi su una riga propria (SIG-3) e la `{` di apertura del corpo sulla riga successiva.

```diff
// Logger.cpp
  void Logger::log(LogLevel           level,
                   const std::string& message,
                   const char*        file,
                   int                line,
-                  const char*        function)
- {
+                  const char*        function
+ )
+ {

// LoggerFactory.cpp
  Logger LoggerFactory::createStdoutLogger(const std::string& name,
                                           LogLevel           level,
-                                          bool               enableSourceLocation)
- {
+                                          bool               enableSourceLocation
+ )
+ {
```

---

### CAT-7 · Includes

---

#### F-15 · INC-1 · 🔴 Error
**File:** `gmLog/LoggerFactory.cpp`
**Location:** righe include, `#include <memory>`
**Description:** L'header `<memory>` della stdlib appare **dopo** gli header di progetto. L'ordine corretto è: header proprio → stdlib → terze parti → progetto.

```diff
  #include "LoggerFactory.hpp"

+ #include <memory>
+
  #include "dispatchers/SyncDispatcher.hpp"
  #include "formatters/JsonFormatter.hpp"
  #include "sinks/FileSink.hpp"
  #include "sinks/StdoutSink.hpp"
-
- #include <memory>
```

---

#### F-16 · INC-1 · 🔴 Error
**File:** `gmLog/formatters/JsonFormatter.cpp`
**Location:** blocco include
**Description:** `#include "../LogLevel.hpp"` (header di progetto) appare prima degli header stdlib (`<chrono>`, `<cstdio>`, ecc.), invertendo l'ordine corretto.

```diff
  #include "JsonFormatter.hpp"

+ #include <chrono>
+ #include <cstdio>
+ #include <ctime>
+ #include <sstream>
+
  #include "../LogLevel.hpp"
-
- #include <chrono>
- #include <cstdio>
- #include <ctime>
- #include <sstream>
```

---

#### F-17 · INC-2 · 🔴 Error
**File:** `gmLog/dispatchers/SyncDispatcher.hpp`, `gmLog/formatters/JsonFormatter.hpp`, `gmLog/formatters/JsonFormatter.cpp`, `gmLog/sinks/FileSink.hpp`, `gmLog/sinks/StdoutSink.hpp`
**Location:** direttive `#include "../..."`
**Description:** La regola INC-2 vieta il componente `..` nei percorsi di include relativi. Le sottocartelle (`dispatchers/`, `formatters/`, `sinks/`) devono usare percorsi assoluti rispetto alla radice della libreria oppure configurare correttamente il path del compilatore.

```diff
// SyncDispatcher.hpp
- #include "../ILogDispatcher.hpp"
- #include "../ILogFormatter.hpp"
- #include "../ILogSink.hpp"
+ #include "gmLog/ILogDispatcher.hpp"
+ #include "gmLog/ILogFormatter.hpp"
+ #include "gmLog/ILogSink.hpp"

// oppure, con -I flag che punta alla root di gmLog:
+ #include "ILogDispatcher.hpp"
+ #include "ILogFormatter.hpp"
+ #include "ILogSink.hpp"
```

> La stessa correzione si applica a `JsonFormatter.hpp` (`"../ILogFormatter.hpp"`), `JsonFormatter.cpp` (`"../LogLevel.hpp"`), `FileSink.hpp` e `StdoutSink.hpp` (`"../ILogSink.hpp"`).

---

### CAT-8 · Preprocessor

---

#### F-18 · PP-1 · 🟡 Warning
**File:** `gmLog/macros/LogMacros.hpp`
**Location:** `#define LOG_COMPILED_LEVEL LOG_LEVEL_DEBUG` (riga ~67) e tutti i `#define logXxx` all'interno dei blocchi `#if`
**Description:** Le direttive `#define` annidate nei blocchi `#if`/`#else` sono rientrate di 4 spazi. La regola PP-1 richiede che tutte le direttive preprocessore inizino alla colonna 1.

```diff
  #ifndef LOG_COMPILED_LEVEL
-     #define LOG_COMPILED_LEVEL LOG_LEVEL_DEBUG
+ #define LOG_COMPILED_LEVEL LOG_LEVEL_DEBUG
  #endif

  #if LOG_COMPILED_LEVEL <= LOG_LEVEL_DEBUG
-     #define logDebug(logger, expr) \
+#define logDebug(logger, expr) \
          _GMLOG_DO_LOG(...)
  #else
-     #define logDebug(logger, expr) do {} while (0)
+ #define logDebug(logger, expr) do {} while (0)
  #endif
```

---

#### F-19 · PP-2 · 🟡 Warning
**File:** `gmLog/macros/LogMacros.hpp`
**Location:** ogni `#endif` intermedio che chiude un `#if LOG_COMPILED_LEVEL <= ...`
**Description:** I blocchi `#endif` intermedi non hanno un commento identificativo. La regola PP-2 richiede commenti sui `#endif` complessi.

```diff
- #endif
+ #endif // LOG_COMPILED_LEVEL <= LOG_LEVEL_DEBUG

- #endif
+ #endif // LOG_COMPILED_LEVEL <= LOG_LEVEL_INFO

  // ecc. per WARNING, ERROR, CRITICAL
```

---

### CAT-9 · Documentation

---

#### F-20 · DOC-1 · 🔵 Info
**File:** `gmLog/Logger.hpp`
**Location:** `Logger::minLevel()`, riga ~100
**Description:** Il metodo pubblico `minLevel()` ha solo `@brief` ma manca del tag `@return` obbligatorio per tutti i metodi pubblici non-void.

```diff
  /**
   * @brief Returns the current minimum log level.
+  * @return The minimum @ref LogLevel accepted; events below it are dropped.
   */
  LogLevel minLevel() const;
```

---

## Correction Plan

Le correzioni sono ordinate per severità (🔴 prima) e impatto pervasivo (rename globali prima).

### 🔴 Fase 1 — Rename pervasive (impattano tutti i file)

| # | File/Scope | Violazione | Azione |
|---|------------|------------|--------|
| 1 | Tutta la libreria | F-01 NS-1 | Rinominare `namespace GmLog` → `gmLog` in tutti i file |
| 2 | `Logger.hpp/.cpp` + dipendenti | F-02 CL-2 | Rinominare `Logger` → `GmLogger` in tutti i file |
| 3 | `LogLevel.hpp` + tutti i file | F-08 CONST-2 | Rinominare enumeratori `Debug`→`DEBUG`, `Info`→`INFO`, ecc. |
| 4 | Tutta la libreria | F-10 FMT-1 | Convertire indentazione da spazi a tab (operazione batch) |

### 🔴 Fase 2 — Rename locali + nuova classe eccezione

| # | File | Violazione | Azione |
|---|------|------------|--------|
| 5 | `LogLevel.hpp/.cpp` | F-03 FN-1 | `levelToString` → `level_to_string`, `levelFromString` → `level_from_string` |
| 6 | `Logger.hpp/.cpp` | F-04 FN-1/FN-2 | `minLevel` → `min_level`, `setLevel` → `set_level`, `isEnabled` → `is_enabled` |
| 7 | `LoggerFactory.hpp/.cpp` | F-05 FN-1/FN-3 | `createStdoutLogger` → `create_stdout_logger`, `createFileLogger` → `create_file_logger` |
| 8 | `JsonFormatter.hpp/.cpp` | F-06 FN-1 | `escapeJsonString` → `escape_json_string`, `formatTimestamp` → `format_timestamp` |
| 9 | `Logger.hpp/.cpp` + `SyncDispatcher.hpp` + `FileSink.hpp` | F-07 VAR-2 | Tutti i membri privati: suffisso `_` → prefisso `_` |
| 10 | Nuovo file `GmLogError.hpp` | F-09 EX-2 | Creare classe base `ELogError`; aggiornare `FileSink.cpp` |

### 🔴 Fase 3 — Formatting e includes

| # | File | Violazione | Azione |
|---|------|------------|--------|
| 11 | `LogLevel.cpp`, `Logger.cpp`, `FileSink.cpp`, `JsonFormatter.cpp` | F-11 FMT-2 | Portare tutti i `{` su riga propria (Allman) |
| 12 | `LoggerFactory.cpp` | F-15 INC-1 | Spostare `#include <memory>` prima degli header di progetto |
| 13 | `JsonFormatter.cpp` | F-16 INC-1 | Spostare stdlib includes prima di `"../LogLevel.hpp"` |
| 14 | Sottocartelle `dispatchers/`, `formatters/`, `sinks/` | F-17 INC-2 | Eliminare `..` dai percorsi include (configurare `-I` flag o usare percorsi assoluti) |

### 🟡 Fase 4 — Warning

| # | File | Violazione | Azione |
|---|------|------------|--------|
| 15 | `Logger.cpp`, `LoggerFactory.cpp`, `SyncDispatcher.cpp` | F-14 SIG-3 | Portare `)` chiudente su riga propria nelle definizioni multi-linea |
| 16 | `macros/LogMacros.hpp` | F-18 PP-1 | Portare tutti i `#define` annidati a colonna 1 |
| 17 | `macros/LogMacros.hpp` | F-19 PP-2 | Aggiungere commento identificativo ai `#endif` intermedi |

### 🔵 Fase 5 — Info

| # | File | Violazione | Azione |
|---|------|------------|--------|
| 18 | `Logger.cpp` | F-12 FMT-2 | Aggiungere `{}` al guard del distruttore |
| 19 | `Logger.cpp` | F-13 SPC-1 | Rimuovere allineamento colonnare nelle assegnazioni |
| 20 | `Logger.hpp` | F-20 DOC-1 | Aggiungere `@return` a `min_level()` |

---

*Fine della review — nessuna modifica ai sorgenti applicata in questo documento.*
