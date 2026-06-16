# Guida GRS — Dungeon Crawler a Turni con Carte

Riferimento: `turn-card-dungeon.example.grs`

---

## Introduzione

Questo documento insegna a scrivere regole in formato GRS
partendo da una frase in linguaggio naturale.

Il caso d'uso è:

> Ad ogni turno il giocatore deve usare **una sola azione** e deve decidere
> se usare un'**Azione Base** (Movimento Base o Attacco Base) oppure
> mettere in gioco una **Carta** della sua Mano.
> Ogni Carta ha una sua regola peculiare scritta sulla carta stessa.

---

## 1 — Come si decompone una regola in linguaggio umano

### Il metodo: domande guida

Quando hai una regola scritta in italiano, poniti queste 5 domande:

| Domanda | Risposta nel testo | Traduzione GRS |
|---|---|---|
| Chi agisce? | il giocatore / l'eroe | source actor → `input.hero_id` |
| Quando? | ad ogni turno, una volta sola | gate via status `action_used` |
| Cosa può fare? | 3 scelte: Move, Attack, Play Card | 3 famiglie di `@rules` |
| Su chi/cosa agisce? | stanza adiacente, nemico adiacente, carta in mano | `@targets` + `@conditions` |
| Cosa succede? | si sposta / infligge danno / applica effetto carta | `@effects` |

### Applicazione al testo

```
"Ad ogni turno il giocatore deve usare UNA SOLA azione"
```

Frase chiave: **una sola**. Questo è un **vincolo di frequenza**.

Non esiste un effetto GRS che limiti direttamente le azioni per turno.
La soluzione è: usare uno **status come gate**:

- All'inizio del turno: nessuno status attivo.
- Quando l'eroe esegue qualsiasi azione: applica status `action_used`.
- La condizione `C_ActionAvailable` controlla `NOT ACTOR_HAS_STATUS(input.hero_id, action_used)`.
- Un trigger `T_BlockDoubleAction` intercetta tentativi di seconda azione.
- Lo status `action_used` ha durata `UNTIL_NEXT_TURN`: si azzera automaticamente.

```
"decidere se usare un'Azione Base ... oppure giocare una Carta"
```

Questa è una **biforcazione di categoria**, non una condizione singola.
Significa che nel GRS avremo due gruppi di regole distinti,
entrambi vincolati da `C_ActionAvailable`.

```
"Ogni Carta ha la sua regola peculiare scritta sulla carta stessa"
```

Ogni carta è una **regola GRS separata** con ID proprio (`Card_Fireball`,
`Card_Shield`, ecc.). Il meccanismo comune è la condizione `C_CanPlayCard`
che verifica che la carta sia in mano.

---

## 2 — Cos'è il Target di una Rule e come si sceglie

### Definizione

Il **target** risponde alla domanda: *su cosa agisce l'effetto?*

In GRS il target è dichiarato in `@targets` con:

```
NomeFiltro :: TargetKind TargetSelector [range] [required|optional] [no_self]
```

Il nome è poi riusato negli `@effects` e nelle `@rules`.

### Regola pratica: scegli dal più restrittivo

Parti dalla domanda: l'effetto deve colpire un singolo attore specifico,
un gruppo, o nessuno?

| Situazione | TargetSelector consigliato |
|---|---|
| Un singolo nemico (scelto dal giocatore) | `SELECTED_ENEMY` |
| Un singolo alleato (scelto dal giocatore) | `SELECTED_ALLY` |
| Se stesso | `SELF` |
| Tutti i nemici nella stanza | `ALL_ENEMIES_IN_LOCATION` |
| Tutti gli alleati nella stanza | `ALL_ALLIES_IN_LOCATION` |
| Un qualsiasi attore selezionato | `SELECTED_ACTOR` |
| Nessun attore (log, gate, notifica) | `NONE MANUAL` |

### Range: aggiungi solo se serve

Il range filtra per prossimità. Aggiungilo solo se la regola
ha un requisito geografico esplicito:

