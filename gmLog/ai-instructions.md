=========================
# Libreria : gmLog
=========================

# 1. Mappa delle esigenze

## Obiettivo della libreria

Creare una libreria C++17 per logging applicativo, pensata principalmente per applicazioni desktop cross-OS, con architettura estendibile verso nuovi canali di output, configurazioni più evolute e logging asincrono.

La libreria deve partire semplice, ma non deve essere progettata “chiusa”.

---

# 2. Requisiti funzionali

## 2.1 Livelli di log

Livelli richiesti:

```cpp
DEBUG
INFO
WARNING
ERROR
CRITICAL
```

Requisito:

```cpp
logger.setLevel(LogLevel::Warning);
```

Significa che vengono scritti solo:

```text
WARNING
ERROR
CRITICAL
```

mentre vengono ignorati:

```text
DEBUG
INFO
```

---

## 2.2 Logger multipli

Devi poter creare più logger indipendenti.

Esempio concettuale:

```cpp
auto dbLogger = Logger::create("Database");
auto netLogger = Logger::create("Network");
auto uiLogger = Logger::create("UI");
```

Ogni logger deve poter avere:

| Aspetto                        | Esempio                            |
| ------------------------------ | ---------------------------------- |
| Nome diverso                   | `Database`, `Network`, `UI`        |
| Livello diverso                | `Database = DEBUG`, `UI = WARNING` |
| Canale diverso                 | `stdout`, `file`, futuro seriale   |
| File diverso                   | `db.log`, `network.log`, `ui.log`  |
| Formattazione comune o diversa | JSON Lines                         |
| Configurazione indipendente    | sì                                 |

Quindi eviterei un unico logger globale rigido.

Puoi eventualmente avere un `LoggerRegistry`, ma non come requisito della V1.

---

## 2.3 Canale di output singolo per logger

Ogni singolo logger scrive verso **un solo canale**.

Esempio:

```text
Database logger → db.log
UI logger       → stdout
Serial logger   → futuro canale seriale
```

Non serve, per ora:

```text
stesso logger → stdout (e anche) file (e anche) seriale
```

Quindi il `MultiSink` non è necessario nella V1.

---

## 2.4 Canali richiesti nella V1

Da implementare subito:

```text
StdoutSink
FileSink
```

Da non implementare ora, ma da prevedere architetturalmente:

```text
SerialSink / ComSink / CustomSink
```

Quindi serve un’interfaccia astratta:

```cpp
class ILogSink
{
public:
    virtual ~ILogSink() = default;
    virtual void write(const std::string& message) = 0;
};
```

Poi:

```cpp
class StdoutSink : public ILogSink
{
public:
    void write(const std::string& message) override;
};
```

```cpp
class FileSink : public ILogSink
{
public:
    explicit FileSink(const std::string& path);
    void write(const std::string& message) override;

private:
    std::ofstream file_;
};
```

In futuro:

```cpp
class SerialSink : public ILogSink
{
public:
    void write(const std::string& message) override;
};
```

Il `Logger` non deve sapere se sta scrivendo su stdout, file o seriale.

---

# 3. Requisiti non funzionali

## 3.1 Cross-OS

Target principale:

```text
Windows
Linux
macOS
```

Per questo motivo la V1 dovrebbe usare solo C++17 standard:

```cpp
<iostream>
<fstream>
<string>
<memory>
<mutex>
<chrono>
<sstream>
<iomanip>
```

Da evitare nella parte core:

```text
API Win32
POSIX
librerie seriali OS-specifiche
dipendenze non standard
```

La futura seriale andrà tenuta fuori dal core, come plugin/sink separato.

---

## 3.2 Thread safety

La libreria sarà usata da più thread contemporaneamente.

Quindi serve protezione.

Nella V1, il logger sincrono può usare:

```cpp
std::mutex
```

Nel metodo `log()`:

```cpp
std::lock_guard<std::mutex> lock(mutex_);
sink_->write(formattedMessage);
```

Questo garantisce che due thread non scrivano contemporaneamente sullo stesso sink.

---

## 3.3 Logging sincrono nella V1, asincrono in futuro

Per ora:

```text
log() → formatta → scrive subito
```

Futuro:

```text
log() → inserisce in coda → thread separato scrive
```

Per non stravolgere il design, conviene introdurre già un livello di astrazione interno.

Invece di far fare tutto direttamente al `Logger`, puoi pensare a questo modello:

```text
Logger
  ↓
ILogDispatcher
  ↓
ILogSink
```

Nella V1:

```cpp
SyncDispatcher
```

In futuro:

```cpp
AsyncDispatcher
```

