# Piano Implementazione Integrazioni GRS

Documento di pianificazione attività basato su
`gmRules/specs/grs-integration-suggestions.md`.

---

## Obiettivo

Definire cosa implementare e come, per integrare in modo progressivo
`gmRules` con le librerie del workspace:

- `gmFlow`
- `gmDispatch`
- `gmActor`
- `gmAlea`
- `gmMap`

L'ordine segue la priorità tecnica di intervento, partendo da `gmRules`.

---

## Ordine di Priorità

| Priorità | Libreria | Motivazione |
|---|---|---|
| P0 | `gmRules` | Fondazione DSL/runtime e contratti comuni |
| P1 | `gmFlow` | Orchestrazione turni/fasi e lifecycle eventi |
| P2 | `gmDispatch` | Bridge eventi e distribuzione cross-sistema |
| P3 | `gmActor` | Stato attori, risorse, equipaggiamento |
| P4 | `gmAlea` | Random pipeline, deck/dice e pre/post resolve |
| P5 | `gmMap` | Topologia avanzata, line-of-sight e barriere |

---

## Capitolo 1 - gmRules (P0)

### Cosa implementare

- Estensione vocabolario eventi trigger:
  - `TURN_STARTED`, `TURN_COMPLETED`
  - `ROUND_STARTED`, `ROUND_COMPLETED`
  - `ACTION_WINDOW_OPENED`, `ACTION_WINDOW_CLOSED`
  - `PHASE_CHANGED`, `GAME_STATE_CHANGED`
  - `TOKEN_PRE_DRAW`, `TOKEN_DRAWN`
  - `DICE_PRE_ROLL`, `DICE_ROLLED`, `ALEA_RESOLVED`
  - Eventi actor/map estesi già proposti nel documento base.
- Estensione vocabolario effetti per azioni cross-lib:
  - actor: `MODIFY_RESOURCE`, `EQUIP_ITEM`, `REVIVE_ACTOR`, ecc.
  - alea: `SHUFFLE_ZONE`, `LOOK_TOP_CARD`, `ROLL_DICE`, ecc.
  - map: `SET_LOCATION_PASSABLE`, `CREATE_BARRIER`, ecc.
- Contratto `RuleContext` unificato per resolver actor/map/deck/event.
- Validazione semantica aggiornata per nuovi eventi/effetti.

### Come implementare

1. Aggiornare enum/registry interni (`TriggerType`, `EffectType`, resolver table).
2. Estendere parser/compilatore GRS per riconoscere i nuovi token.
3. Aggiungere fallback chiaro per feature non implementate nel runtime.
4. Estendere lint semantico con errori mirati su argomenti/namespace runtime refs.
5. Aggiornare documentazione spec e mapping GRS -> YAML.

### Deliverable

- Supporto parser e runtime ai nuovi eventi/effetti.
- Suite test unitari su parsing, validazione, risoluzione.
- Changelog API della DSL.

### Criteri di accettazione

- Almeno un file `.grs` di esempio usa ogni nuovo gruppo evento/azione.
- Parsing e lint passano senza warning bloccanti.
- Nessuna regressione sulle regole esistenti.

---

## Capitolo 2 - gmFlow (P1)

### Cosa implementare

- Emissione lifecycle eventi verso gmRules:
  - `TURN_STARTED`, `TURN_COMPLETED`
  - `ROUND_STARTED`, `ROUND_COMPLETED`
  - `ACTION_WINDOW_OPENED`, `ACTION_WINDOW_CLOSED`
  - `ACTION_SUBMITTED`, `ACTION_COMPLETED`, `ACTION_SKIPPED`
  - `PHASE_CHANGED`, `GAME_STATE_CHANGED`
- Pipeline invocazione regole con binding `input.xxx` coerenti.

### Come implementare

1. Definire event contract gmFlow -> gmRules (payload minimo stabile).
2. Inserire hook di emissione evento nei punti del lifecycle.
3. Implementare action gateway:
   - pre-check trigger (`ACTION_SUBMITTED`)
   - execute rule
   - post-check trigger (`ACTION_COMPLETED`)
4. Mappare motivi di blocco in outcome consumabile da state machine gmFlow.

### Deliverable

- Adapter di orchestrazione gmFlow-gmRules.
- Test di integrazione su un intero turno e un intero round.

### Criteri di accettazione

- Le transizioni fase non bypassano i trigger.
- Un blocco azione da trigger produce outcome deterministico.

---

## Capitolo 3 - gmDispatch (P2)

### Cosa implementare

- Bridge `gmRules -> gmDispatch` per `EMIT_EVENT` e `MANUAL_EFFECT`.
- Standardizzazione canali:
  - `game.events`
  - `combat.events`
  - `actor.events`
  - `deck.events`
  - `map.events`

### Come implementare

1. Creare `RuleEventBridge` con mapping namespace -> channel/topic.
2. Costruire envelope con header standard:
   - `source_system=gmRules`
   - `rule_priority`
   - timestamp
3. Introdurre gestione errori non bloccante per publish fallite.
4. Aggiungere metriche minime (success/failure dispatch count).

### Deliverable

- Componente bridge riusabile.
- Test integrazione con mock dispatcher.

### Criteri di accettazione

- Tutti gli eventi rules/trigger vengono tracciati e pubblicati.
- Il fallimento del bus non corrompe la risoluzione regole.

---

## Capitolo 4 - gmActor (P3)

### Cosa implementare