- Attacco base: `range ADJACENT_LOCATION` — devi essere vicino.
- Quick Strike (carta): nessun range — colpisci chiunque nel dungeon.
- Cure (carta): nessun range — curi te stesso, non serve distanza.

### required vs optional

- `required`: se nessun target valido viene trovato, la regola fallisce e **si ferma**.
- `optional`: se nessun target valido, la regola **prosegue** (zero target è ok).

Usa `optional` per effetti AoE (area di effetto) dove è normale che
ci siano zero nemici nella stanza.

```grs
@targets
# Fireball: colpisci TUTTI i nemici nella stanza. Zero nemici = ok.
Target_AllEnemies :: ACTOR ALL_ENEMIES_IN_LOCATION range SAME_LOCATION optional

# Attacco base: deve esserci un nemico adiacente. Zero = errore.
Target_Enemy :: ACTOR SELECTED_ENEMY range ADJACENT_LOCATION required
@end
```

### no_self

`no_self` impedisce all'eroe di colpire se stesso con un effetto
anche se il selector lo permetterebbe.
Usalo su `Target_Ally` per evitare che l'eroe si "curi da solo"
usando una carta pensata per gli alleati:

```grs
Target_Ally :: ACTOR SELECTED_ALLY range SAME_LOCATION optional no_self
```

---

## 3 — Cosa sono i Trigger e quando usarli

### Regole vs Trigger: la differenza fondamentale

| | `@rules` | `@triggers` |
|---|---|---|
| Chi lo attiva? | Il giocatore (scelta esplicita) | Il sistema (automaticamente) |
| Quando scatta? | Quando il giocatore lo invoca | Quando accade un evento di gioco |
| Input ref | `input.xxx` | `event.xxx` |
| Esempio | "Voglio attaccare quel nemico" | "Un attore si è mosso" |

### Quando usare i trigger

Usa i trigger per:

1. **Gate automatici**: bloccare azioni non consentite prima dell'esecuzione.
2. **Reazioni passate**: effetti che scattano senza che il giocatore li scelga.
3. **Cleanup di fine turno**: rimuovere status temporanei.
4. **Notifiche di sistema**: aggiornare la UI, logare eventi, cambiare fase.

### Il trigger più importante in questo esempio

```grs
T_BlockDoubleAction [priority=5] ::
    ON_EVENT ACTION_SUBMITTED
    IF ACTOR_HAS_STATUS(event.actor_id, action_used)
    THEN E_ActionBlocked?
```

`ACTION_SUBMITTED` scatta **prima** che la regola venga eseguita.
Priorità 5 assicura che questo trigger venga valutato prima di qualsiasi
altra reazione. Se lo status `action_used` è presente, il game loop
riceve l'evento `dungeon.turn.action_blocked` e può annullare l'azione.

### Tabella eventi e uso tipico

| EventType | Quando scatta | Uso tipico in dungeon crawler |
|---|---|---|
| `ACTION_SUBMITTED` | Prima dell'esecuzione | Blocchi, intercettazioni |
| `ACTION_COMPLETED` | Dopo esecuzione | Cleanup, notifiche fine turno |
| `ACTOR_MOVED` | Dopo spostamento | Promozioni, trappole attivate dopo |
| `LOCATION_ENTERED` | All'ingresso (immediato) | Trappole, trigger geografici |
| `ACTOR_DAMAGED` | Dopo aver subito danno | HP basso, rage, morte |
| `STATUS_APPLIED` | Dopo status applicato | Log, UI, reaction chain |
| `CARD_PLAYED` | Dopo carta giocata | Hook per abilità passive |
| `TIME_REACHED` | A tick preciso | Fine round, fase timer |

---

## 4 — Come gestire al meglio le Conditions

### Principio: una condizione = una domanda

Ogni condizione deve rispondere a **una sola** domanda booleana.
Condizioni composte si costruiscono combinando condizioni semplici.

### Pattern: gerarchia di condizioni

Costruisci le condizioni dal basso verso l'alto:

