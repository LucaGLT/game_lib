# game_lib

Raccolta di librerie C++17 per la costruzione di game app tabletop.

## Librerie

- `gmSave/`: serializzazione JSON tipizzata (con versioning e gerarchia eccezioni)
- `gmLog/`: logging strutturato JSON Lines con filtri compile-time/runtime
- `gmMap/`: mappa topologica generica basata su grafo (locations, tiles, adjacency, items, metadata)

## Stato sviluppo

- `gmSave`: implementata e testata
- `gmLog`: implementata e pronta all'integrazione
- `gmMap`: completata fino a **Phase 10** (`PLAN.md`)

## Documentazione

- `gmSave/gmSave_API.md`
- `gmLog/gmLog_API.md`
- `gmMap/gmMap_API.md`

## Build rapida (Windows + clang++)

Esempio di compilazione test gmMap (fasi 2-4):

```powershell
clang++ -std=c++17 -I. gmMap/tests/test_phases_2_4.cpp gmLog/*.cpp gmLog/sinks/*.cpp gmLog/formatters/*.cpp gmLog/dispatchers/*.cpp -o test_phases_2_4.exe
```