Schema:

```text
Logger
 ├── filtra livello
 ├── crea LogRecord
 └── passa il record al Dispatcher

SyncDispatcher
 ├── formatta
 └── scrive sul Sink

AsyncDispatcher futuro
 ├── mette in coda
 ├── thread worker
 ├── formatta
 └── scrive sul Sink
```

Così in futuro non cambi l’API pubblica del logger.

---

# 4. Formato dei log

Hai scelto JSON Lines.

Esempio:

```json
{"time":"2026-06-10T17:42:11.235","level":"INFO","logger":"Database","message":"Sistema inizializzato"}
```

Io aggiungerei già il campo `logger`, perché hai esplicitamente richiesto più logger/moduli.

Formato minimo consigliato:

```json
{"time":"2026-06-10T17:42:11.235","logger":"Database","level":"INFO","message":"Sistema inizializzato"}
```

Formato esteso opzionale:

```json
{"time":"2026-06-10T17:42:11.235","logger":"Database","level":"ERROR","file":"Database.cpp","line":87,"function":"connect","message":"Connection failed"}
```

Poiché poi sarà una GUI/UI a leggere il file, JSON Lines è una scelta corretta: una riga = un evento log.

---

# 5. Struttura concettuale consigliata

## Componenti principali

```text
LogLevel
LogRecord
LoggerConfig
Logger
ILogSink
StdoutSink
FileSink
ILogFormatter
JsonFormatter
ILogDispatcher
SyncDispatcher
```

---

## 5.1 LogLevel

```cpp
enum class LogLevel
{
    Debug = 0,
    Info,
    Warning,
    Error,
    Critical
};
```

L’ordine numerico serve per il filtro:

```cpp
if (level < minLevel)
{
    return;
}
```

---

## 5.2 LogRecord

Il record rappresenta l’evento grezzo.

```cpp
struct LogRecord
{
    LogLevel level;
    std::string loggerName;
    std::string message;

    std::chrono::system_clock::time_point timestamp;

    const char* file = nullptr;
    int line = 0;
    const char* function = nullptr;
};
```

Il `Logger` crea un `LogRecord`.

Il formatter lo trasforma in stringa JSON.

Il sink lo scrive.

---

## 5.3 LoggerConfig

Ogni logger dovrebbe avere la propria configurazione.

```cpp
struct LoggerConfig
{
    std::string name;
    LogLevel minLevel = LogLevel::Debug;
    bool enableSourceLocation = true;
};
```

In futuro potrai aggiungere:

```cpp
bool asyncEnabled;
size_t queueSize;
bool flushOnError;
bool enableFileRotation;
size_t maxFileSize;
size_t maxBackupFiles;
```

---

## 5.4 ILogSink

```cpp
class ILogSink
{
public:
    virtual ~ILogSink() = default;

    virtual void write(const std::string& message) = 0;
    virtual void flush() = 0;
};
```

Metterei già `flush()`.

Ti servirà per:

```text
file
errori critici
chiusura applicazione
logger asincrono futuro
```

---

## 5.5 ILogFormatter

```cpp
class ILogFormatter
{
public:
    virtual ~ILogFormatter() = default;

    virtual std::string format(const LogRecord& record) = 0;
};
```

V1:

```cpp
JsonFormatter
```

Futuro:

```text
PlainTextFormatter
CompactFormatter
CustomFormatter
```

---

## 5.6 ILogDispatcher

Questo è il punto chiave per non stravolgere tutto quando passerai da sincrono ad asincrono.

```cpp
class ILogDispatcher
{
public:
    virtual ~ILogDispatcher() = default;

    virtual void dispatch(const LogRecord& record) = 0;
    virtual void flush() = 0;
};
```

V1:

```cpp
class SyncDispatcher : public ILogDispatcher
{
public:
    SyncDispatcher(
        std::unique_ptr<ILogSink> sink,
        std::unique_ptr<ILogFormatter> formatter
    );

    void dispatch(const LogRecord& record) override;
    void flush() override;

private:
    std::mutex mutex_;
    std::unique_ptr<ILogSink> sink_;
    std::unique_ptr<ILogFormatter> formatter_;
};
```

Futuro:

```cpp
class AsyncDispatcher : public ILogDispatcher
{
    // queue
    // worker thread
    // condition_variable
    // stop flag
};
```

Così il `Logger` non cambia.

---

# 6. API di logging consigliata

## 6.1 API normale

```cpp
logger.info("Sistema inizializzato");
logger.error("Errore apertura file");
```

Questa è semplice, ma ha un problema: se il messaggio viene costruito prima della chiamata, paghi comunque il costo.

