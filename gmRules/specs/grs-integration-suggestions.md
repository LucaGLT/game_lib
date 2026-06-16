# GRS Integration Suggestions

Suggerimenti pratici per integrare gmRules con librerie esterne o adiacenti,
quando non esiste una dipendenza diretta nel core.

---

## Obiettivo

gmRules deve restare un motore dichiarativo e standalone.
Le integrazioni applicative si implementano tramite adapter sottili,
per evitare accoppiamento tra librerie.

---

## 1. Bridge gmRules -> gmDispatch

### Problema

gmRules espone eventi logici (`EMIT_EVENT`, `MANUAL_EFFECT`) ma non deve
conoscere i dettagli di envelope/canali/router di gmDispatch.

### Soluzione consigliata

Creare un adapter di pubblicazione eventi lato applicazione:

- Input: evento logico prodotto da gmRules (nome, source_id, target_id,
  metadati minimi)
- Output: envelope gmDispatch pubblicato sul topic/canale corretto

### Convenzioni utili

- Namespace evento: `domain.context.event_name`
  - esempio: `dungeon.turn.action_blocked`
- Mappare severita/priorita in header envelope, non nel nome evento.
- Conservare payload minimale e serializzabile (ID, amount, status_id, timestamp).

### Pseudoflusso

1. `gmRulesEngine` completa una rule.
2. Effetto `EMIT_EVENT(...)` produce un record evento interno.
3. `GmRulesDispatchBridge` converte in envelope gmDispatch.
4. Dispatch su canale (`game.events`, `combat.events`, ecc.).

---

## 2. Orchestrazione gmFlow <-> gmRules

### Problema

gmRules non governa il loop di gioco: serve un orchestratore di fase/turno.

### Soluzione consigliata

Usare gmFlow come orchestratore e gmRules come evaluator pure-function-like.

### Contratto minimo

- gmFlow fornisce:
  - action request
  - runtime bindings (`input.xxx`)
  - contesto turno/fase
- gmRules restituisce:
  - outcome (success/failure)
  - effetti applicati
  - eventi emessi
  - motivi di blocco

### Sequenza raccomandata

1. gmFlow riceve comando giocatore/AI.
2. gmFlow pubblica `ACTION_SUBMITTED`.
3. gmFlow invoca gmRules (rule o trigger path).
4. gmFlow applica outcome al proprio state machine.
5. gmFlow pubblica `ACTION_COMPLETED`.
6. gmFlow valuta transizioni di fase.

### Eventi gmFlow estesi

gmRules può reagire a questi eventi di lifecycle e fase:

#### Fase turno

| Evento | Quando scatta | Disponibili |
|---|---|---|
| `TURN_STARTED` | Inizio turno di un attore | `event.actor_id`, `event.turn_number` |
| `TURN_COMPLETED` | Fine turno di un attore | `event.actor_id`, `event.turn_number` |
| `ROUND_STARTED` | Inizio round completo (tutti gli attori hanno giocato) | `event.round_number` |
| `ROUND_COMPLETED` | Fine round completo | `event.round_number` |

#### Finestra di azione

| Evento | Quando scatta | Disponibili |
|---|---|---|
| `ACTION_WINDOW_OPENED` | Finestra azione aperta (es. "scegli un'azione") | `event.actor_id`, `event.window_type` |
| `ACTION_WINDOW_CLOSED` | Finestra azione chiusa (es. timeout, decisione) | `event.actor_id`, `event.reason` |
| `ACTION_SKIPPED` | Attore passa senza agire | `event.actor_id`, `event.turn_number` |

#### Transizione fase

| Evento | Quando scatta | Disponibili |
|---|---|---|
| `PHASE_CHANGED` | Cambio di fase (planning -> execution -> resolution) | `event.from_phase`, `event.to_phase` |
| `GAME_STATE_CHANGED` | Cambio stato globale (in_progress -> paused -> ended) | `event.from_state`, `event.to_state` |

### Condizioni e trigger suggeriti per gmFlow

```grs
@conditions
C_TurnStart       :: ALWAYS
C_RoundStart      :: ALWAYS
C_ActionWindowOpen :: ALWAYS
C_ActorTurnOrder  :: ACTOR_EQUALS(event.actor_id, input.current_actor)

@triggers
T_TurnBegin [priority=1000] ::
    ON_EVENT TURN_STARTED
    THEN MANUAL_EFFECT(game.turn.initialize) [optional]

T_TurnCleanup [priority=100] ::
    ON_EVENT TURN_COMPLETED
    THEN MANUAL_EFFECT(game.turn.cleanup) [optional]

T_RoundPhaseCheck [priority=50] ::
    ON_EVENT ROUND_COMPLETED
    THEN MANUAL_EFFECT(game.round.phase_eval) [optional]
```

