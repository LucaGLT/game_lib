==============================
# Libreria : gmDispatch
==============================

# 1. Contesto e obiettivo

## Posizione nel progetto

`gmDispatch` è la libreria di comunicazione asincrona (o sincrona V1) per
il game engine `game_lib`.  Disaccoppia i produttori di messaggi/eventi (es.
`CoreEngine`, `InputSystem`) dai consumatori (es. UI-CLI, UI-GUI, AI, Logger).

La sua architettura riprende deliberatamente lo schema di `gmLog`
(Dispatcher → Router → Channel), ma generalizza:
- il payload da `std::string` a `std::any`
- il routing da 1:1 (un logger, un sink) a 1:N dinamico

## Relazione con gmLog

| Aspetto              | gmLog              | gmDispatch                      |
|----------------------|--------------------|---------------------------------|
| Non toccare gmLog    | [OK] rimane invariato | questa lib è indipendente        |
| Bridge opzionale     | —                  | `LogDispatchBridge` (Phase 4)   |

---

# 2. Stile del codice

- Segui sempre lo stile di `gmLog`: guard `#ifndef`, doxygen `@brief/@param/@return`
- Namespace: `GmDispatch`
- Niente `auto` come tipo di ritorno nei prototipi pubblici
- Nessuna dipendenza esterna (solo C++17 stdlib)
- I commenti di TODO nelle stub usano il pattern: `// TODO: Phase N — descrizione`
- Ogni TODO deve indicare il numero di fase (2, 3, 4)

---

# 3. Include chain (ordine di dipendenza)

```
Envelope.hpp           ← nessuna dipendenza gmDispatch
IChannel.hpp           ← Envelope.hpp
ISerializer.hpp        ← Envelope.hpp
IRouter.hpp            ← IChannel.hpp
IDispatcher.hpp        ← IChannel.hpp, Envelope.hpp
DispatcherConfig.hpp   ← <string>
Dispatcher.hpp         ← DispatcherConfig.hpp, Envelope.hpp, IDispatcher.hpp, IChannel.hpp
DispatcherFactory.hpp  ← Dispatcher.hpp

channels/EventBusChannel.hpp  ← ../IChannel.hpp
channels/StdoutChannel.hpp    ← ../IChannel.hpp, ../ISerializer.hpp
serializers/JsonSerializer.hpp← ../ISerializer.hpp
routers/SyncRouter.hpp        ← ../IRouter.hpp   (NO mutex proprio)
dispatchers/SyncDispatcher.hpp← ../IDispatcher.hpp, ../IRouter.hpp
```

---

# 4. Regole di threading

- `SyncDispatcher` possiede l'unico `std::mutex`; lo acquisisce per
  `dispatch()`, `subscribe()`, `unsubscribe()` e `flush()`.
- `SyncRouter` NON ha un proprio mutex (sarebbe double-locking).
- `EventBusChannel` chiama le callback direttamente nel thread del dispatcher:
  le callback devono essere thread-safe se necessario.

---

# 5. Payload (std::any)

Il payload è `std::any`.  Il ricevente usa `std::any_cast<T>(env.payload)`.
Nessun tipo di payload è obbligatorio.  Per dati senza payload, lasciare
`env.payload` vuoto (`std::any{}`).

`JsonSerializer` serializza il payload nel campo `"payload"` come stringa
`env.payload.type().name()` se non esiste un serializer specifico registrato
(Phase 3 feature per serializer custom per tipo).

---

# 6. Estensioni future (non implementare prima della fase corrente)

| Estensione              | Fase | Note                                         |
|-------------------------|------|----------------------------------------------|
| `AsyncDispatcher`       | 4    | queue + worker thread + condition_variable   |
| `FileChannel`           | 3    | usa JsonSerializer                           |
| `IpSocketChannel`       | 3    | interfaccia generica; impl OS-specifica fuori|
| `WebSocketChannel`      | 3+   | via libreria esterna                         |
| Pattern-matching router | 4    | `"engine.*"` via wildcard                   |
| Targeted delivery       | 4    | route solo a canali in `Envelope::targets`   |
| `LogDispatchBridge`     | 4    | `LogRecord → Envelope` adapter               |