Esempio:

```cpp
logger.debug("Value = " + expensiveToString(value));
```

Anche se `DEBUG` è disabilitato, `expensiveToString(value)` viene eseguito.

---

## 6.2 API lazy

Per evitare costruzione inutile del messaggio:

```cpp
logger.debug([&]() {
    return "Value = " + expensiveToString(value);
});
```

Il logger esegue la lambda solo se il livello è abilitato.

Questa è molto utile.

Esempio:

```cpp
template <typename MessageFactory>
void debug(MessageFactory&& factory)
{
    if (!isEnabled(LogLevel::Debug))
    {
        return;
    }

    log(LogLevel::Debug, factory());
}
```

---

## 6.3 Macro per source location e compile-time filtering

In C++17 non hai `std::source_location`, quindi per file, riga e funzione usi macro.

Esempio:

```cpp
#define logInfo(logger, msg) \
    (logger).log(LogLevel::Info, (msg), __FILE__, __LINE__, __func__)
```

Ma per evitare il costo del messaggio, meglio macro lazy:

```cpp
#define logDebug(logger, expr) \
    do { \
        if ((logger).isEnabled(LogLevel::Debug)) { \
            (logger).log(LogLevel::Debug, (expr), __FILE__, __LINE__, __func__); \
        } \
    } while (0)
```

Uso:

```cpp
logDebug(dbLogger, "Record count: " + std::to_string(count));
```

La macro controlla prima il livello.

---

# 7. Disabilitare DEBUG a compile-time

Vuoi poter escludere completamente i log DEBUG in release.

Soluzione consigliata:

```cpp
#ifndef LOG_COMPILED_LEVEL
#define LOG_COMPILED_LEVEL LOG_LEVEL_DEBUG
#endif
```

Definizione numerica:

```cpp
#define LOG_LEVEL_DEBUG    0
#define LOG_LEVEL_INFO     1
#define LOG_LEVEL_WARNING  2
#define LOG_LEVEL_ERROR    3
#define LOG_LEVEL_CRITICAL 4
#define LOG_LEVEL_OFF      5
```

Macro:

```cpp
#if LOG_COMPILED_LEVEL <= LOG_LEVEL_DEBUG
    #define logDebug(logger, expr) \
        do { \
            if ((logger).isEnabled(LogLevel::Debug)) { \
                (logger).log(LogLevel::Debug, (expr), __FILE__, __LINE__, __func__); \
            } \
        } while (0)
#else
    #define logDebug(logger, expr) do {} while (0)
#endif
```

In release:

```cpp
#define LOG_COMPILED_LEVEL LOG_LEVEL_INFO
```

Risultato:

```cpp
logDebug(logger, expensiveFunction());
```

viene rimosso a compile-time.

---

# 8. File logging V1

Per la V1:

```text
un solo file sempre crescente
```

`FileSink`:

```cpp
class FileSink : public ILogSink
{
public:
    explicit FileSink(const std::string& filePath);

    void write(const std::string& message) override;
    void flush() override;

private:
    std::ofstream file_;
};
```

Apertura consigliata:

```cpp
std::ios::out | std::ios::app
```

Quindi:

```cpp
file_.open(filePath, std::ios::out | std::ios::app);
```

Per futuro file rotation, evita di mettere tutta la logica nel `Logger`.

Meglio prevedere una famiglia di sink:

```text
FileSink
RotatingFileSink futuro
DailyFileSink futuro
```

Il `Logger` continuerà a vedere solo:

```cpp
ILogSink
```

---

# 9. Configurazione

## V1: solo da codice

Esempio:

```cpp
LoggerConfig config;
config.name = "Database";
config.minLevel = LogLevel::Debug;

auto logger = Logger(
    config,
    std::make_unique<SyncDispatcher>(
        std::make_unique<FileSink>("database.log"),
        std::make_unique<JsonFormatter>()
    )
);
```

Oppure con factory più leggibile:

```cpp
auto dbLogger = LoggerFactory::createFileLogger(
    "Database",
    "database.log",
    LogLevel::Debug
);
```

---

## Futuro: configurazione da file

In futuro potresti caricare qualcosa di questo tipo:

```json
{
  "loggers": [
    {
      "name": "Database",
      "level": "DEBUG",
      "sink": {
        "type": "file",
        "path": "database.log"
      }
    },
    {
      "name": "UI",
      "level": "WARNING",
      "sink": {
        "type": "stdout"
      }
    }
  ]
}
```

Per non stravolgere la V1, basta non hardcodare la configurazione nel `Logger`.

