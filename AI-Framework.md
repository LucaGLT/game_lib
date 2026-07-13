# Report Analisi AI Framework (.github)

## Obiettivo

Valutare il framework AI presente nella cartella `.github` del repository `game_lib`, con focus su:

- attività concretamente supportate;
- livello di maturità del sistema;
- grado di riuso fuori dal dominio specifico della libreria.

## Perimetro Analizzato

Sono stati analizzati questi blocchi:

- Istruzioni operative: `.github/instructions/*`
- Prompt di workflow: `.github/prompts/*`
- Specifiche machine-readable: `.github/specs/*`
- Regole globali workspace: `.github/copilot-instructions.md`

## Executive Summary

Il contenuto in `.github` costituisce un framework AI strutturato, non una raccolta casuale di prompt.
Il sistema copre governance, quality gates, scaffolding, review, pianificazione e documentazione.

Valutazione sintetica:

- Copertura funzionale: alta
- Formalizzazione regole: alta (istruzioni + schema YAML)
- Riusabilità generale: medio-alta
- Specificità game_lib: presente soprattutto su naming, struttura librerie e vincoli cross-library

## Capacità Supportate

### 1) Governance e comportamento agente

File principale:

- `.github/copilot-instructions.md`

Capacità:

- definizione standard globali di progetto;
- enforcement di vincoli tecnici permanenti;
- linee guida per planning, coding, review e documentazione.

### 2) Regole operative per tipo di file

File chiave:

- `.github/instructions/cpp-style.instructions.md`
- `.github/instructions/code-review.instructions.md`
- `.github/instructions/documentation.instructions.md`
- `.github/instructions/planning.instructions.md`
- `.github/instructions/markdown-style.instructions.md`
- `.github/instructions/python-gui-style.instructions.md`

Capacità:

- standard C++ uniformi e verificabili;
- review guidata con checklist e severità;
- standard Doxygen + sincronizzazione documentazione API;
- gestione roadmap tramite `PLAN.md`;
- qualità Markdown (markdownlint-aligned);
- separazione rigorosa logica/stile in GUI Python (QSS + temi).

### 3) Workflow preconfigurati via prompt

File chiave:

- `.github/prompts/new-library.prompt.md`
- `.github/prompts/update-plan.prompt.md`
- `.github/prompts/new-class.prompt.md`
- `.github/prompts/review-code.prompt.md`
- `.github/prompts/generate-docs.prompt.md`
- `.github/prompts/new-gui-widget.prompt.md`
- `.github/prompts/new-gui-theme.prompt.md`
- `.github/prompts/apply-gui-theme.prompt.md`

Capacità:

- creazione e aggiornamento roadmap;
- scaffold classi/moduli;
- review con output standardizzato;
- generazione/completamento documentazione;
- creazione e audit compliance di widget/temi GUI.

### 4) Contratti machine-readable (schema YAML)

File chiave:

- `.github/specs/cpp-style.yml`
- `.github/specs/library-structure.yml`
- `.github/specs/plan-schema.yml`
- `.github/specs/review-plan-schema.yml`
- `.github/specs/gui-theme.yml`

Capacità:

- regole traducibili in verifiche automatizzate;
- output strutturati e coerenti tra agenti diversi;
- riduzione ambiguità tra “linea guida” e “requisito”.

## Riusabilità Fuori da game_lib

### Componente fortemente riusabile (generica)

Questi blocchi sono riutilizzabili con modifiche minime in altri progetti SW:

- processo di code review con schema output e severità;
- schema e metodo di pianificazione incrementale (`PLAN.md`);
- pipeline documentazione tecnica (Doxygen + API markdown);
- regole di qualità Markdown;
- framework theming GUI Python/Qt con separazione stile-logica.

### Componente riusabile ma da parametrizzare

Questi elementi funzionano bene anche altrove, ma richiedono adattamento:

- naming convention C++ specifiche `gm*` / `Gm*`;
- struttura librerie con vincoli su `bridges/`;
- prompt di scaffolding che assumono namespace e taxonomy locali.

### Componente specifica game_lib

Elementi strettamente legati al dominio attuale:

- pattern namespace e naming dedicati alle librerie `gm*`;
- invarianti su dipendenze tra librerie interne;
- alcune sottostrutture cartella orientate alle librerie del progetto.

## Conclusioni

Il framework AI attuale è maturo e già utilizzabile come base metodologica per sviluppo software oltre `game_lib`.
La parte più trasferibile è il modello di processo (review, planning, documentazione, qualità, compliance GUI).

Per il riuso cross-progetto è sufficiente estrarre un “core agnostico” e mantenere separati gli adattatori
dominio-specifici (naming e struttura librerie).

## Raccomandazioni Operative

1. Estrarre un core riusabile in una cartella dedicata (ad esempio `.github/framework-core`).
2. Spostare le regole specifiche `game_lib` in un layer adattatore (ad esempio `.github/framework-game-lib`).
3. Parametrizzare i prompt tramite variabili (namespace, convenzioni naming, struttura modulo).
4. Mantenere gli schema YAML come source of truth per automatizzare validazioni future.