---

## 3. Integrazione gmAlea in domini non-card-centric

### Problema

In alcuni giochi gmAlea non è centrale, ma serve comunque per risorse random.

### Soluzione consigliata

Introdurre un adapter `RuleRandomProvider` separato da gmRules core:

- pesca carta: delega a gmAlea deck API
- tiro casuale: delega a gmAlea dice API
- conversione in binding runtime (`input.roll`, `input.card_id`)

### Regola pratica

- Non inserire logica random direttamente in condizioni/effetti custom.
- Risolvere il random prima dell'invocazione regola e passare valori determinati.

### Eventi gmAlea

#### Pre-risoluzione (permette intercettazione/modifica)

| Evento | Quando scatta | Disponibili | Uso |
|---|---|---|---|
| `TOKEN_PRE_DRAW` | Prima di pescare una carta | `event.deck_id`, `event.amount` | Costi, effetti preventivi |
| `DICE_PRE_ROLL` | Prima di lanciare i dadi | `event.dice_id`, `event.sides` | Modifiche al tiro (malus/bonus) |

#### Post-risoluzione (effetti consequenziali)

| Evento | Quando scatta | Disponibili | Uso |
|---|---|---|---|
| `TOKEN_DRAWN` | Dopo pesca carta | `event.card_id`, `event.deck_id`, `event.zone` | Reazioni, trigger cascata |
| `DICE_ROLLED` | Dopo lancio dadi | `event.dice_id`, `event.result`, `event.amount` | Reazioni critiche |
| `ALEA_RESOLVED` | Dopo risoluzione random finale | `event.source_type` (CARD\|DICE), `event.result` | Log globale |

### Azioni gmAlea da esporre in GRS

Estensioni suggerite alla tabella `@effects`:

| Azione GRS | Corrispettivo gmAlea |
|---|---|
| `DRAW_CARDS(target, deck_ref, amount)` | Pesca `amount` carte da `deck_ref` |
| `MOVE_CARD_TO_ZONE(card_ref, zone_name)` | Sposta carta in zona (hand, discard, banish, play, bottom, top) |
| `SHUFFLE_ZONE(deck_ref, zone_name)` | Rimescola una zona specifica |
| `LOOK_TOP_CARD(deck_ref, amount)` | Guarda le prime N carte senza pesca |
| `LOOK_BOTTOM_CARD(deck_ref, amount)` | Guarda le ultime N carte |
| `SELECT_SPECIFIC_CARD(deck_ref, zone_name, card_id)` | Estrai una carta specifica da una zona |
| `DISCARD_RANDOM(deck_ref, zone_name, amount)` | Scarta N carte casuali da una zona |
| `PLACE_ON_TOP(card_ref, deck_ref)` | Posiziona carta in cima al mazzo |
| `PLACE_ON_BOTTOM(card_ref, deck_ref)` | Posiziona carta in fondo al mazzo |
| `ROLL_DICE(dice_id, amount, sides)` | Lancia dadi (amount × dadi da sides facce) |

### Esempio trigger per gmAlea

```grs
@triggers
T_OnCardDrawn [priority=100] ::
    ON_EVENT TOKEN_DRAWN
    IF ALWAYS
    THEN EMIT_EVENT(Target_Self, game.card.drawn) [optional]

T_OnCriticalRoll [priority=150] ::
    ON_EVENT DICE_ROLLED
    IF event.result >= 18
    THEN ADD_TAG(Target_Self, critical_success) [optional]

T_DiscardAfterDraw [priority=50] ::
    ON_EVENT TOKEN_DRAWN
    IF CARD_IN_ZONE(event.card_id, hand) AND ACTOR_HAS_TAG(event.actor_id, cursed)
    THEN MOVE_CARD_TO_ZONE(event.card_id, discard) [optional]
```

---

## 3b. Integrazione gmActor con eventi e azioni estese

### Eventi gmActor

#### Stato attore

| Evento | Quando scatta | Disponibili | Uso |
|---|---|---|---|
| `ACTOR_SPAWNED` | Attore creato/apparso | `event.actor_id`, `event.location_id` | Setup iniziale, effetti entrata |
| `ACTOR_DESPAWNED` | Attore rimosso | `event.actor_id`, `event.reason` | Cleanup, loot drop |
| `ACTOR_HP_CHANGED` | HP modificato | `event.actor_id`, `event.old_hp`, `event.new_hp`, `event.delta` | Effetti vita bassa/critica |
| `ACTOR_DIED` | HP raggiunge 0 | `event.actor_id`, `event.last_damage_source` | Morte, loot, drop reward |
| `ACTOR_REVIVED` | Attore ritorna in vita | `event.actor_id`, `event.revive_source` | Rinascita, effetti post-resurrezione |

