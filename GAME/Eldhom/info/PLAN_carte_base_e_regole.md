# Piano — Allineamento Regole e Carte Base di Eldhôm

Versione: 1.0
Data: 2026-07-14
Riferimento: `carte_base_tecniche_arma_comportamento_mostri.md` (§1 Regole comuni, §2 Carte Base)

---

## 1. Scopo

Allineare l'implementazione C++ di `GAME/Eldhom/CoreEngine` alle regole comuni (§1)
e alle 15 Carte Base (§2) del documento di riferimento, seguendo le priorità e le
semplificazioni concordate con l'utente il 2026-07-14.

Le Carte Tecniche d'Arma (§3), il Mazzetto Precisione (§4) e le Carte
Comportamento Mostri (§5-7) sono **esplicitamente fuori scope** per questo piano.

---

## 2. Stato attuale — Regole comuni (§1)

| Regola | Stato | Dove |
|---|---|---|
| §1.1 Movimento/Attacco/Interazione Semplice | Implementato | `EldhomTypes.hpp` (COST_*), `EldhomRuleAdapter` |
| §1.1 Recupero Semplice (scarta/pesca, ricarica) | Parziale | `apply_simple_recover` cura solo PV |
| §1.2 Tipi di carta (5 tipi) | Implementato | `gmAlea::CardType` |
| §1.4 Prima Linea/Retroguardia (RG ≤ PL) | Implementato | `EldhomFormationAdapter` |
| §1.5/§1.6 Bersagli solo PL (Proiezione), Mischia PL→PL | Implementato | `TargetingFilter` |
| §1.7 Attacco a distanza solo da RG | Mancante | Nessun controllo posizione attaccante |
| §1.8 Distanze (mischia/corta/breve/media/lunga) | Mancante | Targeting cerca solo nella location dell'attaccante |

---

## 3. Stato attuale — 15 Carte Base (§2)

| # | Carta | Stato | Gap |
|---|---|---|---|
| 1 | Passo Cauto | Parziale | Path non deve attraversare nemici; terreno ignorato (nessun sistema terreno, non bloccante) |
| 2 | Scatto Breve | Parziale | Come sopra |
| 3 | Assestarsi | Mancante | Trigger istantanea "nemico entra in location/adiacente" non esiste |
| 4 | Colpo Secco | Parziale | Scelta A/B → solo parte A (pesca+scarta) |
| 5 | Fendente Pesante | Mancante | Requisito posizionale attaccante (PL) |
| 6 | Spinta di Corpo | Mancante | Requisito PL + Scompaginamento mirato |
| 7 | Colpo d'Apertura | **Pronto** | Solo dati catalogo |
| 8 | Passo e Lama | **Pronto** | Solo dati catalogo |
| 9 | Secondo Colpo | Rimandata | Serve tracking bersaglio precedente in sequenza |
| 10 | Pressione Continua | Parziale | Scelta A/B → solo parte A (attacco) |
| 11 | Colpo di Chiusura | Rimandata | Serve tracking danno cumulativo per bersaglio in sequenza |
| 12 | Lancio di Fortuna | Mancante | Serve sistema Range |
| 13 | Uso Arco/Balestra | **Esclusa** | Richiede arma equipaggiata (fuori scope) |
| 14 | Mano Ferma | **Pronto** | Solo dati catalogo |
| 15 | Riprendere Fiato | Parziale | RECOVER non scarta/pesca oggi |

---

## 4. Decisioni utente (scope bloccato)

1. **Ordine**: prima le 7 carte facili/parziali (1, 2, 4, 7, 8, 14, 15).
2. **Scelte A/B nelle carte**: implementare **solo l'opzione A** (prima elencata nel
   documento), nessuna UI di scelta per ora.
3. **Sistema Range**: **niente nomi** (corta/breve/media/lunga). Un solo concetto
   generico **Range numerico** via BFS sull'adiacenza:
   - Range 0 = mischia (stessa Locazione)
   - Range 1 = Locazione adiacente
   - Range 2 = adiacente all'adiacente
   - ... fino a Range 10
4. **Arma equipaggiata / Mazzetto Precisione**: **esclusi**. Carta 13 e tutte le
   Tecniche d'Arma (§3) rimangono fuori scope finché non richiesto esplicitamente.
5. **Stati temporanei** (es. "non può ritirarsi fino a...", buff Mirare):
   **semplificare** — solo danno/effetto base, niente stato applicato.
6. **Reazione mostro → PG**: **serve subito**, per sbloccare Assestarsi (trigger
   "nemico entra in location/adiacente") e future carte difensive.
7. Carte 9 e 11 (tracking bersaglio/danno in sequenza): **rimandate**, gap non
   ancora discusso in dettaglio.