```grs
@conditions
# Livello 1: check atomici
C_HeroAlive       :: ACTOR_HP_AT_OR_ABOVE(input.hero_id, 1)
C_HeroNotStunned  :: NOT ACTOR_HAS_TAG(input.hero_id, stunned)
C_ActionAvailable :: NOT ACTOR_HAS_STATUS(input.hero_id, action_used)

# Livello 2: condizione aggregata "l'eroe può agire"
C_HeroCanAct :: C_HeroAlive AND C_HeroNotStunned AND C_ActionAvailable

# Livello 3: condizioni specifiche per azione
C_CanBaseMove   :: C_HeroCanAct AND C_DestinationValid
C_CanBaseAttack :: C_HeroCanAct AND C_EnemyExists AND C_EnemyAdjacent
C_CanPlayCard   :: C_HeroCanAct AND C_CardInHand
```

**Vantaggio**: se la logica di `C_HeroCanAct` cambia (es. aggiunta di un
nuovo stato "paralizzato"), la modifica è in un solo posto.

### Quando usare condizioni inline nelle rules

Le condizioni inline nelle `@rules` sono utili per condizioni
**specifiche di quella carta** che non verranno riusate altrove:

```grs
Card_ArcaneBolt [priority=200] ::
    IF C_CanPlayCard AND C_EnemyVisible AND C_EnoughMana
    ON Target_Hero
    THEN ...
```

`C_EnoughMana` è usata solo da `Card_ArcaneBolt`: ha senso come
condizione nominata solo se più carte richiedono mana.
Se fosse usata da una sola carta, poteva stare inline:

```grs
    IF C_CanPlayCard AND C_EnemyVisible AND RESOURCE_AT_LEAST(input.hero_id, 2)
```

### NOT: attenzione all'ordine

`NOT` lega solo il termine immediato successivo:

```grs
# Corretto: NOT solo su C_HeroHasWeapon
IF C_HeroAlive AND NOT C_HeroHasWeapon

# Corretto: NOT su gruppo con parentesi
IF C_HeroAlive AND NOT (C_HeroHasWeapon OR C_HeroHasShield)
```

---

## 5 — Come gestire al meglio gli Effects

### Principio: ogni effect fa una cosa sola

Un effetto in GRS è **atomico**. La sequenza si ottiene con `AND THEN`.

### Pattern: separare azione da gate da log

Per ogni regola, la catena di effetti segue questo schema:

```
THEN <Effetto principale>
AND THEN <Gate azione>       # sempre E_MarkActionUsed
AND THEN <Scarta carta>?     # solo per carte
AND THEN <Log/notifica>?     # opzionale
```

Esempio per `Card_Fireball`:

```grs
THEN E_FireballDamage      # [stop] — se fallisce, tutto si ferma
AND THEN E_DiscardPlayedCard   # [optional] — se non si può scartare, continua
AND THEN E_MarkActionUsed  # [stop] — essenziale: segna azione usata
AND THEN E_FireballLog?    # [optional] — puro log
```

### Modificatori: quando usarli

| Modificatore | Quando usarlo |
|---|---|
| (nessuno = `[stop]`) | Effetti essenziali: se falliscono, annulla tutto |
| `[optional]` | Effetti secondari: log, notify, cleanup |
| `[continue]` | Raro — prosegui anche su errore grave |

Regola pratica:
- L'effetto che fa il "danno vero" o il "movimento vero": nessun modificatore.
- Lo scarto della carta: `[optional]` (se qualcosa va storto con il deck, la regola non deve fallire).
- I log: sempre `[optional]`.
- `E_MarkActionUsed`: `[stop]` — se non riesci a segnare l'azione usata, c'è un bug grave.

### Riuso degli effetti

Gli effetti sono riusabili tra regole diverse.
`E_MarkActionUsed` e `E_DiscardPlayedCard` compaiono in ogni regola carta:
definiti una sola volta, riusati ovunque.

---

## 6 — Come gestire al meglio gli Statuses