#### Risorse e inventory

| Evento | Quando scatta | Disponibili | Uso |
|---|---|---|---|
| `RESOURCE_CHANGED` | Risorsa (mana, stamina, rage, ecc.) modificata | `event.actor_id`, `event.resource_type`, `event.delta` | Reazioni a esaurimento |
| `ITEM_EQUIPPED` | Oggetto equipaggiato | `event.actor_id`, `event.item_id`, `event.slot` | Buff, bonus applicati |
| `ITEM_UNEQUIPPED` | Oggetto tolto | `event.actor_id`, `event.item_id`, `event.slot` | Rimozione buff |

### Azioni gmActor da esporre in GRS

| Azione GRS | Corrispettivo gmActor |
|---|---|
| `MODIFY_RESOURCE(target, resource_type, delta)` | Aggiunge/sottrae risorsa (mana, stamina, ecc.) |
| `SET_RESOURCE_MAX(target, resource_type, new_max)` | Modifica massimo di una risorsa |
| `EQUIP_ITEM(target, item_id, slot)` | Equipa un oggetto |
| `UNEQUIP_ITEM(target, slot)` | Toglie un oggetto |
| `SPAWN_ACTOR(actor_id, location_id, team)` | Crea un nuovo attore |
| `DESPAWN_ACTOR(actor_id)` | Rimuove un attore |
| `REVIVE_ACTOR(target, hp_restore)` | Riporta attore in vita |
| `CHANGE_TEAM(target, new_team)` | Cambia team di un attore |

### Trigger suggeriti per gmActor

```grs
@triggers
T_OnActorDied [priority=200] ::
    ON_EVENT ACTOR_DIED
    THEN MANUAL_EFFECT(game.actor.death_check) [optional]

T_LowHealthWarning [priority=100] ::
    ON_EVENT ACTOR_HP_CHANGED
    IF ACTOR_HP_AT_OR_BELOW(event.actor_id, 3)
    THEN EMIT_EVENT(Target_Self, game.actor.critical_hp) [optional]

T_OnResourceExhausted [priority=150] ::
    ON_EVENT RESOURCE_CHANGED
    IF RESOURCE_AT_MOST(event.actor_id, 0)
    THEN ADD_TAG(Target_Self, exhausted) [optional]
```

---

## 4. Integrazione gmMap con eventi e azioni estese

### Eventi gmMap

#### Movimento e posizione

| Evento | Quando scatta | Disponibili | Uso |
|---|---|---|---|
| `ACTOR_MOVED` | Attore si sposta da location a location | `event.actor_id`, `event.from_location`, `event.to_location` | Promozioni, trappole post-movimento |
| `ACTOR_APPROACHED` | Attore entra in prossimità di altra location | `event.actor_id`, `event.nearby_location`, `event.distance` | Effetti aura, visibilità |
| `ACTOR_LEFT_LOCATION` | Attore abbandona location | `event.actor_id`, `event.location_id` | Cleanup posizionale |
| `LOCATION_STATE_CHANGED` | Proprietà location cambia (tag, passabile, ecc.) | `event.location_id`, `event.property`, `event.old_value`, `event.new_value` | Effetti terreno |

#### Topologia

| Evento | Quando scatta | Disponibili | Uso |
|---|---|---|---|
| `PATH_BLOCKED` | Un percorso diventa non percorribile | `event.from_location`, `event.to_location`, `event.blocker` | Rimozione ostacoli |
| `LOS_CHANGED` | Linea di vista tra location cambia | `event.from_location`, `event.to_location`, `event.visible` | Effetti visibilità |

### Azioni gmMap da esporre in GRS

| Azione GRS | Corrispettivo gmMap |
|---|---|
| `MOVE_ACTOR(target, destination)` | Sposta attore verso location (GRS già supporta) |
| `SET_LOCATION_PASSABLE(location_id, passable)` | Rende location passabile/bloccata |
| `ADD_LOCATION_TAG(location_id, tag)` | Aggiunge tag a location (es. `trapped`, `high_ground`) |
| `REMOVE_LOCATION_TAG(location_id, tag)` | Rimuove tag da location |
| `SET_LOCATION_OWNER(location_id, actor_id)` | Marca location come controllata da attore |
| `CREATE_BARRIER(from_location, to_location)` | Blocca passaggio tra due location |
| `REMOVE_BARRIER(from_location, to_location)` | Rimuove barriera |
| `SPAWN_INTERACTABLE(interactable_id, location_id)` | Crea oggetto interattivo |
| `DESPAWN_INTERACTABLE(interactable_id)` | Rimuove oggetto interattivo |

