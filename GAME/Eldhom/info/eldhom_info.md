# Le Pergamene di Eldhôm — Informazioni di Sintesi per lo Sviluppo

**Versione:** 0.1.0  
**Ultimo aggiornamento:** 2026-06-28  
**Scopo:** Raccolta coerente di tutto ciò che è rilevante per i prossimi sviluppi del gioco e della piattaforma engine.  
**Stato:** Fasi 0–5 complete ✅ | Fase 6+ in avvio 🚀

---

## 📋 Indice

1. [Identità del gioco](#1-identità-del-gioco)
2. [Architettura di sistema](#2-architettura-di-sistema)
3. [Librerie gmXxx — Ruolo e stato](#3-librerie-gmxxx--ruolo-e-stato)
4. [Stato di sviluppo e fasi](#4-stato-di-sviluppo-e-fasi)
5. [Regole base essenziali](#5-regole-base-essenziali)
6. [Dati di gioco — Carte, Etnie, Effetti](#6-dati-di-gioco--carte-etnie-effetti)
7. [Decisioni architetturali vincolanti](#7-decisioni-architetturali-vincolanti)
8. [Prossimi passi prioritari](#8-prossimi-passi-prioritari)
9. [Riferimenti file dettagliati](#9-riferimenti-file-dettagliati)

---

## 1. Identità del gioco

**Le Pergamene di Eldhôm** è un dungeon crawler avanzato basato su carte con:

- **Deckbuilding** — il PG costruisce il proprio mazzo da una collezione di carte diverse per etnia, affiliazione, ruolo, equipaggiamento.
- **Timeline continua** — nessun Round classico. Ogni attore (PG, mostro, PNG) ha una posizione sulla Linea Temporale; agisce chi è più indietro.
- **Formazioni tattiche** — Prima Linea / Retroguardia. Ogni locazione applica la regola **Retroguardia ≤ Prima Linea** per ogni fazione.
- **Gruppi mostri autonomi** — i mostri agiscono via Carte Comportamento gestite da un engine dedicato, non da script.
- **Sequenze di carte** — il PG può giocare sequenze di carte con stati specifici (Inizio / Continuo / Fine Sequenza).
- **Missioni dinamiche** — trigger su soglie temporali, locazioni e stati attori; vittoria/sconfitta gestite da evento.

**Architettura:** Core C++17 (gmXxx librerie) + GUI Python 3 / PySide6. Core ha autorità assoluta; GUI è client event-driven.

Dettagli complete: 📄 [regole_base_eldhom_ai_spec.md](regole_base_eldhom_ai_spec.md)

---

## 2. Architettura di sistema

```
┌─────────────────────────────────────────────────────────────┐
│ PySide6 GUI  (GAME/Eldhom/GUI/)                             │
│ TimelineWidget  FormationWidget  SequenceWidget             │
│ BehaviorCardWidget  MultiHeroPanelWidget  (deck, map, flow) │
└──────────────────────┬──────────────────────────────────────┘
                       │ ZMQ events (9210) / commands (9211)
┌──────────────────────▼──────────────────────────────────────┐
│ EldhomEngine  (GAME/Eldhom/CoreEngine/)                     │
│  ├─ SequenceEngine (state machine carta)                    │
│  ├─ FormationEngine (validazione formazioni)                │
│  ├─ BehaviorCardResolver (turno mostri)                     │
│  ├─ MissionEventSystem (trigger missione)                   │
│  └─ EldhomRuleAdapter (bridge gmRules)                      │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│ gmXxx Libraries (NO MODIFICATION — solo wrapper sottili)    │
│  gmFlow, gmActor, gmAlea, gmRules, gmMap, gmDispatch, gmLog │
└──────────────────────────────────────────────────────────────┘
```

### Principi cardine

1. **Core comanda, GUI visualizza.** La GUI non prende mai decisioni di logica di gioco.
2. **Autorità assoluta Engine.** Stato gioco, regole, flusso vivono solo nel Core C++.
3. **Comunicazione asincrona.** Core → GUI: eventi serializzati JSON via TCP. GUI → Core: comandi inviati via TCP.
4. **Riuso massimo.** ~75% della logica è già coperta dalle librerie gmXxx; Eldhôm aggiunge solo la logica di dominio specifica.

Dettagli: 📄 [../PLAN.md](PLAN.md#architettura)

---

## 3. Librerie gmXxx — Ruolo e stato

### Tabella: Responsabilità per libreria

| Libreria | Componente | Ruolo in Eldhôm | Stato | Note |
|----------|-----------|--|---|---|
| **gmFlow** | `TimelineFlowController` | Ordine attori, avanzamento turni | ✅ v2.0 + F2 | Timeline continua; nessun round |
| **gmActor** | `HeroState`, `MonsterGroupState`, `BossState` | Stato attori PG/mostri/boss | ✅ v0.2 + F3,F4 | HP, status, affiliazione, posizione |
| **gmActor** | `FormationValidator`, `FormationResolver` | Validazione RG ≤ PL, Schieramento/Scompaginamento | ✅ F3 | 35/35 test; algoritmo priorità |
| **gmActor** | `BehaviorCardProcessor`, `BehaviorReactionSystem` | Risoluzione turni mostri, reazioni | ✅ F4 | 20/20 test; carte comportamento |
| **gmAlea** | `GmCompDeck`, 6 zone | Deck/mano/memoria/scarti | ✅ v3.0 | MainDeck, Hand, Memory, Discard, PlayArea, Banish |
| **gmAlea** | `SequenceEngine`, `CardType` | State machine sequenze di carte | ✅ F1 | 29/29 test; SINGLE/SEQ_START/SEQ_CONTINUE/SEQ_END/INSTANT |
| **gmRules** | `RuleBook`, `EffectResolver` | Risoluzione effetti carte/azioni | ✅ Phases 1–6 | 46 EffectType; no dipendenze esterne |
| **gmRules** | `RuleGroupRegistry`, `gmRulesEngine` | Attivazione gruppi, caricamento regole | ✅ Phases 1–6 | Supporta add-on runtime |
| **gmMap** | Locazioni, topologia | Mappa, connessioni, passabilità | ✅ | Delegato, integrabile a richiesta |
| **gmDispatch** | `EventBus`, protocollo JSON | Comunicazione evento Core ↔ GUI | ✅ | Usa ZMQ per porte 9210/9211 |
| **gmLog** | Structured logging | Logging gameplay e debug | ✅ | Delegato |

### Feature generiche (F1–F5) — Sono già in gmXxx e pronti per Eldhôm

| Feature | Componente | Ubicazione | Stato | Test |
|---------|-----------|-----------|-------|------|
| **F1** CardType + SequenceEngine | `gmAlea` | `gmAlea/SequenceEngine.hpp/.cpp` | ✅ | 29/29 ✅ |
| **F2** TimelineMilestoneSystem | `gmFlow` | `gmFlow/TimelineMilestoneSystem.hpp/.cpp` | ✅ | 19/19 ✅ |
| **F3** FormationValidator + Resolver | `gmActor` | `gmActor/FormationValidator.hpp` | ✅ | 35/35 ✅ |
| **F4** BehaviorCardProcessor + Reactions | `gmActor` | `gmActor/BehaviorCardProcessor.hpp` | ✅ | 20/20 ✅ |
| **F5** 4 Widget PySide6 generici | `gmGui` | `pyLib/gmGui/` | ✅ | Sintassi ✅ |

Documentazione API completa: 📄 [../../Game-Lib_readme.md](../../Game-Lib_readme.md)

---

## 4. Stato di sviluppo e fasi

### Fasi completate ✅

| Fase | Componente | Stato | Riferimento |
|------|-----------|-------|-------------|
| **P0** | Analisi regolamento e mapping librerie | ✅ | [PLAN.md § Phase 0](PLAN.md#phase-0--analisi-e-mapping-) |
| **P2** | CardType + SequenceEngine (C++) | ✅ | [PLAN.md § Phase 2](PLAN.md#phase-2--cardtype--sequenceengine-c) |
| **P3** | FormationEngine + ScompaginamentoResolver (C++) | ✅ | [PLAN.md § Phase 3](PLAN.md#phase-3--formationengine--scompaginamentoresolver-c) |
| **P4** | BehaviorCardResolver + MonsterReactionSystem (C++) | ✅ | [PLAN.md § Phase 4](PLAN.md#phase-4--behaviorcardresolver--monsterreactionsystem-c) |
| **P8** | 4 Widget PySide6 generici | ✅ | [PLAN.md § Phase 8](PLAN.md#phase-8--widget-pyside6-generici-f5) |

### Fasi in corso 🚀

| Fase | Componente | Priorità | Blocchi | Riferimento |
|------|-----------|----------|---------|-------------|
| **P6** | EldhomEngine + RuleAdapter (C++) | P0 | **Nessuno** | [PLAN.md § Phase 6](PLAN.md#phase-6--eldhomengine--eldhomruleadapter-c--next) |
| **P1** | Data Layer JSON (carte, regole, missioni) | P1 | Nessuno | [PLAN.md § Phase 1](PLAN.md#phase-1--data-layer-json-card-definitions) |

### Fasi future ⏳

| Fase | Componente | Priorità | Dipende da |
|------|-----------|----------|-----------|
| **P5** | MissionEventSystem (C++) | P2 | P6 |
| **P7** | Mock Engine GUI (Python) | P3 | P6 + P1 |
| **P9** | Minor gmXxx extensions | P4 | – |
| **P10** | Integration Test | P5 | P6–P8 |

Dettagli complete: 📄 [PLAN.md](PLAN.md)

---

## 5. Regole base essenziali

### 5.1 Timeline continua (non Round)

- **Ordine attori:** Chi è più indietro sulla Linea Temporale agisce per primo.
- **Parità:** Se più attori sono alla stessa posizione, risolvere priorità: PG → PNG alleati → Mostri → Boss.
- **Avanzamento:** Dopo ogni azione/carta giocata, il segnalino dell'attore avanza di ⌛ (costo in tempo).
- **Nessun pagamento cumulativo:** Ogni costo è pagato subito.

Dettagli: 📄 [regole_base_eldhom_ai_spec.md § 1–2](regole_base_eldhom_ai_spec.md#1-struttura-generale-del-gioco)

### 5.2 Turno del PG

```
Nel turno un PG sceglie ESATTAMENTE UNO:
  A. eseguire 1 Azione Semplice  (Movimento / Attacco / Interazione / Recupero)
  OPPURE
  B. giocare 1 Carta Azione dalla Mano
```

Tipi di Carta Azione:
- **Singola** — giocata, risolta, scartata. Turno finisce.
- **Inizio Sequenza** — apre una sequenza. Seguito da Continuo o Fine, o fermarsi.
- **Continuo Sequenza** — solo dentro sequenza aperta. Seguito da Continuo o Fine, o fermarsi.
- **Fine Sequenza** — chiude sequenza. Turno finisce.
- **Istantanea** — giocata fuori turno se trigger soddisfatto. Valida sempre.

Dettagli: 📄 [regole_base_eldhom_ai_spec.md § 3](regole_base_eldhom_ai_spec.md#3-turno-del-pg)

### 5.3 Formazioni: Prima Linea / Retroguardia

**Regola fondamentale:** Per ogni fazione in ogni locazione:
```
Retroguardia ≤ Prima Linea
```

**Bersagliamento base:**
- Solo Prima Linea può essere bersagliata normalmente.
- Retroguardia bersagliabile solo se carta/arma/mostro lo permette esplicitamente.

**Attacchi:**
- **Mischia:** stesso luogo, PL attacca PL.
- **Distanza (corta):** stesso luogo, attacco a distanza interno.
- **Distanza (breve):** locazione adiacente.
- **Distanza (media):** 2 locazioni.
- **Distanza (lunga):** 3+ locazioni (solo se indicato).

Dettagli: 📄 [regole_base_eldhom_ai_spec.md § 1.4–1.8](regole_base_eldhom_ai_spec.md#14-regola-generale--prima-linea-e-retroguardia)

### 5.4 Azioni Semplici

Sempre disponibili (salvo effetti contrari). Costi in ⌛:

| Azione | Costo | Effetto |
|--------|-------|--------|
| **Movimento Semplice** | 1⌛ | Muovi fino a 2 locazioni |
| **Attacco Semplice** | 2⌛ | Infliggi 1❌ a bersaglio valido |
| **Interazione Semplice** | 3⌛ | Interagisci elemento scena |
| **Recupero Semplice** | 3⌛ | Recupera 1 PV + gestione mano |

Dettagli: 📄 [regole_base_eldhom_ai_spec.md § 5](regole_base_eldhom_ai_spec.md#5-azioni-semplici)

### 5.5 Effetti (Status positivi e negativi)

**Negativi:** Vista Offuscata, Rallentato, Avvelenato, Immobilizzato, Disarmato, Sanguinante, Svenuto, Maledetto.

**Positivi:** Vista Acuita, Concentrato, Energizzato, Invisibile, Resistente, Benedetto.

Dettagli: 📄 [regole_base_eldhom_ai_spec.md § 0.3](regole_base_eldhom_ai_spec.md#03-effetti)

---

## 6. Dati di gioco — Carte, Etnie, Effetti

### 6.1 Carte Base (15 universali)

Ogni PG inizia con 15 carte base universali (origine: Base):

| Carte | Descrizione | Riferimento |
|-------|-----------|-------------|
| Passo Cauto, Scatto Breve, Assestarsi | Movimento + schieramento | [carte_base_tecniche_arma_comportamento_mostri.md § 2](carte_base_tecniche_arma_comportamento_mostri.md#2-carte-base-per-tutti-i-giocatori) |
| Colpo Secco, Fendente Pesante, Spinta di Corpo | Attacco e varianti | [carte_base_tecniche_arma_comportamento_mostri.md § 2](carte_base_tecniche_arma_comportamento_mostri.md#2-carte-base-per-tutti-i-giocatori) |
| (10 altre carte di interazione, recupero, supporto) | Azioni specializzate | [carte_base_tecniche_arma_comportamento_mostri.md § 2](carte_base_tecniche_arma_comportamento_mostri.md#2-carte-base-per-tutti-i-giocatori) |

### 6.2 Carte Etnia (4 etnie × ~3 carte)

Ogni PG appartenente a un'etnia accede a carte etnia specifiche (più deboli di Affiliazione, più forti di Base).

| Etnia | Carte di esempio | Tema | Riferimento |
|-------|-----------------|------|-------------|
| **Thael** | Mani da Cava, Attrezzo Giusto, Schiena Spezzata | Concretezza, fatica, materiali, resistenza | [carte_etnia_eldhom.md § 5](carte_etnia_eldhom.md#5-thael) |
| **Velyr** | Parola Giusta, Loto Bianco, Maschera Gentile | Parola, cura, protezione indiretta, influenza | [carte_etnia_eldhom.md § 6](carte_etnia_eldhom.md#6-velyr) |
| **Khar** | Respiro dell'Oasi, Acqua Nascosta, Memoria del Sale | Deserto, oasi, risorse, sopravvivenza | [carte_etnia_eldhom.md § 7](carte_etnia_eldhom.md#7-khar) |
| **Erranti** | Strada di Traverso, Debito di Carovana, Segno sul Sentiero | Mobilità, scambio, manipolazione mazzo | [carte_etnia_eldhom.md § 4](carte_etnia_eldhom.md#4-etnie-incluse-in-questo-file) |

Dettagli complete: 📄 [carte_etnia_eldhom.md](carte_etnia_eldhom.md)

### 6.3 EffectType estesi per Dungeon Crawler (46 tipi totali)

gmRules supporta 41 EffectType base + 5 specifici Dungeon Crawler:

| EffectType | Semantica | Nuovo |
|-----------|-----------|-------|
| `MODIFY_RESOURCE`, `SET_ACTOR_RESOURCE` | Modifica/imposta risorsa attore | `SET_ACTOR_RESOURCE` ✨ |
| `APPLY_STATUS`, `APPLY_CHAIN_STATUS` | Applica status / status solo in sequenza attiva | `APPLY_CHAIN_STATUS` ✨ |
| `CHAIN_DRAW` | Pesca N carte extra durante sequenza attiva | ✨ |
| `PUSH_ACTOR` | Sposta attore in formazione (PL ↔ RG) | ✨ |
| `REVEAL_CARD_TOP` | Rivela carta in cima mazzo senza pescare | ✨ |
| (41 altri: danno, cura, tag, status base, ecc.) | Vedi tabella completa in gmRules_API.md | – |

Dettagli: 📄 [../../gmRules/gmRules_API.md](../../gmRules/gmRules_API.md#effecttype---tabella-46-tipi)

### 6.4 Regole e schema JSON

Le regole sono caricate da JSON; ciascuna carta/azione è parametrizzata tramite GRS / JSON senza dipendenze esterne:

- **Schema:** `RuleId` → `RuleDefinition` → `EffectSpec[]` → `EffectResolver` applica
- **File dati:** `GAME/Eldhom/data/*.json` (carte base, etnie, affiliazioni, missioni)
- **Caricamento:** `gmRulesEngine::load_rules_json()` — supporta accumulo (add-on runtime)

Dettagli: 📄 [../../gmRules/PLAN.md](../../gmRules/PLAN.md) | 📄 [../../tools/GRS_MANUAL.md](../../tools/GRS_MANUAL.md)

---

## 7. Decisioni architetturali vincolanti

### D1 — Timeline continua (nessun Round)
Eldhôm usa `TimelineFlowController` di gmFlow, non round turn-based classici.  
🔗 [RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md § 1](../altro/RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md#1-gmflow--decisioni-chiave-confermate)

### D2 — gmRules separa attivazione e esecuzione
`RuleGroupRegistry` risponde al "cosa è attivo"; `RuleBook` + `EffectResolver` eseguono.  
🔗 [RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md § 2](../altro/RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md#2-gmrules--grs--decisioni-chiave-confermate)

### D3 — Regole add-on runtime supportate
Le regole possono essere caricate, rimosse, sostituite a runtime senza reset motore.  
🔗 [RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md § 3](../altro/RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md#3-capacit%C3%A0-di-creare-regole-add-on-a-runtime)

### D4 — Core C++ ha autorità assoluta
GUI Python è client event-driven che visualizza, non prende decisioni.  
🔗 [STORIA_integrazione_librerie_DungeonCrawler_generico.md § 1](../altro/STORIA_integrazione_librerie_DungeonCrawler_generico.md#1-principio-architetturale-cardine)

### D5 — Estensioni alle gmXxx solo additive e retrocompatibili
Nessuna rottura di API o comportamento.  
🔗 [RIASSUNTO_DECISIONI_2026-06.md § Decision Log](../altro/RIASSUNTO_DECISIONI_2026-06.md#e--decision-log-per-i-prossimi-thread)

### D6 — FlowPhase è un IPhase normale
Gerarchia di sessioni trasparente al controller padre.  
🔗 [RIASSUNTO_DECISIONI_2026-06.md § A1 — gmFlow](../altro/RIASSUNTO_DECISIONI_2026-06.md#a1--gmflow--completato-phase-515)

### D7 — ActionQueue è globale per sessione
Azioni di livelli interni (FlowPhase) entrano nella stessa coda.  
🔗 [RIASSUNTO_DECISIONI_2026-06.md § A1 — gmFlow Decisioni di design](../altro/RIASSUNTO_DECISIONI_2026-06.md#decisioni-di-design-vincolanti-da-non-modificare)

---

## 8. Prossimi passi prioritari

### Immediato (Settimana corrente)

1. **P6 — EldhomEngine + EldhomRuleAdapter** (C++)
   - Implementare `EldhomEngine.hpp/.cpp` — orchestratore principale
   - Implementare `EldhomRuleAdapter.hpp/.cpp` — bridge gmRules ↔ stato gioco
   - Collegare `TimelineFlowController` + `SequenceEngine` + `FormationEngine` + `BehaviorCardResolver`
   - Nessun blocco; tutte le dipendenze (F1–F5) sono pronte.

2. **P1 — Data Layer JSON** (in parallelo)
   - Definire schema JSON esteso per carte Eldhôm (card_type, card_origin, icons, trigger)
   - `cards_base.json`, `cards_etnia_*.json`, `cards_affiliazione_*.json`
   - `rules_base.json`, `rules_cards.json`, `rules_monster_base.json`
   - `mission_01.json` — prima missione test

### Medio termine (settimana + 1)

3. **P5 — MissionEventSystem** (C++)
   - Implementare trigger su soglie timeline e locazioni
   - Caricare missioni da JSON
   - Collegare a gmDispatch per emettere eventi vittoria/sconfitta

4. **P7 — Mock Engine GUI** (Python)
   - Parallelo a P6: sviluppare GUI su motore mock
   - Integrare mod gmGui (TimelineWidget, FormationWidget, etc.)
   - Validare flusso Core ↔ GUI tramite ZMQ

### Riferimenti

- 📄 [PLAN.md § Prossimi lavori C++](PLAN.md#fasi-di-implementazione) per timeline dettagliata
- 📄 [RIASSUNTO_DECISIONI_2026-06.md § F — Prossimi passi](../altro/RIASSUNTO_DECISIONI_2026-06.md#f--prossimi-passi-prioritari-ordine-suggerito)

---

## 9. Riferimenti file dettagliati

### File storici (contesto decisionale)

| File | Contenuto | Uso |
|------|-----------|-----|
| [../altro/RIASSUNTO_DECISIONI_2026-06.md](../altro/RIASSUNTO_DECISIONI_2026-06.md) | Stato librerie C++, piano integrazione, decision log | Contesto generale aggiornato al 2026-06-28 |
| [../altro/RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md](../altro/RIASSUNTO_STORICO_gmFlow_gmRules_GRS_Eldhom.md) | Decisioni chiave gmFlow/gmRules, capacità runtime, implicazioni Eldhom | Architettura decisionale fondamentale |
| [../altro/STORIA_decisioni_gmFlow_gmRules_GRS_DungeonCrawler.md](../altro/STORIA_decisioni_gmFlow_gmRules_GRS_DungeonCrawler.md) | GRS Tool, RuleBook, EffectType esteso, Sandbox mock engine | Dettagli tecnici regole e parser |
| [../altro/STORIA_integrazione_librerie_DungeonCrawler_generico.md](../altro/STORIA_integrazione_librerie_DungeonCrawler_generico.md) | Integrazione librerie gmXxx, Python GUI pattern, bug risolti | Pattern di integrazione, best practices |

### File regolamento (gameplay)

| File | Contenuto | Uso |
|------|-----------|-----|
| [regole_base_eldhom_ai_spec.md](regole_base_eldhom_ai_spec.md) | Timeline continua, turni, azioni semplici, scheda PG, effetti | Fonte autorevole regolamento gameplay |
| [carte_base_tecniche_arma_comportamento_mostri.md](carte_base_tecniche_arma_comportamento_mostri.md) | 15 carte base universali, tecniche d'arma, carte comportamento | Definizione carte e meccaniche base |
| [carte_etnia_eldhom.md](carte_etnia_eldhom.md) | 4 etnie (Thael, Velyr, Khar, Erranti) con carte identità | Identità culturale e carte etnia |

### File sviluppo (piano, architettura)

| File | Contenuto | Uso |
|------|-----------|-----|
| [PLAN.md](PLAN.md) | Fasi sviluppo P0–P10, architettura, file structure, dipendenze | Piano operativo, file structure, timeline |

### Librerie gmXxx — API e documentazione

| Libreria | File API | Uso |
|----------|----------|-----|
| **gmAlea** | [../../gmAlea/gmAlea_API.md](../../gmAlea/gmAlea_API.md) | CardType, SequenceEngine, GmCompDeck (6 zone) |
| **gmFlow** | [../../gmFlow/gmFlow_API.md](../../gmFlow/gmFlow_API.md) | TimelineFlowController, TimelineMilestoneSystem |
| **gmActor** | [../../gmActor/gmActor_API.md](../../gmActor/gmActor_API.md) | HeroState, MonsterGroupState, FormationValidator, BehaviorCardProcessor |
| **gmRules** | [../../gmRules/gmRules_API.md](../../gmRules/gmRules_API.md) | RuleBook, EffectResolver, 46 EffectType, RuleGroupRegistry |
| **gmGui** | [../../pyLib/gmGui/gmGui_API.md](../../pyLib/gmGui/gmGui_API.md) | TimelineWidget, FormationWidget, SequenceStateWidget, BehaviorCardWidget |
| **Game-Lib overview** | [../../Game-Lib_readme.md](../../Game-Lib_readme.md) | Navigazione generale librerie, status CMake |

### Strumenti e utility

| Risorsa | File | Uso |
|---------|------|-----|
| **GRS Manual** | [../../tools/GRS_MANUAL.md](../../tools/GRS_MANUAL.md) | Linguaggio GRS, parser, validazione |
| **gmFlow FlowPhase Plan** | [../../gmFlow/FlowPhase_PLAN.md](../../gmFlow/FlowPhase_PLAN.md) | Documentazione FlowPhase per gerarchia sessioni |
| **gmRules Integration Plan** | [../../gmRules/specs/grs-integration-implementation-plan.md](../../gmRules/specs/grs-integration-implementation-plan.md) | Piano integrazione librerie gmXxx con gmRules |

---

## 10. Come usare questo documento

### Per sviluppatori C++

1. Leggere **§ 2 Architettura** per capire dove il codice si inserisce.
2. Leggere **§ 3 Librerie gmXxx** per individuare quale libreria usare.
3. Consultare gli **API reference** in § 9 per dettagli di interfaccia.
4. Consultare **§ 7 Decisioni architetturali** per capire i vincoli.
5. Leggere **§ 4 Stato di sviluppo** per sapere quale fase affrontare.

**Prossima lettura:** 📄 [PLAN.md § Phase 6](PLAN.md#phase-6--eldhomengine--eldhomruleadapter-c--next)

### Per sviluppatori Python / GUI

1. Leggere **§ 2 Architettura** per capire l'integrazione GUI ↔ Core.
2. Leggere **§ 3 Librerie gmXxx** sezione gmGui (F5).
3. Consultare [gmGui_API.md](../../pyLib/gmGui/gmGui_API.md) per i widget disponibili.
4. Consultare **§ 7 Decisioni architetturali** D4 (Core comanda, GUI visualizza).
5. Consultare **§ 6 Dati di gioco** per capire cosa visualizzare.

**Prossima lettura:** 📄 [PLAN.md § Phase 7](PLAN.md#phase-7--mock-engine-gui-python--pending) | 📄 [../../STORIA_integrazione_librerie_DungeonCrawler_generico.md § 3](../altro/STORIA_integrazione_librerie_DungeonCrawler_generico.md#3-integrazione-librerie-python-gui-gmgui)

### Per game designers / regolamenti

1. Leggere **§ 1 Identità del gioco** per comprendere l'essenza.
2. Consultare **§ 5 Regole base essenziali** per meccaniche fondamentali.
3. Consultare **§ 6 Dati di gioco** per carte e effetti.
4. Consultare 📄 [regole_base_eldhom_ai_spec.md](regole_base_eldhom_ai_spec.md) per il regolamento completo.

---

**Ultimo aggiornamento:** 28 giugno 2026  
**Manutentore:** Copilot + Team game_lib