### Uno status = memoria persistente su un attore

Lo status non fa nulla da solo: è un **flag** che altri sistemi leggono.
Il suo valore è nella combinazione di:

- quando viene applicato (ON_APPLY hooks)
- quanto dura (DurationType)
- come si comporta se applicato di nuovo (StackingMode)

### Il gate azione: caso pratico

Il problema centrale di questo dungeon crawler è:
**come impedire due azioni per turno?**

La soluzione GRS:

```grs
action_used :: ONE_ONLY UNTIL_NEXT_TURN
    ON_APPLY  ADD_TAG(Target_Self, action_spent)
    ON_REMOVE REMOVE_TAG(Target_Self, action_spent)
```

- `ONE_ONLY`: se qualcuno tenta di applicarlo due volte, viene ignorato.
- `UNTIL_NEXT_TURN`: si rimuove automaticamente all'inizio del turno successivo.
- `ON_APPLY ADD_TAG(action_spent)`: aggiunge un tag visibile alla UI.
- `ON_REMOVE REMOVE_TAG(action_spent)`: pulisce il tag quando il turno finisce.

### Matrice di scelta StackingMode

| Scenario | StackingMode |
|---|---|
| Status unico, non si accumula, non si rinnova | `ONE_ONLY` |
| Status che si rinnova (durata riparte) | `REFRESH` |
| Status che si intensifica (più stack = più effetto) | `ADD_STACK` |
| Status che deve essere completamente sostituito | `REPLACE` |
| Status applicabile da più sorgenti indipendenti | `UNIQUE_BY_SOURCE` |

### Matrice di scelta DurationType

| Scenario | DurationType |
|---|---|
| Dura un turno e poi sparisce | `UNTIL_NEXT_TURN` |
| Dura N turni dell'attore | `FOR_N amount N` |
| Dura finché non viene rimosso esplicitamente | `UNTIL_REMOVED` |
| Non scade mai | `PERMANENT` |
| Dura solo in una location specifica | `WHILE_IN_LOCATION value V` |

---

## 7 — Quando e come usare le Runtime Refs

### Il problema senza runtime refs

Immagina di dover scrivere una regola per OGNI combinazione
(eroe + nemico + carta). Sarebbe impossibile:

```
# IMPOSSIBILE — combinatoria esplosiva
Card_Fireball_Hero1_Enemy2  :: ...
Card_Fireball_Hero1_Enemy3  :: ...
Card_Fireball_Hero2_Enemy2  :: ...
```

Le runtime refs risolvono questo: la regola è scritta **una volta sola**
con segnaposti, e il runtime li riempie al momento dell'esecuzione.

### I tre namespace

```
input.xxx   — valori forniti DAL CHIAMANTE (giocatore, AI, UI)
event.xxx   — valori provenienti DALL'EVENTO che ha attivato il trigger
source.xxx  — attributi dell'attore sorgente dell'effetto
```

### Quando usare `input.xxx`

In `@rules`, `@conditions` e `@effects`:

- `input.hero_id` — quale eroe sta agendo
- `input.enemy_id` — quale nemico è stato selezionato
- `input.destination` — in quale casella vuole spostarsi
- `input.card_id` — quale carta sta giocando

```grs
@conditions
C_CanBaseMove :: C_HeroCanAct AND LOCATION_EXISTS(input.destination)
#                                                  ^^^^^^^^^^^^^^^^
#                              La destinazione non è nota a compile time:
#                              dipende da dove il giocatore clicca.
```

### Quando usare `event.xxx`

In `@triggers` **soltanto**. I trigger reagiscono a eventi del sistema:

```grs
T_HeroLowHp [priority=200] ::
    ON_EVENT ACTOR_DAMAGED
    IF ACTOR_HP_AT_OR_BELOW(event.actor_id, 3)
    #                        ^^^^^^^^^^^^^^
   #  L'attore che ha subito danno è noto solo a runtime,
    #  quando l'evento viene emesso.
    THEN MANUAL_EFFECT(dungeon.hero.critical_hp) [optional]
```

