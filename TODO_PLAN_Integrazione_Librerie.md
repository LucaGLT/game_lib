# Integrazione fra Librerie

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmRules)

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmActor)

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmAlea)

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmMap)

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmRules/gmRules_API.md#1-1), lines 1 to 50

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmActor/PLAN.md#1-1), lines 1 to 50

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmMap/PLAN.md#1-1), lines 1 to 50

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmAlea/PLAN.md#1-1), lines 1 to 50

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmRules/specs/grs-spec.md#1-1), lines 1 to 100

Searched for text `Capitolo|Chapter|phase|events` (`**/gmActor/**/*.md`), 5 results

Searched for text `Capitolo|Chapter|phase|events|lifecycle` (`**/gmAlea/**/*.md`), 5 results

Searched for text `Capitolo|Chapter|phase|events` (`**/gmMap/**/*.md`), 5 results

Searched for text `gmFlow|lifecycle|TURN_STARTED|TURN_COMPLETED|ROUND_STARTED|evento` (`**/gmRules/**/*.md`), 10 results

Searched for text `ACTOR_SPAWNED|ACTOR_HP|ACTOR_DIED|RESOURCE|EQUIP|evento` (`**/gmActor/**/*.md`), 10 results

Searched for text `TOKEN_PRE|DICE_PRE|PRE_DRAW|PRE_ROLL|evento` (`**/gmAlea/**/*.md`), no results

Searched for text `ACTOR_MOVED|PATH_BLOCKED|LOS_CHANGED|evento` (`**/gmMap/**/*.md`), no results

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmRules/specs/grs-integration-implementation-plan.md#1-1), lines 1 to 150

