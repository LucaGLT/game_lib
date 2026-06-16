# GRS — Game Rule Script v0.3

Language Specification for gmRules

---

## Obiettivo

GRS è un DSL testuale minimale e leggibile per descrivere regole di gioco
senza scrivere direttamente YAML o JSON.

Il testo GRS viene compilato in JSON/YAML validato contro
`gmRules/specs/game-rules.schema.json` è poi usato per
code generation verso gmRules.

---

## Principi di design

- Leggibile come prosa strutturata.
- Ogni blocco ha un tipo, nessun tipo è ripetuto riga per riga.
- Definizioni con `::` — distinguono dichiarazioni da valori.
- Keyword inglesi semplici: `ON`, `IF`, `THEN`, `AND`, `OR`, `NOT`,
  `AND THEN`.
- Parametri posizionali, senza nomi — l'ordine è fisso e documentato per tipo.
- Ref runtime espliciti per i valori non noti a compile time.
- Errore se un nome non è definito.

---

## Struttura documento

Un file `.grs` è composto da blocchi ordinati (l'ordine nei blocchi
è libero, l'ordine dei blocchi è consigliato ma non obbligatorio):

```
@meta       ... @end
@targets    ... @end
@conditions ... @end
@effects    ... @end
@statuses   ... @end
@rules      ... @end
@triggers   ... @end
```

Ogni riga dentro un blocco è una definizione oppure un attributo meta.
Il tipo dell'oggetto che si sta definendo è dato dal blocco: non si
ripete mai nella riga.

---

## Commenti

```
# Questo è un commento — ignorato dal parser
```

---

## Blocco `@meta`

Attributi chiave-valore, uno per riga.

```grs
@meta
game    checkers
ns      checkers.core
version 1.0.0
min_gmrules 0.5.0
@end
```

Mapping YAML:

| GRS key | YAML key |
|---|---|
| `game` | `meta.game_id` |
| `ns` | `meta.namespace` |
| `version` | `meta.schema_version` |
| `min_gmrules` | `meta.compatibility.gmRules_min` |

---

## Blocco `@targets`

Ogni riga definisce un named target.

```
Name :: TargetKind TargetSelector [range RangeType [N]] [required | optional] [no_self]
```

Un target è un "filtro di selezione": dice al motore **chi puo essere colpito** da una regola/effetto. Il nome (es. `Target_Enemy`) viene poi riusato in `@effects`, `@rules` e `@statuses`.

### Argomenti posizionali (in ordine)

#### 1) `TargetKind` (obbligatorio)

Definisce il dominio dell'oggetto selezionabile.

- `ACTOR`: personaggi/mostri/eroi.
- `ACTOR_GROUP`: gruppo di attori.
- `LOCATION`: casella/area/stanza.
- `CARD`: carta singola.
- `DECK`: mazzo.
- `ITEM`: oggetto.
- `INTERACTABLE`: oggetto interagibile del mondo.
- `NONE`: nessun target esplicito.

Quando usare `NONE`:

- effetti di sola notifica (`MANUAL_EFFECT`, logging, telemetria).
- effetti globali non legati a un bersaglio singolo.

#### 2) `TargetSelector` (obbligatorio)

Definisce **come** vengono scelti i target dentro il dominio.

Selettori tipici:

- `SELF`: il source actor.
- `SOURCE`: sorgente dichiarata dell'effetto.
- `SELECTED_ACTOR`: target passato dal chiamante.
- `SELECTED_ALLY`: target selezionato ma validato come alleato.
- `SELECTED_ENEMY`: target selezionato ma validato come nemico.
- `ALL_ACTORS_IN_LOCATION`: tutti gli attori nella location del source.
- `ALL_ALLIES_IN_LOCATION`: tutti gli alleati nella location del source.
- `ALL_ENEMIES_IN_LOCATION`: tutti i nemici nella location del source.
- `ACTORS_WITH_STATUS`: attori che hanno uno status richiesto.
- `LOCATION`: location selezionata dal chiamante.
- `SELECTED_CARD`: carta selezionata.
- `SELECTED_ITEM`: oggetto selezionato.
- `MANUAL`: target forniti direttamente dal runtime.

#### 3) `range RangeType [N]` (opzionale, default `NONE`)

Vincolo spaziale applicato dopo il selector.

- `NONE`: nessun vincolo.
- `SAME_LOCATION`: stessa location del source.
- `ADJACENT_LOCATION`: location adiacente.
- `WITHIN_N_LOCATIONS N`: entro N passi (N obbligatorio).
- `ANY_VISIBLE_LOCATION`: location visibile (semantica game-specific).
- `GLOBAL`: nessun limite geografico.

Esempio:

- `range WITHIN_N_LOCATIONS 2` significa "entro raggio 2".

#### 4) `required | optional` (opzionale, default `required`)

Specifica cosa succede se non viene trovato nessun target valido.

- `required`: errore di target resolution.
- `optional`: zero target è accettato (la regola puo proseguire).

#### 5) `no_self` (opzionale)

Shortcut per `allow_self: false`.
Impedisce che il source actor possa targettare se stesso, anche se selector e range lo permetterebbero.

---

### Keyword per i tag

Le keyword tag si applicano **dopo** selector e range, come filtro finale.

- `needs TAG1,TAG2,...`
    Significa: il target deve avere **tutti** i tag elencati.
    Mapping: `required_tags`.

- `forbids TAG1,TAG2,...`
    Significa: il target non deve avere **nessuno** dei tag elencati.
    Mapping: `forbidden_tags`.

Ordine logico dei filtri:

1. selector
2. range
3. `needs`
4. `forbids`
5. `no_self`

Esempio pratico:

- `Target_BerserkEnemy :: ACTOR SELECTED_ENEMY range ADJACENT_LOCATION needs elite,berserk forbids invisible no_self`

Interpretazione:

- deve essere un nemico selezionato
- adiacente al source
- con tag `elite` e `berserk`
- senza tag `invisible`
- non puo essere il source stesso

```grs
@targets
Target_Piece  :: ACTOR SELECTED_ACTOR required
Target_Enemy  :: ACTOR SELECTED_ENEMY required
Target_Self   :: ACTOR SELF           required
Target_Allies :: ACTOR ALL_ALLIES_IN_LOCATION range SAME_LOCATION optional no_self
@end
```

---

## Blocco `@conditions`

Ogni riga definisce una condizione nominata o composta.

### Condizione atomica

```
Name :: FunctionName(Arg1, Arg2, ...)
```

Argomenti posizionali per tipo di condizione:

| Tipo | Arg1 | Arg2 |
|---|---|---|
| `ACTOR_EXISTS` | actor_ref | — |
| `ACTOR_HAS_TAG` | actor_ref | tag_literal |
| `ACTOR_HAS_STATUS` | actor_ref | status_id |
| `ACTOR_HP_AT_OR_BELOW` | actor_ref | amount |
| `ACTOR_HP_AT_OR_ABOVE` | actor_ref | amount |
| `ACTOR_IN_LOCATION` | actor_ref | location_ref |
| `LOCATION_EXISTS` | location_ref | — |
| `LOCATION_HAS_TAG` | location_ref | tag_literal |
| `LOCATION_IS_ADJACENT` | location_ref | location_ref |
| `TARGET_EXISTS` | — | — |
| `TARGET_HAS_TAG` | tag_literal | — |
| `TARGET_HAS_STATUS` | status_id | — |
| `DECK_HAS_AT_LEAST` | deck_ref | amount |
| `CARD_IN_ZONE` | card_ref | zone_name |
| `ALWAYS` | — | — |
| `NEVER` | — | — |

### Condizione composta (ref a condition nominate)

```
Name :: C_A AND C_B
Name :: C_A OR C_B
Name :: NOT C_A
Name :: C_A AND (C_B OR C_C)
```

Parentesi supportate per raggruppare.

```grs
@conditions
C_ActorExists     :: ACTOR_EXISTS(input.actor_id)
C_DestinationOk   :: LOCATION_EXISTS(input.destination)
C_MoveValid       :: C_ActorExists AND C_DestinationOk
C_CaptureTarget   :: ACTOR_EXISTS(input.enemy_actor_id)
C_PromoWhite      :: ACTOR_HAS_TAG(input.actor_id, white_piece)
                     AND LOCATION_HAS_TAG(input.destination, promotion_white)
C_PromoBlack      :: ACTOR_HAS_TAG(input.actor_id, black_piece)
                     AND LOCATION_HAS_TAG(input.destination, promotion_black)
C_Promotion       :: C_PromoWhite OR C_PromoBlack
C_TargetIsEnemy   :: TARGET_EXISTS
@end
```

---

## Blocco `@effects`

Ogni riga definisce un effetto nominato.

```
Name :: EffectType(TargetRef, ValueArg)
```

### Tabella argomenti posizionali (riferimento rapido)

| Tipo | Arg1 | Arg2 |
|---|---|---|
| `MOVE_ACTOR` | target_ref | destination_ref |
| `DEAL_DAMAGE` | target_ref | amount |
| `HEAL` | target_ref | amount |
| `APPLY_STATUS` | target_ref | status_id |
| `REMOVE_STATUS` | target_ref | status_id |
| `ADD_TAG` | target_ref | tag |
| `REMOVE_TAG` | target_ref | tag |
| `DRAW_CARDS` | target_ref | deck_ref (+ opz. amount) |
| `MOVE_CARD_TO_ZONE` | target_ref | zone_name |
| `EMIT_EVENT` | target_ref | event_name |
| `MANUAL_EFFECT` | — | event_name |

### Modificatori (suffissi sulla riga)

- `[optional]` — fallimento diventa warning, la catena prosegue
- `[stop]` — stop_on_failure: true (default implicito)
- `[continue]` — stop_on_failure: false, la catena prosegue anche su errore

---

### `MOVE_ACTOR(target, destination_ref)`

Sposta l'attore target nella location indicata da `destination_ref`.
Il ref puo essere un ref runtime (`input.destination`) oppure un literal (`sq_17`).

```grs
@effects
E_Move :: MOVE_ACTOR(Target_Piece, input.destination)
# Sposta la pedina selezionata nella casella scelta dal giocatore.
@end
```

---

### `DEAL_DAMAGE(target, amount)`

Riduce i punti vita del target del valore `amount`.
Amount e un intero positivo. Il contesto (RuleContext) applica il
clamping a zero e le transizioni di stato (KO, DEAD).

```grs
@effects
E_Veleno_Danno :: DEAL_DAMAGE(Target_Self, 2)
# Infligge 2 danni all'attore che porta il veleno (usato in ON_TURN_END).
@end
```

---

### `HEAL(target, amount)`

Aumenta i punti vita del target del valore `amount`.
Non supera il massimo HP. Amount e un intero positivo.

```grs
@effects
E_Cura :: HEAL(Target_Piece, 3)
# Recupera 3 HP all'attore selezionato.
@end
```

---

### `APPLY_STATUS(target, status_id)`

Applica lo status con id `status_id` al target.
Lo stacking e governato dalla `StackingMode` dichiarata nel blocco
`@statuses`. Scatena automaticamente gli hook `ON_APPLY`.

```grs
@effects
E_Captured  :: APPLY_STATUS(Target_Enemy, captured)  [stop]
# Marca la pedina nemica come catturata. Se fallisce blocca la catena.

E_Avvelena  :: APPLY_STATUS(Target_Enemy, poisoned)  [optional]
# Tenta di avvelenare il nemico; se fallisce la catena continua.
@end
```

---

### `REMOVE_STATUS(target, status_id)`

Rimuove dal target tutte le istanze dello status con id `status_id`.
Scatena automaticamente gli hook `ON_REMOVE`.

```grs
@effects
E_CuraVeleno :: REMOVE_STATUS(Target_Piece, poisoned)
# Rimuove il veleno dall'attore selezionato.
@end
```

---

### `ADD_TAG(target, tag)`

Aggiunge un tag classificatorio all'attore target.
I tag sono stringe libere usate come flag semantici (es. `king`,
`stunned`, `visible`, `elite`).

```grs
@effects
E_AddKing :: ADD_TAG(Target_Piece, king)
# Promuove la pedina a dama aggiungendo il tag king.
@end
```

---

### `REMOVE_TAG(target, tag)`

Rimuove un tag dall'attore target. Nessun effetto se il tag non e presente.

```grs
@effects
E_RimuoviStordito :: REMOVE_TAG(Target_Piece, stunned)
# Fine stordimento: rimuove il tag stunned.
@end
```

---

### `DRAW_CARDS(target, deck_ref)`

Fa pescare `amount` carte dal mazzo `deck_ref` per il target.
Il ref puo essere un ID mazzo fisso o un ref runtime.
`amount` si specifica con il modificatore implicito posizionale.

Nota: in GRS Arg1 e il target, Arg2 e il deck_ref; la quantita
e un terzo argomento opzionale (default 1).

```grs
@effects
E_PescaCarta :: DRAW_CARDS(Target_Piece, input.deck_id, 2)
# Il giocatore pesca 2 carte dal mazzo indicato a runtime.
@end
```

---

### `MOVE_CARD_TO_ZONE(target, zone_name)`

Sposta una carta (il target e di tipo CARD) nella zona `zone_name`
all'interno del mazzo da cui proviene.
Typical zones: `hand`, `discard`, `banish`, `play`.

```grs
@effects
E_ScartaCarta :: MOVE_CARD_TO_ZONE(Target_Card, discard)
# Manda la carta selezionata negli scarti.
@end
```

---

### `EMIT_EVENT(target, event_name)`

Emette un evento nominato verso il bus eventi del gioco (gmDispatch o
equivalente), con il target come soggetto.
Non muta lo stato — serve per notifiche, logging, reaction chain.

```grs
@effects
E_NotificaDanno :: EMIT_EVENT(Target_Enemy, checkers.piece.damaged)
# Notifica che la pedina nemica ha subito danno, senza mutare stato.
@end
```

---

### `MANUAL_EFFECT(event_name)`

Escape hatch: emette un evento ma non muta nulla.
Usato per azioni game-specific che il runtime gestisce fuori da gmRules,
o per log/debug/hook personalizzati.
Non ha target (Arg1 assente).

```grs
@effects
E_LogMove :: MANUAL_EFFECT(checkers.move.applied)  [optional]
# Segnala al game loop che e avvenuta una mossa. Non fa nulla di concreto.
@end
```

---

## Blocco `@rules`

Ogni riga definisce una regola con keyword parlate.

### Sintassi canonica

```
Name [priority=N] [disabled] ::
    IF ConditionExpr
    ON TargetRef
    THEN EffectChain
```

Forme alternative accettate (tutte equivalenti):

```
Name :: IF ConditionExpr ON TargetRef THEN EffectChain
Name :: ON TargetRef IF ConditionExpr THEN EffectChain
```

### ConditionExpr nelle regole

Stesse regole del blocco `@conditions` ma inline:
usa nomi definiti in `@conditions` oppure funzioni dirette.

```
IF C_MoveValid AND NOT C_PromoWhite
IF C_A OR (C_B AND C_C)
```

### EffectChain

```
THEN E_Move
THEN E_Move AND THEN E_Captured
THEN E_Move AND THEN E_Captured AND THEN E_LogMove
```

Ogni `AND THEN` aggiunge un effetto alla sequenza.
Modificatori opzionali per effetto inline:

- `THEN E_Move?` — optional
- `THEN E_Move!` — stop_on_failure: true

```grs
@rules
Move_Simple  [priority=100] ::
    IF C_MoveValid
    ON Target_Piece
    THEN E_Move AND THEN E_LogMove

Move_Capture [priority=200] ::
    IF C_MoveValid AND C_CaptureTarget
    ON Target_Piece
    THEN E_Move AND THEN E_Captured AND THEN E_LogMove

Promotion    [priority=300] ::
    IF C_PromoWhite OR C_PromoBlack
    ON Target_Piece
    THEN E_AddKing
@end
```

---

## Blocco `@statuses`

Ogni status usa sotto-blocchi inline indentati.

```
Name :: StackingMode DurationType [amount N] [value V]
    ON_APPLY      EffectRef [AND THEN EffectRef ...]
    ON_REMOVE     EffectRef
    ON_TURN_START EffectRef
    ON_TURN_END   EffectRef
```

---

### StackingMode — cosa succede se lo stesso status viene applicato due volte

#### `REFRESH`

Se il target ha gia lo status, la durata viene azzerata e ripartita
dall'inizio. Gli stack rimangono a 1. Il blocco `ON_APPLY` non viene
rieseguito.

```grs
@statuses
burnig :: REFRESH FOR_N amount 3
    ON_TURN_END DEAL_DAMAGE(Target_Self, 1) [optional]
# Una seconda applicazione di burning azzera il conto a 3 turni,
# senza aggiungere danno doppio.
@end
```

#### `ADD_STACK`

Ogni applicazione aggiunge uno stack fino al massimo (`max_stacks`).
Usato per effetti che si intensificano: piu stack = piu danno/effetto.
Se si raggiunge il massimo, si comporta come `REFRESH`.

```grs
@statuses
frenzy :: ADD_STACK UNTIL_REMOVED
    ON_APPLY ADD_TAG(Target_Piece, frenzy)
# Prima applicazione: 1 stack. Seconda: 2 stack. Effetto scala con gli stack.
# Limite di stack dichiarato nel compilatore (es. max_stacks=3).
@end
```

#### `ONE_ONLY`

Se lo status e gia presente, la nuova applicazione viene scartata.
Lo status esistente rimane immutato. Utile per stati "una tantum" che
non devono essere resettati.

```grs
@statuses
captured :: ONE_ONLY UNTIL_REMOVED
    ON_APPLY ADD_TAG(Target_Piece, captured)
# Una pedina gia catturata non puo essere catturata una seconda volta.
@end
```

#### `REPLACE`

L'istanza esistente viene rimossa (con `ON_REMOVE`) e sostituita da
una nuova istanza fresca (con `ON_APPLY`). Usato quando la nuova
applicazione deve resettare completamente lo status.

```grs
@statuses
shield :: REPLACE FOR_N amount 2
    ON_APPLY   ADD_TAG(Target_Piece, shielded)
    ON_REMOVE  REMOVE_TAG(Target_Piece, shielded)
# Applicare uno scudo mentre e gia attivo cancella il vecchio
# e ricomincia da 2 turni.
@end
```

#### `UNIQUE_BY_SOURCE`

Permette al massimo una istanza per sorgente (`source_id`).
Se la stessa sorgente riapplica lo status, fa REFRESH.
Se una sorgente diversa applica lo status, aggiunge una seconda istanza.
Usato per effetti come maledizioni che piu attori possono applicare
indipendentemente.

```grs
@statuses
curse :: UNIQUE_BY_SOURCE FOR_N amount 5
    ON_TURN_START DEAL_DAMAGE(Target_Self, 1) [optional]
# Strega A e Strega B possono entrambe maledire lo stesso bersaglio:
# il bersaglio subisce danno due volte per turno.
# Se Strega A maledice di nuovo, la sua istanza viene rinnovata.
@end
```

---

### DurationType — quando scade lo status

#### `PERMANENT`

Lo status non scade mai automaticamente.
Puo essere rimosso solo da `REMOVE_STATUS` esplicito o da `ON_REMOVE`
triggerato da altra regola.

```grs
@statuses
blind :: ONE_ONLY PERMANENT
    ON_APPLY  ADD_TAG(Target_Piece, blind)
    ON_REMOVE REMOVE_TAG(Target_Piece, blind)
# Cecita permanente: rimane finche una cura specifica non la rimuove.
@end
```

#### `UNTIL_REMOVED`

Come PERMANENT, ma il nome segnala esplicitamente l'intento:
lo status dura finche non viene rimosso da una regola esplicita.
Semantica identica a PERMANENT nel runtime; differisce solo
nell'intenzione del designer.

```grs
@statuses
captured :: ONE_ONLY UNTIL_REMOVED
    ON_APPLY ADD_TAG(Target_Piece, captured)
# La pedina resta catturata finche il game loop non chiama REMOVE_STATUS.
@end
```

#### `FOR_N amount N`

Lo status dura `N` attivazioni dell'attore che lo porta.
Ogni volta che scatta `ON_TURN_END` per quell'attore,
il contatore decresce. A zero lo status scade e scatta `ON_REMOVE`.

```grs
@statuses
poisoned :: REFRESH FOR_N amount 3
    ON_TURN_END DEAL_DAMAGE(Target_Self, 1) [optional]
    ON_REMOVE   REMOVE_TAG(Target_Self, poisoned)
# Il veleno dura 3 turni dell'attore avvelenato,
# poi si rimuove automaticamente.
@end
```

#### `UNTIL_NEXT_TURN`

Lo status scade alla fine del turno corrente dell'attore che lo porta
(alla prima occorrenza di `ON_TURN_END` dopo l'applicazione).
Usato per buff/debuff di un solo turno.

```grs
@statuses
haste :: REFRESH UNTIL_NEXT_TURN
    ON_APPLY  ADD_TAG(Target_Piece, haste)
    ON_REMOVE REMOVE_TAG(Target_Piece, haste)
# Celerità dura solo fino alla fine del turno corrente.
@end
```

#### `WHILE_IN_LOCATION value V`

Lo status e attivo finche l'attore rimane nella location `V`.
Appena l'attore lascia quella location (rilevato da `ACTOR_MOVED`
oppure da check esplicito nel game loop), lo status scade.

```grs
@statuses
high_ground :: REFRESH WHILE_IN_LOCATION value sq_15
    ON_APPLY  ADD_TAG(Target_Piece, elevated)
    ON_REMOVE REMOVE_TAG(Target_Piece, elevated)
# Il bonus altura dura solo mentre la pedina e sulla casella sq_15.
@end
```

---

## Blocco `@triggers`

Ogni trigger reagisce a un evento di gioco.

```
Name [priority=N] [disabled] ::
    ON_EVENT EventType
    IF ConditionExpr
    THEN EffectChain
```

I ref disponibili negli `@triggers` sono `event.xxx` (non `input.xxx`).
I campi tipici dell'evento sono:

| Campo evento | Contenuto |
|---|---|
| `event.actor_id` | Attore che ha generato l'evento |
| `event.target_id` | Attore/oggetto bersaglio dell'evento |
| `event.from_location` | Casella di partenza (per MOVED/ENTERED) |
| `event.to_location` | Casella di arrivo (per MOVED/ENTERED) |
| `event.source_id` | Chi ha causato l'evento (effetto/sorgente) |
| `event.status_id` | Status coinvolto (per STATUS_APPLIED) |
| `event.amount` | Quantita numerica (per DAMAGED) |

---

### `ACTION_SUBMITTED`

L'evento scatta nel momento in cui un'azione e stata inviata al
flow controller, **prima** della sua esecuzione. Utile per intercettare
e annullare o modificare azioni (es. blocco per stordimento).

```grs
@triggers
T_BlockIfStunned [priority=50] ::
    ON_EVENT ACTION_SUBMITTED
    IF ACTOR_HAS_TAG(event.actor_id, stunned)
    THEN MANUAL_EFFECT(game.action.blocked)
# Se l'attore e stordito, l'azione viene intercettata
# e il game loop riceve l'evento di blocco.
@end
```

---

### `ACTION_COMPLETED`

L'evento scatta dopo che un'azione e stata eseguita completamente.
Usato per cleanup di fine azione (rimuovere status temporanei,
aggregare punteggi, avanzare fase).

```grs
@triggers
T_CleanupCaptured [priority=100] ::
    ON_EVENT ACTION_COMPLETED
    THEN MANUAL_EFFECT(checkers.cleanup.captured) [optional]
# Dopo ogni azione, notifica il game loop che deve rimuovere
# le pedine con status captured dalla scacchiera.
@end
```

---

### `CARD_PLAYED`

L'evento scatta quando un attore gioca una carta dalla mano.
Disponibili `event.actor_id` (chi ha giocato) e `event.target_id`
(la carta giocata).

```grs
@triggers
T_OnAttackCard [priority=100] ::
    ON_EVENT CARD_PLAYED
    IF TARGET_HAS_TAG(attack_card)
    THEN EMIT_EVENT(Target_Enemy, game.attack_card.played)
# Ogni volta che viene giocata una carta con tag attack_card,
# notifica il bus eventi.
@end
```

---

### `ACTOR_DAMAGED`

L'evento scatta dopo che un attore ha subito danno.
Disponibili `event.actor_id` (chi ha subito) e `event.amount`
(quantita di danno ricevuto).

```grs
@triggers
T_RageSpark [priority=100] ::
    ON_EVENT ACTOR_DAMAGED
    IF ACTOR_HAS_TAG(event.actor_id, berserker)
    THEN ADD_TAG(Target_Self, enraged) [optional]
# Un berserker che subisce danno guadagna il tag enraged.
@end
```

---

### `ACTOR_MOVED`

L'evento scatta dopo che un attore si e spostato.
Disponibili `event.actor_id`, `event.from_location`, `event.to_location`.
Usato per promozioni, trappole, effetti geografici.

```grs
@triggers
T_Promotion [priority=200] ::
    ON_EVENT ACTOR_MOVED
    IF ACTOR_EXISTS(event.actor_id) AND LOCATION_EXISTS(event.to_location)
    THEN MANUAL_EFFECT(checkers.trigger.promotion) [optional]
# Dopo ogni spostamento, notifica il game loop che deve
# controllare se la pedina va promossa.
@end
```

---

### `STATUS_APPLIED`

L'evento scatta dopo che uno status e stato applicato a un attore.
Disponibili `event.actor_id` (chi ha ricevuto) e `event.status_id`
(quale status e stato applicato).

```grs
@triggers
T_NotifyCapture [priority=100] ::
    ON_EVENT STATUS_APPLIED
    IF TARGET_HAS_STATUS(captured)
    THEN MANUAL_EFFECT(checkers.piece.capture.confirmed)
# Ogni volta che uno status captured viene applicato,
# informa il sistema di scoring.
@end
```

---

### `TIME_REACHED`

L'evento scatta quando il clock interno del gioco raggiunge
un valore specifico (tick, round, fase). Disponibile `event.amount`
(il valore di tempo raggiunto).

```grs
@triggers
T_EndGame [priority=999] ::
    ON_EVENT TIME_REACHED
    IF ALWAYS
    THEN MANUAL_EFFECT(game.end_of_round)
# Alla fine di ogni round, avvia la procedura di fine round.
@end
```

---

### `LOCATION_ENTERED`

L'evento scatta nel momento in cui un attore entra in una location.
Differisce da `ACTOR_MOVED`: MOVED scatta dopo lo spostamento completo,
ENTERED scatta nel momento dell'ingresso, utile per trappole e
checks geografici immediati.
Disponibili `event.actor_id` e `event.to_location`.

```grs
@triggers
T_TrapSquare [priority=100] ::
    ON_EVENT LOCATION_ENTERED
    IF LOCATION_HAS_TAG(event.to_location, trapped)
    THEN DEAL_DAMAGE(Target_Self, 2)
# Entrare in una casella con tag trapped infligge 2 danno immediatamente.
@end
```

---

## Runtime Refs

Le regole GRS (e gmRules) sono **definizioni statiche** — scritte una
volta sola nel file di regole.

Ma i valori su cui operano sono **dinamici** — dipendono da cosa succede a runtime: quale pedina il giocatore ha scelto, in quale casella vuole spostarla, quale attore ha subito danno.

I ref runtime sono **segnaposto nominati** che il runtime rimpiazza con i valori reali nel momento in cui la regola viene eseguita.

### 3 namespace

| Prefix | Fonte del valore | Esempio |
|---|---|---|
| `input.xxx` | Chiamante (giocatore, AI, UI) | `input.actor_id` = pedina scelta |
| `event.xxx` | Evento che ha attivato il trigger | `event.to_location` = casella di destinazione |
| `target.xxx` | Attore che ha eseguito la regola | `target.actor_id` = la pedina stessa |

### Esempio concreto

Con runtime ref scrivi **una regola parametrica**:

```
Move_Simple :: IF C_MoveValid ON Target_Piece THEN E_Move

# con:
E_Move :: MOVE_ACTOR(Target_Piece, input.destination)
#                                  ^^^^^^^^^^^^^^^^^
#                                  valore fornito a runtime
```

### Mapping verso gmRules/C++

In C++, il runtime chiama il resolver cosi:

```cpp
// Il caller riempie i binding prima di chiamare il resolver:
TargetRef selected_target;
selected_target.kind = TargetKind::ACTOR;
selected_target.id = "w_01";            // <- era input.actor_id

EffectSpec effect = registry.build("Move_Simple");
effect.value = "sq_17";                  // <- era input.destination

engine.resolve_effect(effect, "w_01", {selected_target}, ctx);
```

Il generatore di codice traduce i `value_ref: input.destination` in parametri della funzione factory o in un oggetto `RuntimeBindings` passato al momento dell'invocazione.

---

## Integrazione con le altre librerie

Questa sezione chiarisce come gmRules si integra con il resto di game_lib.
gmRules resta un motore di valutazione regole: non possiede lo stato globale,
ma legge/scrive tramite `RuleContext` e tramite adapter runtime.

### Matrice integrazione

| Libreria | Tipo integrazione | Come avviene |
|---|---|---|
| `gmActor` | Diretta (runtime) | `ACTOR_*`, tag, status, HP e identity sono risolti su attori runtime |
| `gmAlea` | Diretta (runtime) | `CARD_IN_ZONE`, `DRAW_CARDS`, `MOVE_CARD_TO_ZONE` su deck/zone/carte |
| `gmMap` | Diretta (runtime) | `LOCATION_*`, adiacenza, spostamenti e filtri geografici |
| `gmDispatch` | Indiretta (bridge) | `EMIT_EVENT` / `MANUAL_EFFECT` verso event bus applicativo |
| `gmFlow` | Indiretta (orchestrazione) | dispatch di action/turn lifecycle, trigger events e sequencing |

### gmActor

Interazione diretta tramite primitive di condizione/effetto:

- condizioni: `ACTOR_EXISTS`, `ACTOR_HAS_TAG`, `ACTOR_HAS_STATUS`,
  `ACTOR_HP_AT_OR_BELOW`, `ACTOR_HP_AT_OR_ABOVE`
- effetti: `DEAL_DAMAGE`, `HEAL`, `ADD_TAG`, `REMOVE_TAG`,
  `APPLY_STATUS`, `REMOVE_STATUS`

In pratica, gmRules usa ID attore e metadati actor-centrici; la gestione
concreta degli attori rimane nel dominio gmActor (o adapter equivalente).

### gmAlea

Interazione diretta quando il dominio usa carte/mazzi:

- condizioni: `CARD_IN_ZONE`, `DECK_HAS_AT_LEAST`
- effetti: `DRAW_CARDS`, `MOVE_CARD_TO_ZONE`

gmRules non implementa il deck engine: richiede solo che il runtime esponga
operazioni coerenti su zone (`hand`, `discard`, `play`, `banish`, ecc.).

### gmMap

Interazione diretta per vincoli spaziali e movimento:

- condizioni: `LOCATION_EXISTS`, `LOCATION_HAS_TAG`, `LOCATION_IS_ADJACENT`,
  `ACTOR_IN_LOCATION`
- target modifiers: `range SAME_LOCATION`, `ADJACENT_LOCATION`,
  `WITHIN_N_LOCATIONS N`, `ANY_VISIBLE_LOCATION`
- effetti: `MOVE_ACTOR`

gmRules delega a gmMap la semantica topologica (adiacenza, visibilità,
reachability), mantenendo nel DSL solo la descrizione declarativa.

### gmDispatch

Integrazione non diretta nel core: gmRules produce segnali (`EMIT_EVENT`,
`MANUAL_EFFECT`) e il bridge runtime li pubblica sul dispatcher reale.

Pattern consigliato:

1. gmRules emette un evento dominio (`combat.hit.confirmed`).
2. Adapter traduce in envelope/evento gmDispatch.
3. Altri sistemi (UI, telemetry, AI observer) si sottoscrivono via gmDispatch.

### gmFlow

Integrazione non diretta: gmFlow orchestra il ciclo turni/azioni e invoca
gmRules nei punti stabiliti del flow.

Pattern tipico:

1. gmFlow riceve comando azione (player/AI).
2. gmFlow costruisce `input.xxx` e invoca rule evaluation.
3. gmFlow emette eventi lifecycle (`ACTION_SUBMITTED`, `ACTION_COMPLETED`).
4. gmRules valuta trigger collegati e produce effetti/eventi.
5. gmFlow decide avanzamento fase/turno.

Per linee guida operative sui bridge non diretti, vedi
`gmRules/specs/grs-integration-suggestions.md`.

---

## Grammatica EBNF v0.1

```ebnf
Document =
    { Block }+ ;

Block =
      MetaBlock
    | TargetBlock
    | ConditionBlock
    | EffectBlock
    | RuleBlock
    | StatusBlock
    | TriggerBlock ;

ConditionExprOrName =
      <condition_name>
    | <ConditionExpr> ;

EffectChain =
    <EffectEntry> { "AND THEN" <EffectEntry> } ;

EffectEntry =
      <effect_name>
    | <EffectType> "(" <TargetRefOrName> [ "," <Arg> { "," <Arg> } ] ")"
    [ "?" ] ;

EffectModifier =
      "[optional]"
    | "[stop]"
    | "[continue]" ;

ConditionExpr =
      <CondAtom>
    | "NOT" <ConditionExpr>
    | <ConditionExpr> "AND" <ConditionExpr>
    | <ConditionExpr> "OR" <ConditionExpr>
    | "(" <ConditionExpr> ")" ;

CondAtom =
    <ConditionType> "(" [ <Arg> { "," <Arg> } ] ")" ;

Arg =
      <Ref>
    | <StringLit>
    | <Integer>
    | <Identifier> ;

Ref =
    ( "input" | "event" | "source" ) "." <Identifier> ;

TargetRefOrName =
      <target_name>
    | <Ref>
    | <Identifier> ;

Identifier =
    <Letter> { <Letter> | <Digit> | "_" | "." } ;

StringLit =
    '"' { <AnyCharExceptQuote> } '"' ;

Integer =
    [ "-" ] <Digit> { <Digit> } ;
```

### EBNF sintetica per blocco

Le forme sotto descrivono la struttura minima di ogni blocco GRS.

Convenzioni usate:

- `(...)` raggruppamento
- `|` alternativa
- `[...]` opzionale
- `{...}` ripetizione zero o più volte
- `{...}+` ripetizione una o più volte

---

#### `@meta`

```ebnf
MetaBlock =
    "@meta"
        "game" <GameId>
        "ns" <Namespace>
        "version" <Version>
        [ "min_gmrules" <Version> ]
    "@end" ;
```

---

#### `@targets`

```ebnf
TargetBlock =
    "@targets"
        { TargetLine }+
    "@end" ;

TargetLine =
    <target_name> "::" <TargetKind> <TargetSelector>
    [ "range" <RangeType> [ <Integer> ] ]
    [ "required" | "optional" ]
    [ "needs" <Tag> { "," <Tag> } ]
    [ "forbids" <Tag> { "," <Tag> } ]
    [ "no_self" ] ;
```

---

#### `@conditions`

```ebnf
ConditionBlock =
    "@conditions"
        { ConditionLine }+
    "@end" ;

ConditionLine =
    <condition_name> "::" <ConditionExpr> ;

ConditionExpr =
      <CondAtom>
    | "NOT" <ConditionExpr>
    | <ConditionExpr> "AND" <ConditionExpr>
    | <ConditionExpr> "OR" <ConditionExpr>
    | "(" <ConditionExpr> ")" ;

CondAtom =
    <ConditionType> "(" [ <Arg> { "," <Arg> } ] ")" ;
```

---

#### `@effects`

```ebnf
EffectBlock =
    "@effects"
        { EffectLine }+
    "@end" ;

EffectLine =
    <effect_name> "::" <EffectType> "(" <TargetRefOrName>
    [ "," <Arg> { "," <Arg> } ] ")"
    { <EffectModifier> } ;

EffectModifier =
      "[optional]"
    | "[stop]"
    | "[continue]" ;
```

---

#### `@rules`

```ebnf
RuleBlock =
    "@rules"
        { RuleLine }+
    "@end" ;

RuleLine =
    <rule_name> { <RuleAttr> } "::"
    [ "IF" <ConditionExprOrName> ]
    "ON" <target_name>
    "THEN" <EffectChain> ;

RuleAttr =
      "[priority=" <Integer> "]"
    | "[disabled]" ;

EffectChain =
    <EffectEntry> { "AND THEN" <EffectEntry> } ;

EffectEntry =
    <effect_name> [ "?" ] ;
```

---

#### `@statuses`

```ebnf
StatusBlock =
    "@statuses"
        { StatusLine }+
    "@end" ;

StatusLine =
    <status_name> "::" <StackingMode> <DurationType>
    { <StatusAttr> }
    { <StatusHook> } ;

StatusAttr =
      "amount" <Integer>
    | <Identifier> ;

StatusHook =
      "ON_APPLY" <EffectChain>
    | "ON_REMOVE" <EffectChain>
    | "ON_TURN_START" <EffectChain>
    | "ON_TURN_END" <EffectChain>
    | "ON_ACTION_COMPLETED" <EffectChain> ;
```

---

#### `@triggers`

```ebnf
TriggerBlock =
    "@triggers"
        { TriggerLine }+
    "@end" ;

TriggerLine =
    <trigger_name> { <TriggerAttr> } "::"
    "ON_EVENT" <TriggerType>
    [ "IF" <ConditionExprOrName> ]
    "THEN" <EffectChain> ;

TriggerAttr =
    "[priority=" <Integer> "]" ;
```

---

## Mapping GRS -> YAML completo

| Costruttore GRS | Campo YAML |
|---|---|
| `@meta game X` | `meta.game_id` |
| `@meta ns X` | `meta.namespace` |
| `Name :: ACTOR SELECTED_ACTOR` | `target.kind`, `target.selector` |
| `range ADJACENT_LOCATION` | `target.range_type: ADJACENT_LOCATION` |
| `required` | `target.required: true` |
| `no_self` | `target.allow_self: false` |
| `Name :: ACTOR_EXISTS(input.x)` | `condition.type: ACTOR_EXISTS`, `subject_id_ref: input.x` |
| `C_A AND C_B` | `condition.op: ALL_OF, children: [C_A, C_B]` |
| `C_A OR C_B` | `condition.op: ANY_OF, children: [C_A, C_B]` |
| `NOT C_A` | `condition.op: NOT, children: [C_A]` |
| `MOVE_ACTOR(Target_Piece, input.dest)` | `effect.type: MOVE_ACTOR`, `target: <Target_Piece>`, `value_ref: input.dest` |
| `[optional]` | `effect.optional: true` |
| `[stop]` | `effect.stop_on_failure: true` |
| `[continue]` | `effect.stop_on_failure: false` |
| `IF C ON T THEN E` | `rule.target`, `rule.conditions`, `rule.effects[0]` |
| `AND THEN E2` | `rule.effects[1]` |
| `[priority=200]` | `rule.priority: 200` |
| `[disabled]` | `rule.enabled: false` |
| `ONE_ONLY UNTIL_REMOVED` | `stacking_policy.mode`, `default_duration.type` |
| `ON_APPLY E` | `status.on_apply: [E]` |
| `ON_EVENT ACTOR_MOVED` | `trigger.type: ON_ACTOR_MOVED` |

---

## Regole di validazione semantica (lint)

Le seguenti condizioni producono errore di compilazione:

1. Un nome usato in `@rules` non è definito in `@targets`,
   `@conditions` o `@effects`.
2. Un `EffectCall` con tipo non nel vocabolario `EffectType`.
3. Un `CondAtom` con tipo non nel vocabolario `ConditionType`.
4. Un ref `input.xxx` usato in un `@trigger` — deve essere `event.xxx`.
5. Un ref `event.xxx` usato in una `@rule` standalone — deve essere
   `input.xxx`.
6. Argomento posizionale in più per un tipo che ne aspetta meno.
7. ID duplicato nella stessa categoria nello stesso file.
8. Ciclo diretto in condition composite (A -> B -> A).
9. `AND THEN` su effetto con `[stop]` se l'effetto prima ha `[continue]`
   è obbligatorio — warning, non errore.
10. Status referenziato in `APPLY_STATUS` non dichiarato in `@statuses`.