**Attenzione**: `input.xxx` in un `@trigger` è un errore semantico.
Usa sempre `event.xxx` nei trigger.

### Tabella dei campi evento disponibili

| Campo | Disponibile per |
|---|---|
| `event.actor_id` | tutti gli eventi |
| `event.target_id` | CARD_PLAYED, ACTOR_DAMAGED |
| `event.from_location` | ACTOR_MOVED, LOCATION_ENTERED |
| `event.to_location` | ACTOR_MOVED, LOCATION_ENTERED |
| `event.source_id` | ACTOR_DAMAGED, STATUS_APPLIED |
| `event.status_id` | STATUS_APPLIED |
| `event.amount` | ACTOR_DAMAGED |

### Come il runtime risolve i refs: sequenza completa

Quando il giocatore sceglie di giocare la carta Fireball sul nemico goblin,
il game loop fa:

```
1. Giocatore seleziona carta "Fireball" dalla mano
   -> input.card_id = "card_fireball_001"

2. Giocatore seleziona l'azione Card_Fireball
   -> regola: Card_Fireball

3. Regola richiede la valutazione di C_CanPlayCard:
   -> C_HeroCanAct: verifica hero_id, status, tag
   -> C_CardInHand: CARD_IN_ZONE(input.card_id, hand)
                                 ^^^^^^^^^^^^^
                   Risolto: CARD_IN_ZONE("card_fireball_001", hand) -> true

4. Target: Target_AllEnemies (tutti nemici nella stanza)
   -> Risolto: [goblin_01, goblin_02]

5. Effetto E_FireballDamage: DEAL_DAMAGE(Target_AllEnemies, 3)
   -> Applicato a goblin_01: -3 HP
   -> Applicato a goblin_02: -3 HP

6. Effetto E_DiscardPlayedCard: MOVE_CARD_TO_ZONE(Target_PlayedCard, discard)
   -> Target_PlayedCard risolto dal selected target passato dal chiamante
   -> "card_fireball_001" -> zona discard

7. Effetto E_MarkActionUsed: APPLY_STATUS(Target_Self, action_used)
   -> action_used applicato all'eroe

8. Trigger T_TurnEndNotify scatta su ACTION_COMPLETED
   -> event.actor_id = "hero_01"
   -> ACTOR_HAS_STATUS("hero_01", action_used) -> true
   -> E_TurnEndLog emesso
```

### Tabella riassuntiva ref per blocco

| Blocco | Ref usabili | Ref vietati |
|---|---|---|
| `@conditions` | `input.xxx`, `source.xxx` | `event.xxx` |
| `@effects` | `input.xxx`, `source.xxx` | `event.xxx` |
| `@rules` | `input.xxx`, `source.xxx` | `event.xxx` |
| `@triggers IF` | `event.xxx` | `input.xxx` |
| `@triggers THEN` | `event.xxx` come source | `input.xxx` |
| `@statuses` hook | `source.xxx` | `input.xxx`, `event.xxx` |

---

## Riepilogo: checklist per scrivere una nuova regola in GRS

Prima di scrivere, rispondi a queste domande:

- [ ] Chi esegue l'azione? (→ definisci `input.hero_id` o `input.actor_id`)
- [ ] C'è un vincolo di frequenza? (→ status gate come `action_used`)
- [ ] Su chi agisce? (→ scegli o crea un `@targets`)
- [ ] Quali precondizioni devono essere vere? (→ scrivi `@conditions` dal basso)
- [ ] Cosa succede esattamente? (→ uno `@effects` atomico per ogni passo)
- [ ] Ci sono reazioni automatiche? (→ scrivi un `@triggers` con `ON_EVENT`)
- [ ] I valori cambiano a runtime? (→ usa `input.xxx` nelle rules, `event.xxx` nei trigger)
- [ ] Lo stato deve essere ricordato tra un turno e l'altro? (→ crea uno `@statuses`)
