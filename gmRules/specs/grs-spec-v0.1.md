# GRS — Game Rule Script v0.1

Language Specification for gmRules

---

## Obiettivo

GRS e un DSL testuale minimale e leggibile per descrivere regole di gioco
senza scrivere direttamente YAML o JSON.

Il testo GRS viene compilato in JSON/YAML validato contro
`gmRules/specs/game-rules.schema.json` e poi usato per
code generation verso gmRules.

---

## Principi di design

- Leggibile come prosa strutturata.
- Ogni blocco ha un tipo, nessun tipo e ripetuto riga per riga.
- Definizioni con `::` — distinguono dichiarazioni da valori.
- Keyword inglesi semplici: `ON`, `IF`, `THEN`, `AND`, `OR`, `NOT`,
  `AND THEN`.
- Parametri posizionali, senza nomi — l'ordine e fisso e documentato per tipo.
- Ref runtime espliciti per i valori non noti a compile time.
- Errore se un nome non e definito.

---

## Struttura documento

Un file `.grs` e composto da blocchi ordinati (l'ordine nei blocchi
e libero, l'ordine dei blocchi e consigliato ma non obbligatorio):

```
@meta       ... @end
@targets    ... @end
@conditions ... @end
@effects    ... @end
@statuses   ... @end
@rules      ... @end
@triggers   ... @end
```

Ogni riga dentro un blocco e una definizione oppure un attributo meta.
Il tipo dell'oggetto che si sta definendo e dato dal blocco: non si
ripete mai nella riga.

---

## Commenti

```
# Questo e un commento — ignorato dal parser
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

Argomenti posizionali:

1. `TargetKind` — `ACTOR`, `LOCATION`, `CARD`, `DECK`, `ITEM`, `NONE`
2. `TargetSelector` — `SELECTED_ACTOR`, `SELECTED_ALLY`, `SELECTED_ENEMY`,
   `SELF`, `ALL_ALLIES_IN_LOCATION`, `ALL_ENEMIES_IN_LOCATION`, ecc.
3. `range RangeType [N]` — opzionale, default `NONE`
4. `required` / `optional` — default `required`
5. `no_self` — abbr. per `allow_self: false`

Keyword per i tag:

- `needs TAG,TAG` — required_tags
- `forbids TAG,TAG` — forbidden_tags

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

Argomenti posizionali per tipo di effetto:

| Tipo | Arg1 | Arg2 |
|---|---|---|
| `MOVE_ACTOR` | target_ref | destination_ref |
| `DEAL_DAMAGE` | target_ref | amount |
| `HEAL` | target_ref | amount |
| `APPLY_STATUS` | target_ref | status_id |
| `REMOVE_STATUS` | target_ref | status_id |
| `ADD_TAG` | target_ref | tag |
| `REMOVE_TAG` | target_ref | tag |
| `DRAW_CARDS` | target_ref | deck_ref |
| `MOVE_CARD_TO_ZONE` | target_ref | zone_name |
| `EMIT_EVENT` | target_ref | event_name |
| `MANUAL_EFFECT` | — | event_name |

Modificatori (suffissi sulla riga):

- `[optional]` — fallimento diventa warning
- `[stop]` — stop_on_failure: true (default)
- `[continue]` — stop_on_failure: false

```grs
@effects
E_Move      :: MOVE_ACTOR(Target_Piece, input.destination)
E_Damage_1  :: DEAL_DAMAGE(Target_Enemy, 1)
E_Captured  :: APPLY_STATUS(Target_Enemy, captured)   [stop]
E_AddKing   :: ADD_TAG(Target_Piece, king)
E_LogMove   :: MANUAL_EFFECT(checkers.move.applied)   [optional]
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
    ON_APPLY     EffectRef [AND THEN EffectRef ...]
    ON_REMOVE    EffectRef
    ON_TURN_START  EffectRef
    ON_TURN_END  EffectRef
```

`StackingMode`: `REFRESH`, `ADD_STACK`, `IGNORE_NEW`, `REPLACE`,
`UNIQUE_BY_SOURCE`

`DurationType`: `PERMANENT`, `UNTIL_REMOVED`, `FOR_N amount N`,
`UNTIL_NEXT_TURN`, `WHILE_IN_LOCATION value V`

```grs
@statuses
captured :: IGNORE_NEW UNTIL_REMOVED
    ON_APPLY ADD_TAG(Target_Piece, captured)

poisoned :: REFRESH FOR_N amount 3
    ON_TURN_END DEAL_DAMAGE(Target_Self, 1) [optional]
    ON_REMOVE   REMOVE_TAG(Target_Self, poisoned)
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

`EventType`: `ACTION_SUBMITTED`, `ACTION_COMPLETED`, `CARD_PLAYED`,
`ACTOR_DAMAGED`, `ACTOR_MOVED`, `STATUS_APPLIED`, `TIME_REACHED`,
`LOCATION_ENTERED`

```grs
@triggers
T_PostMove_Cleanup [priority=100] ::
    ON_EVENT ACTION_COMPLETED
    THEN MANUAL_EFFECT(checkers.cleanup.captured) [optional]

T_PostMove_Promotion [priority=200] ::
    ON_EVENT ACTOR_MOVED
    IF ACTOR_EXISTS(event.actor_id) AND LOCATION_EXISTS(event.to_location)
    THEN MANUAL_EFFECT(checkers.trigger.promotion) [optional]
@end
```

---

## Runtime Refs

Le regole GRS (e gmRules) sono **definizioni statiche** — scritte una
volta sola nel file di regole.

Ma i valori su cui operano sono **dinamici** — dipendono da cosa
succede a runtime: quale pedina il giocatore ha scelto, in quale
casella vuole spostarla, quale attore ha subito danno.

I ref runtime sono **segnaposto nominati** che il runtime rimpiazza
con i valori reali nel momento in cui la regola viene eseguita.

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

Il generatore di codice traduce i `value_ref: input.destination`
in parametri della funzione factory o in un oggetto `RuntimeBindings`
passato al momento dell'invocazione.

---

## Grammatica EBNF v0.1

```ebnf
Document      ::= Block+

Block         ::= '@' BlockType Newline Body '@end' Newline

BlockType     ::= 'meta' | 'targets' | 'conditions' | 'effects'
                | 'rules' | 'statuses' | 'triggers'

Body          ::= Line+
Line          ::= MetaLine | DefLine | Newline

MetaLine      ::= Ident WS Value Newline

DefLine       ::= Ident AttrSuffix? WS '::' WS DefBody Newline
                  (WS SubLine Newline)*

SubLine       ::= HookKeyword WS EffectChain

HookKeyword   ::= 'ON_APPLY' | 'ON_REMOVE' | 'ON_TURN_START' | 'ON_TURN_END'

AttrSuffix    ::= '[' AttrList ']'
AttrList      ::= Attr (',' Attr)*
Attr          ::= 'priority' '=' Integer
                | 'disabled'

DefBody       ::= TargetBody | ConditionBody | EffectBody
                | RuleBody   | StatusBody    | TriggerBody

TargetBody    ::= TargetKind WS TargetSelector (WS TargetMod)*
TargetMod     ::= 'required' | 'optional' | 'no_self'
                | 'range' WS RangeType (WS Integer)?
                | 'needs'   WS TagList
                | 'forbids' WS TagList

ConditionBody ::= ConditionExpr
ConditionExpr ::= ConditionTerm (WS ('AND'|'OR') WS ConditionTerm)*
ConditionTerm ::= 'NOT' WS ConditionTerm
                | '(' WS ConditionExpr WS ')'
                | CondAtom
CondAtom      ::= CondType '(' ArgList? ')'
                | Ident

EffectBody    ::= EffectCall EffectMod*
EffectCall    ::= EffectType '(' ArgList? ')'
EffectMod     ::= '[' ('optional' | 'stop' | 'continue') ']'

RuleBody      ::= ('IF' WS ConditionExpr WS)? 'ON' WS Ident
                  WS 'THEN' WS EffectChain
                | 'ON' WS Ident (WS 'IF' WS ConditionExpr)?
                  WS 'THEN' WS EffectChain

EffectChain   ::= EffectEntry (WS 'AND' WS 'THEN' WS EffectEntry)*
EffectEntry   ::= EffectCall EffectMod*
                | Ident EffectMod*

StatusBody    ::= StackingMode WS DurationType (WS StatusDurAttr)*
StackingMode  ::= 'REFRESH' | 'ADD_STACK' | 'IGNORE_NEW'
                | 'REPLACE' | 'UNIQUE_BY_SOURCE'
DurationType  ::= 'PERMANENT' | 'UNTIL_REMOVED' | 'UNTIL_NEXT_TURN'
                | 'FOR_N' | 'WHILE_IN_LOCATION'
StatusDurAttr ::= 'amount' WS Integer | 'value' WS Ident

TriggerBody   ::= 'ON_EVENT' WS TriggerType
                  (WS 'IF' WS ConditionExpr)?
                  WS 'THEN' WS EffectChain

ArgList       ::= Arg (',' WS Arg)*
Arg           ::= Ref | StringLit | Integer
Ref           ::= ('input' | 'event' | 'source') '.' Ident
                | Ident

TagList       ::= Tag (',' WS Tag)*
Tag           ::= Ident

Ident         ::= [a-zA-Z_][a-zA-Z0-9_.]*
StringLit     ::= '"' [^"]* '"'
Integer       ::= '-'? [0-9]+
WS            ::= (' ' | '\t')+
Newline       ::= '\n' | '\r\n'
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
| `IGNORE_NEW UNTIL_REMOVED` | `stacking_policy.mode`, `default_duration.type` |
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