8. Carte 5 e 6 (requisito Prima Linea): incluse in Fase 3. Per la 6, lo
   Scompaginamento si risolve **automaticamente** (meccanismo già esistente),
   non a scelta del giocatore.

---

## 5. Piano tecnico dettagliato

### FASE 0 — Fondamenta motore

| ID | Task | File principali | Dettaglio tecnico |
|---|---|---|---|
| F0.1 | Requisito posizionale attaccante | `CardData.hpp`, `EldhomEngine.cpp`, `MissionLoader.cpp`, `EldhomTypes.hpp` | Nuovo campo `EldhomCard::requires_frontline` (bool). Verificato in `play_card()` prima di applicare effetti → nuovo `ActionResultCode::ERR_POSITION_REQUIRED` se non soddisfatto. Letto da JSON in `parse_hero_card`. |
| F0.2 | Sistema Range generico (BFS 0-10) | `EldhomEffect` (nuovo campo `range`), `EldhomRuleAdapter` (nuovo metodo `nearest_target_in_range`), `TargetingFilter` (riuso `nearest_target`/`has_valid_target` per location) | BFS su `_adjacency` già presente in `EldhomRuleAdapter`, esplora fino a `range` salti applicando le stesse regole di Proiezione per ogni location visitata (si ferma alla prima location con un bersaglio valido, più vicina prima). |
| F0.3 | Movimento: vieta path su location con nemici | `EldhomRuleAdapter::apply_card_move` | Nuovo parametro opzionale (es. `bool avoid_enemy_locations`) sul BFS di movimento: le location intermedie (non la destinazione finale) con nemici della fazione opposta sono escluse dall'esplorazione. |
| F0.4 | RECOVER: scarta N/pesca N | `EldhomEngine::do_simple_action` (case RECOVER), gestione mano esistente (`_hand_states`, `draw_n_cards`) | N=1 normale, N=2 se `area_position == BACKLINE`. Richiede lista di card_id da scartare come parametro dell'azione (l'azione RECOVER dovrà accettare un parametro opzionale `discard_ids`). |
| F0.5 | Finestra reazione/istantanea mostro→PG | `EldhomEngine.hpp/.cpp` (`PendingAttack` o nuova struct dedicata), nuovo trigger in `EldhomTypes.hpp` | Nuovo evento trigger (es. `EVT_ENEMY_APPROACH`) emesso quando un mostro entra in una Locazione con PG o adiacente durante il proprio movimento. Riusa `eligible_instants()`/`play_instants()` con generalizzazione minima per il caso "nessun danno in sospeso, solo finestra istantanea". Necessita lettura approfondita di `play_instants()`/`resolve_reaction()` prima dell'implementazione per non introdurre regressioni sul flusso PG→mostro esistente. |

### FASE 1 — Le 7 carte prioritarie

| # | Carta | Dipende da |
|---|---|---|
| 7 | Colpo d'Apertura | — (solo dati) |
| 8 | Passo e Lama | — (solo dati) |
| 14 | Mano Ferma | — (solo dati) |
| 1 | Passo Cauto | F0.3 |
| 2 | Scatto Breve | F0.3 |
| 4 | Colpo Secco | Solo parte A (pesca 1 scarta 1 se entrambi in PL) |
| 15 | Riprendere Fiato | F0.4 |

### FASE 2 — Assestarsi + Pressione Continua

| # | Carta | Dipende da |
|---|---|---|
| 3 | Assestarsi | F0.5 |
| 10 | Pressione Continua | Solo parte A (attacco 1❌) |

### FASE 3 — Requisito Prima Linea

| # | Carta | Dipende da |
|---|---|---|
| 5 | Fendente Pesante | F0.1 |
| 6 | Spinta di Corpo | F0.1 (Scompaginamento automatico) |

### FASE 4 — Range

| # | Carta | Dipende da |
|---|---|---|
| 12 | Lancio di Fortuna | F0.2 |

### Escluse/Rimandate

- **13. Uso Arco/Balestra** — richiede arma equipaggiata (fuori scope)
- **9. Secondo Colpo**, **11. Colpo di Chiusura** — richiedono tracking
  bersaglio/danno precedente nella Sequenza (gap non ancora discusso)

---

## 6. Verifica

Per ogni fase: build C++ (`cmake --build . --target eldhom_engine --config Debug`),
esecuzione test esistenti (`test_eldhom_mission01.exe`, baseline 27 PASS / 10 FAIL
preesistenti — vedi `/memories/repo/eldhom-zone-door-mechanic.md`), aggiunta di
nuovi test mirati per ogni nuova meccanica introdotta.
