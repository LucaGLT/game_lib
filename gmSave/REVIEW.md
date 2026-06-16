# Code Review — gmSave

| Field | Value |
|---|---|
| Reviewed scope | gmSave (headers, sources, tests) |
| Date | 2026-06-12 |
| Rule-set version | style-rules.md v1.3 |
| Reviewer | AI (GitHub Copilot) |

---

## Summary

| Category | Status | 🔴 | 🟡 | 🔵 | Total |
|---|---|---:|---:|---:|---:|
| CAT-1 Naming | 🔴 Errors | 6 | 0 | 0 | 6 |
| CAT-2 Guards | 🔴 Errors | 1 | 0 | 0 | 1 |
| CAT-3 Formatting | 🔴 Errors | 2 | 0 | 0 | 2 |
| CAT-4 Spacing | ✅ Clean | 0 | 0 | 0 | 0 |
| CAT-5 Switch/Case | ✅ Clean | 0 | 0 | 0 | 0 |
| CAT-6 Signatures | 🟡 Warnings | 0 | 1 | 0 | 1 |
| CAT-7 Includes | 🔴 Errors | 1 | 0 | 0 | 1 |
| CAT-8 Preprocessor | ✅ Clean | 0 | 0 | 0 | 0 |
| CAT-9 Documentation | 🔵 Info | 0 | 0 | 1 | 1 |
| CAT-10 Constraints | 🔴 Errors | 1 | 0 | 0 | 1 |
| TOTAL |  | **11** | **1** | **1** | **13** |

---

## Findings

### CAT-1 · Naming

1. F-01 · NS-1 · 🔴 Error
- File: gmSave/gmSave.hpp, gmSave/gmSave.cpp, gmSave/tests/test_gmSave.cpp
- Location: namespace declarations/usages `GmSave`
- Description: namespace non conforme (`gm` + PascalCase richiesto).
- Correction: `GmSave` -> `gmSave`.

2. F-02 · EX-2 · 🔴 Error
- File: gmSave/gmSave.hpp
- Location: base exception `SaveError`
- Description: manca prefisso `E` richiesto per eccezioni libreria.
- Correction: `SaveError` -> `ESaveError`.

3. F-03 · EX-1 · 🔴 Error
- File: gmSave/gmSave.hpp
- Location: `FileWriteError`, `FileReadError`, `JsonParseError`, `VersionMismatchError`
- Description: eccezioni senza prefisso `E`.
- Correction: `EFileWriteError`, `EFileReadError`, `EJsonParseError`, `EVersionMismatchError`.

4. F-04 · FN-1 · 🔴 Error
- File: gmSave/tests/test_gmSave.cpp
- Location: metodi `checkThrows`, `checkNoThrow`
- Description: metodi in camelCase; richiesto snake_case.
- Correction: `check_throws`, `check_no_throw`.

5. F-05 · VAR-1 · 🔴 Error
- File: gmSave/tests/test_gmSave.cpp
- Location: parametri/metavariabili test (`testName`, ecc.)
- Description: variabili locali/parametri non in snake_case.
- Correction: rinominare in snake_case.

6. F-06 · EX-2 · 🔴 Error
- File: gmSave/gmSave.cpp
- Location: constructor body `VersionMismatchError::VersionMismatchError`
- Description: nome classe eccezione non conforme.
- Correction: allineare al rename `EVersionMismatchError`.

### CAT-2 · Guards

7. F-07 · IG-1 · 🔴 Error
- File: gmSave/gmSave.hpp
- Location: include guard `GMSAVE_HPP`
- Description: guard non conforme al pattern `<NAMESPACE>_<FILENAME>_HPP`.
- Correction: `GMSAVE_GMSAVE_HPP`.

### CAT-3 · Formatting

8. F-08 · FMT-1 · 🔴 Error
- File: gmSave/gmSave.hpp, gmSave/gmSave.cpp, gmSave/tests/test_gmSave.cpp
- Description: indentazione a spazi diffusa.
- Correction: conversione a tab reali (width 4).

9. F-09 · FMT-2 · 🔴 Error
- File: gmSave/gmSave.hpp, gmSave/gmSave.cpp, gmSave/tests/test_gmSave.cpp
- Description: molte aperture `{` su stessa riga (K&R) invece di Allman.
- Correction: portare `{` su riga propria nei costrutti applicabili.

### CAT-6 · Signatures

10. F-10 · SIG-3 · 🟡 Warning
- File: gmSave/gmSave.hpp, gmSave/gmSave.cpp
- Description: alcune firme multilinea oltre 100 colonne non hanno `)` su riga propria.
- Correction: adattare firme lunghe a stile SIG-3.

### CAT-7 · Includes

11. F-11 · INC-2 · 🔴 Error
- File: gmSave/tests/test_gmSave.cpp
- Location: `#include "../gmSave.hpp"`, `#include "../../gmLog/..."`
- Description: uso di `..` vietato negli include relativi.
- Correction: includere da root libreria (`"gmSave/gmSave.hpp"`, `"gmLog/..."`).

### CAT-9 · Documentation

12. F-12 · DOC-1 · 🔵 Info
- File: gmSave/gmSave.hpp
- Location: attributi pubblici `expected_version` / `found_version`
- Description: documentazione minima presente ma migliorabile con testo più esplicito.
- Correction: mantenere inline docs coerenti dopo rename.

### CAT-10 · Constraints

13. F-13 · Constraints · 🔴 Error
- File: gmSave/gmSave.hpp
- Location: documentazione namespace e dipendenze
- Description: naming/API docs non coerenti con naming di libreria aggiornato.
- Correction: allineare riferimenti a `gmSave` e nuova gerarchia eccezioni.

---

## Correction Plan

### Phase 1 — Rename pervasivo (mandatory)
1. Namespace `GmSave` -> `gmSave` (hpp/cpp/tests).
2. Eccezioni `SaveError` family -> `E...` family.
3. Aggiornare tutti i riferimenti nei test (`checkThrows`/`checkNoThrow` e parametri camelCase).

### Phase 2 — Formatting and guards (mandatory)
4. Fix include guard `GMSAVE_GMSAVE_HPP`.
5. Allman braces dove non conformi.
6. Conversione tabs su tutti i file C++ di gmSave.

### Phase 3 — Includes and signatures (mandatory)
7. Eliminare include con `..` in test_gmSave.cpp.
8. Applicare SIG-3 alle firme lunghe rimanenti.

### Phase 4 — Documentation alignment
9. Aggiornare commenti Doxygen e riferimenti testuali a namespace/eccezioni rinominati.

---

End of review.
