# Tool per file `.grs`

scritto in python
per comandi CLI

## Funzionalità

- `lint`: valida la sintassi dei file `.grs`
- `validate`: applica validazioni semantiche al file `.grs`
- `yaml`: genera YAML canonico da `.grs`
- `grapho`: genera diagrammi Mermaid delle regole descritte in file `.grs`

ES:

> grs -lint "percorso/del/file.grs" -o "file_v2.grs"
> grs -yaml "percorso/del/file.grs" -o "file.yaml"
> grs -grapho "percorso/del/file.grs" -o "file.md"


---

## Opinione e suggerimenti pratici

### 1. Sintassi CLI — usa sottocomandi, non flag

Il tuo `grs -lint file.grs` è una flag, non un sottocomando. La convenzione Python standard (argparse subparsers) è:

```powershell
grs lint  file.grs [-o output.grs]
grs yaml  file.grs [-o output.yaml]
grs grapho file.grs [-o output.md] [--rule Base_Move]
grs check file.grs           # alias lint + validate insieme
```

Questo permette `--help` per ogni sottocomando e si comporta come `git`, `pip`, `cmake`.

### 2. Architettura a strati — il parser è il fondamento

Tutte e 4 le funzionalità dipendono da un AST corretto. L'ordine logico è:

```text
.grs file
    │
    ▼
  Lexer (tokenizzatore a regex)
    │
    ▼
  Parser (ricorsivo discendente, stdlib pura — niente dipendenze esterne)
    │
    ▼
  AST (dataclasses)
    │
    ├─► Linter      → errori di sintassi con numero riga
    ├─► Validator   → errori semantici (le 10 regole della spec)
    ├─► YamlGen     → YAML canonico
    └─► GraphGen    → Mermaid .md
```

### 3. No dipendenze esterne

Il progetto evita dipendenze esterne nelle librerie C++. Vale anche qui: solo stdlib Python (`re`, `argparse`, `pathlib`, `dataclasses`, `sys`). Il YAML lo genero come stringa formattata (o con `PyYAML` opzionale).

### 4. Aggiunta utile: `--rule NAME` su `grapho`

`grapho` su un file completo genera un grafo molto denso. Con `--rule Base_Move` visualizzi solo quella regola e i trigger connessi, come nell'esempio che hai già approvato.

### 5. Formato errori lint/validate

Utile supportare due formati di output per gli errori, selezionabili con `--format`:
- `text` (default): `[ERROR] line 42: condition C_Foo not defined`
- `json`: array JSON, utile per integrazione con editor/CI

### 6. Dove vive il tool nel workspace

Propongo `tools/grs/` alla radice del workspace — non dentro gmRules perché il tool non è codice C++ e potrebbe in futuro servire altri file GRS fuori da gmRules.

---

## Piano d'azione

### Fase 1 — Struttura progetto e Lexer

- Crea `tools/grs/` con `__main__.py`, `cli.py`, `lexer.py`, `ast_nodes.py`
- Implementa il Lexer: tokenizza keyword (`@meta`, `@end`, `::`, `IF`, `ON`, `THEN`, `AND`, `OR`, `NOT`, `AND THEN`), identificatori, literal stringa, interi, commenti `#`, ref runtime (`input.x`, `event.x`)
- Test manuale sul file turn-card-dungeon.example.grs

### Fase 2 — Parser e AST

- Implementa parser ricorsivo discendente che produce un `GrsDocument` con i 7 blocchi come liste di dataclass
- Nodi AST: `MetaBlock`, `TargetDef`, `ConditionDef`, `EffectDef`, `StatusDef`, `RuleDef`, `TriggerDef`
- Ogni nodo porta il numero di riga sorgente (essenziale per errori)
- Gestione condizioni composte ricorsive (`AND`, `OR`, `NOT`, parentesi)

### Fase 3 — `lint` (sintassi)

- Verifica struttura blocchi: ogni blocco aperto ha `@end`
- Verifica che ogni definizione abbia `::` e argomenti nel formato atteso
- Verifica che i token obbligatori siano presenti (es. `TargetKind` + `TargetSelector` in `@targets`)
- Output: lista `LintError(line, message, severity)`

### Fase 4 — `validate` (semantica)

- Implementa le 10 regole di validazione della spec (nomi non definiti, tipi non nel vocabolario, ref `input.xxx` in trigger, cicli in condizioni, status non dichiarati, ecc.)
- Output: stesso formato di lint, distingue `ERROR` da `WARNING`

### Fase 5 — `yaml`

- Traversal dell'AST → dict Python → serializzazione YAML
- Segue il mapping `GRS → YAML` della spec (tabella in fondo al documento)
- Supporta `-o file.yaml` oppure stdout

### Fase 6 — `grapho`

- Traversal dell'AST → grafo Mermaid nello stile che hai approvato
- `--rule NAME`: filtra una regola + trigger correlati
- Senza `--rule`: genera un diagramma per ogni regola come sezioni separate nel `.md`
- Status referenziati mostrati come nodi arancioni con frecce tratteggiate

### Fase 7 — CLI e packaging

- `cli.py` con argparse subparsers
- `python -m grs` come entry point
- `--format text|json` per lint/validate
- `-o` opzionale per yaml/grapho

---

**Struttura file risultante:**

```text
tools/
└── grs/
    ├── __main__.py       ← entry point: python -m grs
    ├── cli.py            ← argparse, routing sottocomandi
    ├── lexer.py          ← tokenizzatore regex
    ├── parser.py         ← parser ricorsivo discendente
    ├── ast_nodes.py      ← dataclasses AST
    ├── linter.py         ← fase 3
    ├── validator.py      ← fase 4
    ├── yaml_gen.py       ← fase 5
    ├── graph_gen.py      ← fase 6
    └── tests/
        ├── test_lexer.py
        ├── test_parser.py
        └── test_commands.py
```
