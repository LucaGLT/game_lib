=========================
# Libreria : gmInteraction
=========================

# 1. Mappa delle esigenze

## Obiettivo della libreria

Fornire un registro autorevole degli **oggetti interagibili** (porte, leve,
forzieri, …) presenti su una mappa di gioco. La libreria possiede i *dati*
dell'oggetto (tipo, stato del ciclo di vita, metadati liberi) mentre delega a
`gmMap` la **collocazione spaziale** tramite id opachi `InteractableObjectId`.

La libreria è completamente standalone: dipende solo da `gmSave` per gli
snapshot versionati e tocca `gmMap` esclusivamente tramite l'adapter in
`bridges/`.

---

# 2. Vincoli architetturali

- C++17 Standard — nessuna feature C++20 o successiva.
- Nessuna dipendenza esterna oltre `gmSave` (snapshot) e, solo nei `bridges/`,
  `gmMap`.
- `using InteractableObjectId = std::uint64_t;` — alias opaco condiviso con
  `gmMap` (`0` = non assegnato).
- Eccezione base unica: `EInteractionError : std::runtime_error`
  (prefisso messaggio `"EInteractionError: "`).
- Include guard `#ifndef GMINTERACTION_FILENAME_HPP` — niente `#pragma once`.
- Tabulazioni (width 4), parentesi Allman, limite 100 colonne.

---

# 3. Componenti principali

| Componente | File | Ruolo |
|------------|------|-------|
| `InteractableObject` | `InteractableObject.hpp` | POD: `id`, `type`, `state`, `meta`. |
| `InteractionState` | `InteractionState.hpp` | enum `IDLE/ACTIVE/USED/LOCKED/DISABLED` + conversioni stringa. |
| `InteractableObjectStore` | `InteractableObjectStore.hpp/.cpp` | Registro in-memory + snapshot v1. |
| `MapInteractionBridge` | `bridges/MapInteractionBridge.hpp` | Unico punto che include `gmMap`. |
| Eccezioni | `GmInteractionError.hpp` | Base + `EDuplicate/EUnknownObject/EUnknownMetaKey`. |

---

# 4. Regole per l'agente AI

1. **Non aggiungere** `#include "gmMap/..."` fuori da `bridges/`.
2. Lo store **non** memorizza la posizione: la collocazione è di `gmMap`.
3. `remove_meta` su chiave assente è **no-op idempotente** (non è un errore di
   contratto).
4. `to_json`/`from_json` per `InteractableObject` devono stare in
   `namespace gmInteraction` (non anonimo) per la corretta risoluzione ADL.
5. Ogni snapshot passa da `gmSave::save_versioned`/`load_versioned`
   (`SNAPSHOT_VERSION = 1`); il versioning non va aggirato.
6. Lanciare le eccezioni solo su violazioni di contratto, mai come flusso di
   controllo.

---

# 5. Test

- Runner self-contained: `tests/test_gmInteraction.cpp` (nessuna dipendenza da
  `gmLog`).
- Copertura: create/query, stato/metadati, remove/clear, round-trip stringa di
  stato, bridge con `gmMap`, round-trip snapshot.
- Stato corrente: **29/29 PASS**.

Build rapida (clang++):

```text
clang++ -std=c++17 -I. -IgmInteraction \
  gmInteraction/tests/test_gmInteraction.cpp \
  gmInteraction/InteractableObjectStore.cpp \
  gmInteraction/gmInteraction.cpp \
  gmSave/gmSave.cpp -o bin/exe/test_gmInteraction.exe
```