Read [](file:///c%3A/_GLT_/Qt%20Prj/game_lib/gmRules/specs/grs-integration-implementation-plan.md#150-150), lines 150 to 282

Creo un rapporto di verifica dettagliato:

## ✅ **VERIFICA — Capitoli Futuri PENDENTI**

Confermo che **NON** è stato implementato nulla dei seguenti. Ecco cosa specificamente serve:

---

### **📋 CAPITOLO 2 — gmFlow (P1)** 🔴 NOT STARTED

**Stato:** Completato solo la struttura TimelineFlowController e SequentialFlowController.  
**Manca:** Integrazione eventi → gmRules

**Cosa fare esattamente:**

1. **Emissione lifecycle eventi** verso gmRules da gmFlow:
   - `TURN_STARTED` — all'inizio di ogni turno
   - `TURN_COMPLETED` — al completamento turno
   - `ROUND_STARTED` — all'inizio round
   - `ROUND_COMPLETED` — al completamento round
   - `ACTION_WINDOW_OPENED` — quando ActionWindow si apre
   - `ACTION_WINDOW_CLOSED` — quando ActionWindow si chiude
   - `ACTION_SUBMITTED` — quando un'azione è submessa
   - `ACTION_COMPLETED` — quando un'azione completa
   - `ACTION_SKIPPED` — quando un'azione è skippata
   - `PHASE_CHANGED` — quando fase transisce
   - `GAME_STATE_CHANGED` — quando stato game cambia

2. **Pipeline invocazione regole:**
   - Pre-check trigger (`ACTION_SUBMITTED`) → valida se azione è consentita
   - Execute rule in gmRules
   - Post-check trigger (`ACTION_COMPLETED`) → applica effetti

3. **Deliverable:**
   - Adapter di orchestrazione gmFlow ↔ gmRules
   - Test integrazione su turno completo + round completo

---

### **📋 CAPITOLO 4 — gmActor (P3)** 🔴 NOT STARTED

**Stato:** Implementate strutture actor base (ActorState, stats, inventory, equipment).  
**Manca:** Eventi estesi e effetti actor per gmRules

**Cosa fare esattamente:**

1. **Eventi actor estesi** da emettere:
   - `ACTOR_SPAWNED` — actor crea
   - `ACTOR_DESPAWNED` — actor scompare
   - `ACTOR_HP_CHANGED` — HP modifica
   - `ACTOR_DIED` — HP → 0
   - `ACTOR_REVIVED` — ritorno in vita
   - `RESOURCE_CHANGED` — qualsiasi risorsa cambia
   - `ITEM_EQUIPPED` — item equipaggiato
   - `ITEM_UNEQUIPPED` — item rimosso

2. **Effetti actor supportati da gmRules:**
   - `MODIFY_RESOURCE` — aggiungi/sottrai risorsa
   - `SET_RESOURCE_MAX` — modifica max risorsa
   - `EQUIP_ITEM` — equip item
   - `UNEQUIP_ITEM` — unequip item
   - `SPAWN_ACTOR` — spawn nuovo actor
   - `DESPAWN_ACTOR` — rimuovi actor
   - `REVIVE_ACTOR` — revive actor morto
   - `CHANGE_TEAM` — cambia affiliazione

3. **Deliverable:**
   - Adapter gmActor per condizioni/effetti
   - Test integrazione: danno, morte, revive, risorse

---

### **📋 CAPITOLO 5 — gmAlea (P4)** 🔴 NOT STARTED

**Stato:** Implementate GmDeck, GmCompDeck, GmDice, StdDice (strutture base).  
**Manca:** Pre/post resolve events e effetti extend

**Cosa fare esattamente:**

1. **Eventi pre/post resolve:**
   - `TOKEN_PRE_DRAW` — prima che token sia estratto
   - `TOKEN_DRAWN` — dopo che token è estratto
   - `DICE_PRE_ROLL` — prima che dado sia tirato
   - `DICE_ROLLED` — dopo che dado è tirato
   - `ALEA_RESOLVED` — intera sequenza randomica conclusa

2. **Effetti alea estesi:**
   - `SHUFFLE_ZONE` — rimescola zona specifica
   - `LOOK_TOP_CARD` — ispeziona top card
   - `LOOK_BOTTOM_CARD` — ispeziona bottom card
   - `SELECT_SPECIFIC_CARD` — seleziona card specifica da zona
   - `DISCARD_RANDOM` — scarta card random da hand
   - `PLACE_ON_TOP` — posiziona card sul top
   - `PLACE_ON_BOTTOM` — posiziona card sul bottom
   - `ROLL_DICE` — tira dado

3. **Deliverable:**
   - Adapter gmAlea per draw/roll/shuffle/look/select
   - Test integrazione: curse-draw, critical-roll scenari
   - Determinismo con seed controllato

---

### **📋 CAPITOLO 6 — gmMap (P5)** 🔴 NOT STARTED

**Stato:** Implementate Location/Tile management, topologia base.  
**Manca:** Eventi avanzati e effetti spatial

**Cosa fare esattamente:**

1. **Eventi map estesi:**
   - `ACTOR_MOVED` — actor ha cambiato location
   - `ACTOR_APPROACHED` — actor si è avvicinato
   - `ACTOR_LEFT_LOCATION` — actor ha lasciato location
   - `LOCATION_STATE_CHANGED` — stato location cambia
   - `PATH_BLOCKED` — percorso diventa impercorribile
   - `LOS_CHANGED` — line-of-sight modificato

2. **Effetti map estesi:**
   - `SET_LOCATION_PASSABLE` — rendi location passabile/impassabile
   - `ADD_LOCATION_TAG` — aggiungi tag a location
   - `REMOVE_LOCATION_TAG` — rimuovi tag da location
   - `SET_LOCATION_OWNER` — assegna owner location
   - `CREATE_BARRIER` — crea barriera tra locations
   - `REMOVE_BARRIER` — rimuovi barriera
   - `SPAWN_INTERACTABLE` — spawa oggetto interattivo
   - `DESPAWN_INTERACTABLE` — rimuovi oggetto

3. **Condizioni map avanzate:**
   - `LOCATION_IS_REACHABLE` — location raggiungibile?
   - `LOCATION_HAS_LOS` — LOS da → a?
   - `MOVE_COST_AT_MOST` — costo movimento ≤ N?

4. **Deliverable:**
   - Adapter gmMap per condizioni/effetti/eventi
   - Test integrazione: movimento, LOS, barriere, tag location

---

### **📊 SUMMARY TABLE**

| Capitolo | Libreria | Priority | Status | Events | Effects | Tests |
|----------|----------|----------|--------|--------|---------|-------|
| **2** | gmFlow | P1 | 🔴 Not started | 11 events | — | ✗ |
| **4** | gmActor | P3 | 🔴 Not started | 8 events | 8 effects | ✗ |
| **5** | gmAlea | P4 | 🔴 Not started | 5 events | 8 effects | ✗ |
| **6** | gmMap | P5 | 🔴 Not started | 6 events | 8 effects | ✗ |

---

**Conclusion:** ✅ **CONFERMATO** — Non è stato implementato nulla. Tutto rimane da fare, nell'ordine indicato (P1 → P5).