- Supporto eventi actor estesi:
  - `ACTOR_SPAWNED`, `ACTOR_DESPAWNED`
  - `ACTOR_HP_CHANGED`, `ACTOR_DIED`, `ACTOR_REVIVED`
  - `RESOURCE_CHANGED`, `ITEM_EQUIPPED`, `ITEM_UNEQUIPPED`
- Supporto effetti actor estesi:
  - `MODIFY_RESOURCE`, `SET_RESOURCE_MAX`
  - `EQUIP_ITEM`, `UNEQUIP_ITEM`
  - `SPAWN_ACTOR`, `DESPAWN_ACTOR`, `REVIVE_ACTOR`, `CHANGE_TEAM`

### Come implementare

1. Esporre API atomiche actor-safe nel `RuleContext`.
2. Mappare effetti GRS su comandi gmActor idempotenti.
3. Definire policy di consistenza su actor non esistente/già morto.
4. Aggiungere trigger demo (low HP, death check, exhausted).

### Deliverable

- Adapter gmActor per condizioni/effetti.
- Test integrazione su danno, morte, revive, risorse.

### Criteri di accettazione

- Le mutazioni actor sono transazionali per singola catena effetti.
- Errori actor vengono propagati con diagnostica coerente.

---

## Capitolo 5 - gmAlea (P4)

### Cosa implementare

- Eventi pre/post resolve:
  - `TOKEN_PRE_DRAW`, `TOKEN_DRAWN`
  - `DICE_PRE_ROLL`, `DICE_ROLLED`
  - `ALEA_RESOLVED`
- Effetti/azioni alea estesi:
  - `SHUFFLE_ZONE`, `LOOK_TOP_CARD`, `LOOK_BOTTOM_CARD`
  - `SELECT_SPECIFIC_CARD`, `DISCARD_RANDOM`
  - `PLACE_ON_TOP`, `PLACE_ON_BOTTOM`, `ROLL_DICE`

### Come implementare

1. Definire adapter `RuleRandomProvider` per deck e dice.
2. Separare fase pre-resolve (intercettabile) da resolve finale.
3. Garantire determinismo test con seed controllato.
4. Validare mapping zone (`hand`, `discard`, `play`, `banish`, `top`, `bottom`).

### Deliverable

- Adapter gmAlea completo per draw/roll/shuffle/look/select.
- Test integrazione con scenari curse/critical draw-roll.

### Criteri di accettazione

- Stesso seed produce stesso outcome nei test.
- Eventi pre/post resolve sono sempre emessi in ordine corretto.

---

## Capitolo 6 - gmMap (P5)

### Cosa implementare

- Eventi map estesi:
  - `ACTOR_MOVED`, `ACTOR_APPROACHED`, `ACTOR_LEFT_LOCATION`
  - `LOCATION_STATE_CHANGED`, `PATH_BLOCKED`, `LOS_CHANGED`
- Effetti/azioni map estesi:
  - `SET_LOCATION_PASSABLE`, `ADD_LOCATION_TAG`, `REMOVE_LOCATION_TAG`
  - `SET_LOCATION_OWNER`, `CREATE_BARRIER`, `REMOVE_BARRIER`
  - `SPAWN_INTERACTABLE`, `DESPAWN_INTERACTABLE`
- Condizioni avanzate:
  - `LOCATION_IS_REACHABLE`
  - `LOCATION_HAS_LOS`
  - `MOVE_COST_AT_MOST`

### Come implementare

1. Esporre helper topologici via `RuleContext` senza accoppiare parser a gmMap.
2. Isolare policy geometriche complesse nel dominio gmMap.
3. Integrare validazioni su location inesistenti o percorsi invalidi.
4. Aggiungere trigger demo (trap armed, high ground, path blocked).

### Deliverable

- Adapter gmMap per condizioni/effetti/eventi.
- Test integrazione su movimento, LOS, barriere e tag location.

### Criteri di accettazione

- Le regole spaziali non duplicano logica già presente in gmMap.
- Eventi map sono coerenti con stato topologico corrente.

---

## Piano Test Trasversale

1. Test parser/lint GRS per nuovi token.
2. Test unitari adapter per ogni libreria.
3. Test integrazione end-to-end:
   - action submit -> rule eval -> effect chain -> dispatch event.
4. Test regressione su file `.grs` già esistenti.
5. Test robustezza su failure non bloccanti (`[optional]`).

---

## Milestone consigliate

| Milestone | Scope |
|---|---|
| M1 | `gmRules` (vocabolario + parser + lint) |
| M2 | `gmFlow` + `gmDispatch` (orchestrazione + bridge) |
| M3 | `gmActor` + `gmAlea` (dominio gameplay) |
| M4 | `gmMap` avanzato + hardening test end-to-end |

---

## Rischi e Mitigazioni

| Rischio | Mitigazione |
|---|---|
| Esplosione combinatoria eventi/trigger | Introdurre naming convention e contract test per evento |
| Accoppiamento eccessivo tra librerie | Mantenere adapter separati e interfacce stabili |
| Regressioni su regole esistenti | Test regressione automatici su esempi `.grs` legacy |
| Non determinismo su random | Seed controllato e replay test |

---

## Output atteso finale

- Stack integrato `gmRules + gmFlow + gmDispatch + gmActor + gmAlea + gmMap`
  con contratto eventi/azioni coerente e testato.
- Documentazione aggiornata delle estensioni GRS e delle capability runtime.
