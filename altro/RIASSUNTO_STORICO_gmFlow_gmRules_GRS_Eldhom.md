# Riassunto Storico Pratico — gmFlow, gmRules/GRS, Add-on Runtime, Eldhom

## Scopo del documento

Conservare in modo operativo le decisioni principali emerse e consolidate nel thread, così da poter chiudere la conversazione senza perdere il contesto architetturale per i prossimi thread.

## 1) gmFlow — Decisioni chiave confermate

- Modello principale adottato: timeline continua, non round classici.
- Flow esteso in modo additivo e retrocompatibile: nessuna rottura di API esistenti.
- Introduzione di FlowPhase e PhaseContext come primitive di composizione gerarchica.
- Stato condiviso tra livelli (GameState, ActorRegistry, EventBus) ma contatori di fase locali isolati.
- Nessun limite hard di nesting nel framework (limite pratico consigliato: pochi livelli).
- ActionQueue resta unica e globale per sessione.
- TimelineFlowController resta generico: decide quando/chi agisce, non il significato di gioco dell’azione.

### Implicazione pratica

Per Eldhom il controllo turni/ordine attori deve restare in gmFlow TimelineFlowController; la logica dominio (carta, danno, formazione, trigger missione) non va inglobata nel controller.

## 2) gmRules + GRS — Decisioni chiave confermate

- Separazione netta tra:
  - descrizione regole (JSON/GRS/YAML),
  - registry di attivazione gruppi (RuleGroupRegistry),
  - esecuzione (RuleBook + gmRulesEngine + resolver).
- RuleGroupRegistry risponde al cosa è attivo (WHAT), non esegue regole.
- RuleBook risponde al come eseguire la regola (HOW) da RuleId a effects/conditions.
- RuleBookLoader carica definizioni da JSON verso RuleBook.
- gmRulesEngine espone funzioni di caricamento e risoluzione lato runtime.
- Scelta mantenuta: parser/loader senza dipendenze esterne per mantenere isolamento libreria.

### Implicazione pratica

Le regole possono essere evolute senza riscrivere il motore: si aggiorna la definizione (GRS/JSON), si ricarica e si risolve via RuleId.

## 3) Capacità di creare regole add-on a runtime

### Decisione confermata: SI, supporto previsto e già impostato

- Caricamento regole a runtime tramite API di gmRulesEngine (load da file/stringa).
- Comportamento di accumulo confermato: più load aggiungono definizioni, non azzerano automaticamente quelle esistenti.
- Runtime refs in GRS (input.xxx, event.xxx) usati per rendere le regole parametriche e riutilizzabili.
- Bridge runtime responsabile di fornire i binding concreti al momento dell’esecuzione.
- RuleGroupRegistry abilita/disabilita gruppi attivi; RuleBook/gmRulesEngine eseguono.

### Vincoli pratici

- Le regole add-on devono rispettare capability realmente disponibili nel RuleContext/adapter runtime del gioco.
- Feature non ancora mappate nel runtime vanno dichiarate come estensioni additive pianificate, non emulate in modo ambiguo.

## 4) Decisioni per il nuovo gioco Eldhom

- Identità gameplay confermata:
  - dungeon crawler card-based,
  - deckbuilding,
  - timeline continua,
  - nessun round classico,
  - formazioni Frontline/Backline,
  - gruppi mostri con comportamento autonomo.
- Scelta architetturale confermata: separazione forte Core C++ e GUI Python/PySide6.
- Strategia di sviluppo confermata: riuso elevato delle librerie gmXxx, con estensioni minori solo additive.
- SequenceEngine mantenuto come macchina deterministica con stato esplicito (niente stato globale nascosto).
- Distinzione Schieramento vs Scompaginamento trattata come semantica di dominio dedicata.
- TargetingFilter trattato come query pura (non muta stato).
- Monster reaction: consumo carta comportamento e nuova pescata secondo regola.

### Integrazione librerie per Eldhom (sintesi)

- gmFlow: ordine attori e avanzamento timeline.
- gmRules: risoluzione effetti e condizioni carta/azione.
- gmAlea: deck/zone lifecycle per mano, scarti, memoria, mazzo.
- gmActor: stato attori, formazione, behavior/reaction lato mostri.
- gmDispatch: eventi tra componenti e bridge GUI.

## 5) Decisioni operative emerse dal thread (taglio pratico)

- Per comunicazione Core↔GUI, l’engine resta source of truth per stato gioco e flusso.
- GUI deve reagire a eventi serializzati dal Core, non ricostruire regole di dominio localmente.
- Le correzioni di integrazione devono privilegiare wiring/event routing e contratti evento stabili.

## 6) Cosa considerare “chiuso” e cosa “aperto”

### Chiuso (decisioni già prese)

- Timeline-first (no round) come modello di flow per Eldhom.
- Regole separate da runtime engine e attivazione gruppi.
- Add-on runtime delle regole come capacità supportata e desiderata.
- Architettura Core C++ / GUI Python con responsabilità separate.

### Aperto (da rifinire nei prossimi thread)

- Elenco definitivo capability GRS abilitate in runtime Eldhom (matrice feature -> adapter).
- Policy di merge/override quando più pacchetti add-on definiscono stessa RuleId.
- Governance versioning contenuti regola (compatibilità savegame e migrazioni).

## 7) Decision log sintetico (usabile nei prossimi thread)

- DLOG-01: Eldhom usa timeline continua gmFlow, non round turn-based classico.
- DLOG-02: gmRules separa attivazione gruppi (RuleGroupRegistry) ed esecuzione regole (RuleBook/Engine).
- DLOG-03: Regole add-on runtime abilitate via caricamento dinamico e runtime refs GRS.
- DLOG-04: Core C++ mantiene autorità su stato e regole; GUI è consumer/event-driven.
- DLOG-05: Estensioni a gmXxx ammesse solo additive e retrocompatibili.

## Riferimenti base usati per questo riassunto

- gmFlow/FlowPhase_PLAN.md
- gmFlow/gmFlow_TimelineFlowController_implementation_instructions.md
- gmRules/PLAN.md
- gmRules/specs/grs-spec.md
- tools/GRS_MANUAL.md
- GAME/Eldhom/info/PLAN.md