Il `Logger` deve ricevere una `LoggerConfig`, non costruirsela internamente.

---

# 10. Mappa dei requisiti → scelte progettuali

| Esigenza                          | Scelta progettuale                                |
| --------------------------------- | ------------------------------------------------- |
| Cross-OS                          | Solo C++17 standard nel core                      |
| Stdout e file nella V1            | `StdoutSink`, `FileSink`                          |
| Futuri canali                     | `ILogSink`                                        |
| Seriale futura                    | `SerialSink` separato dal core                    |
| Logging sincrono ora              | `SyncDispatcher`                                  |
| Logging asincrono futuro          | `ILogDispatcher`, futuro `AsyncDispatcher`        |
| Uso multithread                   | `std::mutex` nel dispatcher/sink                  |
| JSON Lines                        | `JsonFormatter`                                   |
| Più logger indipendenti           | `LoggerConfig` per ogni logger                    |
| Canali diversi per logger         | Ogni `Logger` possiede il proprio dispatcher/sink |
| No multi-output per logger        | Niente `MultiSink` nella V1                       |
| Evitare costo messaggio           | macro + lazy evaluation                           |
| Disabilitare DEBUG in release     | compile-time log level                            |
| File crescente nella V1           | `FileSink` append-only                            |
| Rotazione futura                  | `RotatingFileSink` futuro                         |
| Configurazione da codice nella V1 | factory/helper                                    |
| Configurazione da file futura     | `LoggerConfig` + eventuale parser                 |

---

# 11. Roadmap di sviluppo consigliata

## Step 0 — Definire le interfacce

Prima di implementare la logica, definirei bene gli header principali:

```text
LogLevel.hpp
LogRecord.hpp
LoggerConfig.hpp
ILogSink.hpp
ILogFormatter.hpp
ILogDispatcher.hpp
Logger.hpp
```

Questo è lo scheletro architetturale.

---

## Step 1 — Implementare il core minimo

Componenti:

```text
LogLevel
LogRecord
LoggerConfig
Logger
```

Il logger deve sapere fare solo queste cose:

```text
- avere un nome
- avere un livello minimo
- verificare se un livello è abilitato
- creare un LogRecord
- passare il record al dispatcher
```

Il logger non deve:

```text
- aprire file
- scrivere su std::cout direttamente
- formattare JSON direttamente
- gestire seriali
- conoscere la rotazione file
```

---

## Step 2 — Implementare i Sink V1

Implementare:

```text
StdoutSink
FileSink
```

`StdoutSink`:

```cpp
void StdoutSink::write(const std::string& message)
{
    std::cout << message << std::endl;
}
```

`FileSink`:

```cpp
void FileSink::write(const std::string& message)
{
    file_ << message << '\n';
}
```

---

## Step 3 — Implementare JsonFormatter

Produce una riga JSON per evento.

Campi minimi:

```json
{
  "time": "...",
  "level": "...",
  "logger": "...",
  "message": "..."
}
```

Campi opzionali:

```json
{
  "file": "...",
  "line": 42,
  "function": "..."
}
```

Attenzione: bisogna fare escaping corretto delle stringhe JSON.

Esempio: se il messaggio contiene virgolette:

```text
Errore su campo "name"
```

deve diventare:

```json
"Errore su campo \"name\""
```

Quindi serve una funzione:

```cpp
std::string escapeJsonString(const std::string& value);
```

---

## Step 4 — Implementare SyncDispatcher

Il dispatcher sincrono tiene insieme:

```text
Sink
Formatter
Mutex
```

Responsabilità:

```text
- riceve LogRecord
- formatta
- acquisisce mutex
- scrive sul sink
```

Questo è il punto dove mettere la thread safety.

---

## Step 5 — Implementare macro e compile-time filtering

Macro minime:

```cpp
logDebug(logger, expr)
logInfo(logger, expr)
logWarn(logger, expr)
logErr(logger, expr)
logCritic(logger, expr)
```

Con:

```text
__FILE__
__LINE__
__func__
```

E con esclusione compile-time dei livelli disabilitati.

Esempio:

```cpp
#define LOG_COMPILED_LEVEL LOG_LEVEL_INFO
```

Risultato:

```text
DEBUG completamente escluso dalla build
```

---

## Step 6 — Implementare factory comode

Senza factory, la creazione del logger sarà molto verbosa.

Meglio prevedere:

```cpp
Logger LoggerFactory::createStdoutLogger(
    const std::string& name,
    LogLevel level
);
```

```cpp
Logger LoggerFactory::createFileLogger(
    const std::string& name,
    const std::string& path,
    LogLevel level
);
```