### Condizioni gmMap suggerite

```grs
@conditions
C_TargetAdjacent    :: LOCATION_IS_ADJACENT(input.actor_location, input.target_location)
C_TargetWithinRange :: LOCATION_DISTANCE_AT_MOST(input.actor_location, input.target_location, 3)
C_PathExists        :: LOCATION_PATH_EXISTS(input.actor_location, input.destination)
C_LocationOwned     :: LOCATION_OWNER(event.location_id, input.actor_id)
```

### Trigger suggeriti per gmMap

```grs
@triggers
T_OnLocationTagged [priority=100] ::
    ON_EVENT LOCATION_STATE_CHANGED
    IF LOCATION_HAS_TAG(event.location_id, trapped)
    THEN EMIT_EVENT(Target_Self, game.location.trap_armed) [optional]

T_OnPathBlocked [priority=200] ::
    ON_EVENT PATH_BLOCKED
    THEN MANUAL_EFFECT(game.map.path_blocked_check) [optional]

T_HighGroundBonus [priority=50] ::
    ON_EVENT ACTOR_MOVED
    IF LOCATION_HAS_TAG(event.to_location, high_ground)
    THEN ADD_TAG(Target_Self, elevated) [optional]
```

---

## 4b. Integrazione gmDispatch con pattern adapter

### Problema

gmDispatch è il bus globale: gmRules genera eventi, ma non deve dipendere
dai dettagli di routing/envelope.

### Soluzione

Implementare un adapter bridge che:

1. Intercetta ogni `EMIT_EVENT` / `MANUAL_EFFECT` prodotto da gmRules.
2. Enricchisce con metadati (timestamp, source game system, severity).
3. Pubblica come envelope gmDispatch su canale appropriato.

### Mappatura suggerita

```cpp
// Pseudo-codice
struct RuleEventBridge {
    void on_emit_event(const RuleEvent& event, const RuleContext& ctx) {
        // Risolvi actor/target reali
        auto source = ctx.resolve_actor(event.source_id);
        auto target = ctx.resolve_actor(event.target_id);
        
        // Costruisci envelope gmDispatch
        Envelope env;
        env.channel = categorize_channel(event.namespace);  // game.events, combat.events, etc.
        env.topic = event.event_name;
        env.payload = serialize_minimal(event);
        env.timestamp = clock.now();
        env.headers["rule_priority"] = event.priority;
        env.headers["source_system"] = "gmRules";
        
        // Pubblica
        dispatcher.publish(env);
    }
};
```

### Raccomandazione

- Definire canali standard: `game.events`, `combat.events`, `map.events`,
  `actor.events`, `deck.events`.
- Implementare retry logic per eventi critici.
- Loggare fallimenti di dispatch senza bloccare il flow gmRules.

---

## 5. Policy topologiche avanzate in gmMap

### Problema

Le policy topologiche (line-of-sight, terreno, costo movimento) possono essere
più ricche delle primitive GRS base.

### Soluzione consigliata

Tenere in gmMap la semantica complessa e offrire helper in `RuleContext`:

- `LOCATION_IS_REACHABLE(actor, location)`
- `LOCATION_HAS_LOS(actor, location)`
- `MOVE_COST_AT_MOST(actor, location, budget)`

In GRS queste policy diventano condition atomiche leggibili, mentre il calcolo
resta in gmMap.

---

## 6. Checklist implementativa

- Definire interfacce bridge per ogni integrazione indiretta.
- Evitare include cross-lib nei file pubblici gmRules.
- Fare mapping ID/eventi in adapter, non nel parser GRS.
- Testare i bridge con mock di runtime context.
- Mantenere la compatibilità semantica tra trigger lifecycle e gmFlow.

---

## 7. Anti-pattern da evitare

- Chiamare direttamente gmDispatch da parser/compiler GRS.
- Inserire logica di turno nel motore gmRules.
- Fare dipendere gmRules da tipi concreti gmFlow/gmDispatch.
- Duplicare semantica spaziale di gmMap dentro condizioni hardcoded.

---

## Riepilogo

Le integrazioni dirette restano su dati di dominio (attori, mappe, carte).
Le integrazioni non dirette vanno implementate con adapter e contratti
stabili: gmRules valuta regole, gmFlow orchestra, gmDispatch distribuisce
eventi, gmMap/gmAlea forniscono servizi di dominio.