Uso finale desiderabile:

```cpp
auto dbLogger = LoggerFactory::createFileLogger(
    "Database",
    "database.log",
    LogLevel::Debug
);

auto uiLogger = LoggerFactory::createStdoutLogger(
    "UI",
    LogLevel::Warning
);
```

---

## Step 7 — Test minimi

Testerei almeno:

```text
Filtro livelli
Scrittura stdout
Scrittura file
Formato JSON valido
Escaping JSON
Logger multipli
Thread safety base
Compile-time disable DEBUG
```

In particolare:

```text
- DEBUG disabilitato runtime
- DEBUG disabilitato compile-time
- due logger con livelli diversi
- due logger su due file diversi
- messaggio con virgolette
- messaggio con backslash
- messaggio con newline
```

---

# 12. Step futuri già predisposti

## Futuro A — AsyncDispatcher

Quando vuoi passare ad asincrono, aggiungi:

```text
AsyncDispatcher
```

Senza cambiare:

```text
Logger
ILogSink
ILogFormatter
LogRecord
macro
codice applicativo
```

L’API rimane uguale:

```cpp
logInfo(logger, "Messaggio");
```

Cambia solo la costruzione:

```cpp
LoggerFactory::createAsyncFileLogger(...)
```

---

## Futuro B — RotatingFileSink

Aggiungi:

```text
RotatingFileSink
```

Senza cambiare `Logger`.

Strategie future:

```text
rotazione per dimensione
rotazione giornaliera
mantenimento ultimi N file
```

---

## Futuro C — Configurazione da file

Aggiungi:

```text
LoggerConfigLoader
LoggerRegistry
SinkFactory
FormatterFactory
```

Il file di configurazione crea i logger.

---

## Futuro D — SerialSink

Aggiungi:

```text
SerialSink
```

Oppure meglio:

```text
IByteStream
SerialByteStream
SerialSink
```

Perché la seriale è OS-specifica e conviene tenerla sotto un’ulteriore astrazione.

---

# 13. Struttura progetto consigliata

```text
logging-lib/
├── include/
│   └── logging/
│       ├── LogLevel.hpp
│       ├── LogRecord.hpp
│       ├── LoggerConfig.hpp
│       ├── Logger.hpp
│       ├── LoggerFactory.hpp
│       ├── ILogSink.hpp
│       ├── ILogFormatter.hpp
│       ├── ILogDispatcher.hpp
│       ├── sinks/
│       │   ├── StdoutSink.hpp
│       │   └── FileSink.hpp
│       ├── formatters/
│       │   └── JsonFormatter.hpp
│       ├── dispatchers/
│       │   └── SyncDispatcher.hpp
│       └── macros/
│           └── LogMacros.hpp
│
├── src/
│   ├── Logger.cpp
│   ├── LoggerFactory.cpp
│   ├── sinks/
│   │   ├── StdoutSink.cpp
│   │   └── FileSink.cpp
│   ├── formatters/
│   │   └── JsonFormatter.cpp
│   └── dispatchers/
│       └── SyncDispatcher.cpp
│
├── examples/
│   └── basic.cpp
│
├── tests/
│   └── LoggerTests.cpp
│
└── CMakeLists.txt
```

---

# 14. Sequenza di sviluppo concreta

Ordine pratico:

```text
1. LogLevel
2. LogRecord
3. LoggerConfig
4. ILogSink
5. StdoutSink
6. FileSink
7. ILogFormatter
8. JsonFormatter
9. ILogDispatcher
10. SyncDispatcher
11. Logger
12. LoggerFactory
13. Macro LOG_*
14. Test base
15. Esempi d’uso
```

---

# 15. Decisione architetturale principale

La scelta più importante è questa:

```text
Logger non scrive direttamente.
Logger crea record e li passa a un Dispatcher.
Dispatcher usa Formatter + Sink.
```

Schema finale:

```text
Codice applicativo
      ↓
logInfo(dbLogger, "Messaggio")
      ↓
Logger
      ↓
SyncDispatcher
      ↓
JsonFormatter
      ↓
FileSink / StdoutSink
```

In futuro:

```text
Codice applicativo
      ↓
logInfo(dbLogger, "Messaggio")
      ↓
Logger
      ↓
AsyncDispatcher
      ↓
Queue
      ↓
Worker Thread
      ↓
JsonFormatter
      ↓
FileSink / SerialSink / RotatingFileSink
```

Questa struttura soddisfa i tuoi vincoli principali: semplice nella V1, ma aperta a seriale, asincrono, rotazione file, configurazione runtime e logger multipli senza rifare il core.
