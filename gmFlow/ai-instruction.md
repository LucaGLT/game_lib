L’idea è di creare un **motore di flusso di gioco**, non come “libreria per turni”.

Il rischio principale è creare classi troppo rigide tipo `Turn`, `Round`, `Phase`, `GameSession`, che poi funzionano solo per un gioco specifico.

Io la imposterei così:

# 1. Concetto base: separa “regole” da “orchestrazione”

La libreria non dovrebbe sapere cosa significa:

* pescare una carta;
* muovere una pedina;
* attaccare;
* comprare risorse;
* risolvere una quest;
* calcolare punti vittoria.

Per la maggior parte delle sue atività spoecifiche deve usare le lib già sviluppate per il gioco, che trovi descritte qui : 'Game-Lib_readme.md'

La libreria 'gmFlow' dovrebbe invece sapere gestire:

* chi può agire;
* quando può agire;
* in quale contesto;
* con quali step;
* se l’azione è valida;
* se l’azione è completata;
* se bisogna passare allo step/turno/round/fase successiva;
* se ci sono azioni asincrone o reazioni;
* se una sessione è terminata;
* se una campagna deve avanzare.

Quindi distinguerei nettamente:

```text
Game Engine / Flow Library
    gestisce tempi, turni, fasi, stato, eventi, validazione generica

Game Rules Plug-in
    implementa le regole specifiche del singolo gioco
```

La tua libreria 'gmFlow' dovrebbe essere una sorta di **framework di controllo del flusso**.

---

# 2. Entità principali

Io partirei da queste astrazioni.

## `GameSession`

Rappresenta una partita singola.

Contiene:

* lista giocatori;
* stato corrente;
* fase attuale;
* round attuale;
* turni in corso;
* storico eventi;
* coda di azioni;
* eventuali azioni pendenti;
* stato di completamento.

Esempio concettuale:

```cpp
class GameSession {
public:
    void start();
    void pause();
    void resume();
    void submitAction(const PlayerId& player, std::unique_ptr<IAction> action);
    void tick();

    bool isFinished() const;
    const GameState& state() const;

private:
    GameState gameState_;
    std::unique_ptr<IGameFlowController> flowController_;
    ActionQueue actionQueue_;
    EventBus eventBus_;
};
```

Il metodo `tick()` può essere utile se vuoi un motore che processa eventi/azioni in modo controllato.

---

## `Campaign`

Rappresenta una sequenza di più sessioni.

Non dovrebbe essere solo una lista di partite. Dovrebbe gestire:

* progressione;
* stato persistente;
* unlock;
* modifiche permanenti;
* risultati delle sessioni precedenti;
* condizioni per aprire la sessione successiva.

```cpp
class Campaign {
public:
    void startSession(SessionId id);
    void completeCurrentSession(const SessionResult& result);

    CampaignState& state();

private:
    CampaignState campaignState_;
    std::vector<SessionDefinition> sessions_;
};
```

La campagna dovrebbe stare sopra la sessione, non dentro.

---

## `GamePhase`

Una fase è un contenitore logico: setup, planning, action, combat, cleanup, scoring, endgame, ecc.

Ma non la renderei una classe concreta rigida. Meglio una interfaccia:

```cpp
class IPhase {
public:
    virtual ~IPhase() = default;

    virtual PhaseId id() const = 0;

    virtual void onEnter(GameContext& ctx) = 0;
    virtual void onExit(GameContext& ctx) = 0;

    virtual std::vector<std::unique_ptr<IAction>> availableActions(const GameContext& ctx,
                                                                   const PlayerId& player) const = 0;

    virtual bool isComplete(const GameContext& ctx) const = 0;
};
```

Ogni gioco potrà definire le sue fasi.

---

## `Round`

Il round è un ciclo che contiene uno o più turni, ma attenzione: non tutti i giochi hanno round classici.

Quindi lo terrei come componente opzionale del flusso:

```cpp
class Round {
public:
    RoundId id() const;
    int index() const;

private:
    RoundId id_;
    int index_;
};
```

La logica di avanzamento non la metterei in `Round`, ma nel `FlowController`.

---

## `Turn`

Il turno è il diritto temporaneo di uno o più giocatori di agire.

Per gestire anche i turni simultanei, eviterei di assumere che un turno appartenga sempre a un solo giocatore.

Meglio:

```cpp
class Turn {
public:
    TurnId id() const;

    const std::vector<PlayerId>& activePlayers() const;
    bool isPlayerActive(const PlayerId& player) const;

private:
    TurnId id_;
    std::vector<PlayerId> activePlayers_;
};
```

Così puoi avere:

```text
Turno singolo:
activePlayers = [PlayerA]

Turno simultaneo:
activePlayers = [PlayerA, PlayerB, PlayerC]

Turno di squadra:
activePlayers = [PlayerA, PlayerB]
```

---

## `Action`

L’azione è probabilmente il cuore della libreria.

Una buona azione dovrebbe avere almeno:

* identificativo;
* giocatore che la richiede;
* tipo;
* contesto;
* stato;
* validazione;
* esecuzione;
* eventuali step;
* eventi generati.

Eviterei azioni come semplici enum. Meglio un’interfaccia.

```cpp
class IAction {
public:
    virtual ~IAction() = default;

    virtual ActionId id() const = 0;
    virtual PlayerId owner() const = 0;

    virtual bool canExecute(const GameContext& ctx) const = 0;
    virtual ActionResult execute(GameContext& ctx) = 0;

    virtual bool isAsync() const = 0;
    virtual bool requiresTurn() const = 0;
};
```

Per esempio:

```text
MoveAction
DrawCardAction
AttackAction
PassAction
ReactAction
TradeAction
```

sono tutte implementazioni concrete lato gioco.

---

# 3. Azioni multi-step

Questa è una parte importante. In molti giochi una singola “azione” non è atomica.

Esempio:

```text
Azione: Attacca
    Step 1: scegli bersaglio
    Step 2: scegli arma/carta/modificatore
    Step 3: risolvi dado
    Step 4: applica danno
    Step 5: permetti reazioni avversarie
```

Quindi separerei:

```text
Action
    contiene una sequenza di ActionStep
```

Interfaccia:

```cpp
class IActionStep {
public:
    virtual ~IActionStep() = default;

    virtual StepId id() const = 0;
    virtual bool canEnter(const GameContext& ctx) const = 0;
    virtual StepResult execute(GameContext& ctx, const StepInput& input) = 0;
    virtual bool isComplete(const GameContext& ctx) const = 0;
};
```

Poi l’azione diventa una piccola macchina a stati:

```cpp
class StepBasedAction : public IAction {
public:
    ActionResult execute(GameContext& ctx) override;

private:
    std::vector<std::unique_ptr<IActionStep>> steps_;
    std::size_t currentStep_ = 0;
};
```

Questo ti permette di gestire bene:

* azioni interrotte;
* azioni sospese;
* input mancanti;
* conferme utente;
* reazioni avversarie;
* rollback parziale, se ti serve;
* salvataggio dello stato intermedio.

---

# 4. Flow Controller: il vero motore

Il componente più importante dovrebbe essere il `GameFlowController`.

Non dovrebbe contenere le regole specifiche del gioco, ma il modo in cui il gioco progredisce.

```cpp
class IGameFlowController {
public:
    virtual ~IGameFlowController() = default;

    virtual void start(GameContext& ctx) = 0;
    virtual void process(GameContext& ctx) = 0;

    virtual bool canPlayerAct(const GameContext& ctx, const PlayerId& player) const = 0;
    virtual void onActionCompleted(GameContext& ctx, const ActionResult& result) = 0;

    virtual bool isSessionComplete(const GameContext& ctx) const = 0;
};
```

Per giochi diversi puoi avere implementazioni diverse:

```text
SequentialTurnFlowController
SimultaneousTurnFlowController
PhaseBasedFlowController
RealTimeAsyncFlowController
CampaignScenarioFlowController
```

Potresti anche comporli:

```text
Session
 └── PhaseFlowController
      └── RoundFlowController
           └── TurnFlowController
                └── ActionFlowController
```

Ma attenzione: non partire subito troppo astratto.

---

# 5. Turni simultanei

Per i turni simultanei, non ragionerei come “più turni contemporanei”, ma come:

```text
una finestra di azione con più giocatori attivi
```

Esempio:

```cpp
class ActionWindow {
public:
    ActionWindowId id() const;

    bool isOpen() const;
    bool canPlayerSubmit(const PlayerId& player) const;

    void submit(const PlayerId& player, std::unique_ptr<IAction> action);
    bool isComplete(const GameContext& ctx) const;

private:
    std::vector<PlayerId> eligiblePlayers_;
    std::vector<SubmittedAction> submittedActions_;
    CompletionPolicy completionPolicy_;
};
```

La `CompletionPolicy` può essere:

```text
AllPlayersSubmitted
AnyPlayerSubmitted
TimeoutExpired
ManualClose
UntilNoMoreActions
PriorityOrderResolved
```

Questo ti serve per giochi dove:

* tutti scelgono una carta in segreto;
* tutti programmano azioni;
* alcuni possono reagire;
* c’è una fase di negoziazione;
* si aspetta il primo che risponde;
* il turno procede dopo che tutti hanno passato.

---

# 6. Azioni asincrone / fuori turno

Le azioni asincrone non le tratterei come eccezioni sporche al sistema dei turni. Le tratterei come **azioni dentro finestre di reazione**.

Esempio:

```text
Player A attacca
↓
Si apre ReactionWindow
↓
Player B può giocare una carta difensiva
Player C può intervenire
↓
La finestra si chiude
↓
L’attacco si risolve
```

Quindi ti serve un concetto di:

```cpp
class ReactionWindow {
public:
    bool isEligible(const PlayerId& player) const;
    bool accepts(const IAction& action) const;
    void submit(std::unique_ptr<IAction> action);
    void resolve(GameContext& ctx);
};
```

Più genericamente, io userei `ActionWindow`, di cui una `ReactionWindow` è un caso particolare.

Quindi:

```text
Turno normale = ActionWindow principale
Reazione = ActionWindow annidata
Azione fuori turno = ActionWindow aperta da evento
Azione estemporanea = ActionWindow globale o prioritaria
```

Questo ti evita di creare logiche separate per “azione normale” e “azione asincrona”.

---

# 7. Eventi

Ti consiglio fortemente un sistema a eventi.

Ogni azione dovrebbe produrre eventi, non modificare tutto in modo opaco.

Esempi:

```text
ActionSubmitted
ActionValidated
ActionStarted
ActionStepCompleted
ActionCompleted
ActionFailed
TurnStarted
TurnEnded
RoundStarted
RoundEnded
PhaseChanged
ResourceChanged
CardDrawn
PlayerDefeated
ReactionWindowOpened
ReactionWindowClosed
SessionCompleted
```

In C++17 puoi fare una cosa semplice:

```cpp
class IEvent {
public:
    virtual ~IEvent() = default;
    virtual EventType type() const = 0;
};

class EventBus {
public:
    void publish(std::unique_ptr<IEvent> event);
    void subscribe(EventType type, EventHandler handler);
};
```

Gli eventi servono per:

* UI;
* log partita;
* replay;
* undo/redo;
* salvataggio;
* debug;
* trigger di reazioni;
* trigger di achievement;
* sincronizzazione multiplayer.

---

# 8. Stato di gioco

Qui devi decidere una cosa importante.

Hai due modelli possibili.

## Modello A — Stato mutabile

Le azioni modificano direttamente `GameState`.

```cpp
ActionResult MoveAction::execute(GameContext& ctx) {
    ctx.state().movePawn(player_, from_, to_);
    return ActionResult::success();
}
```

È più semplice.

Pro:

* facile da implementare;
* naturale in C++;
* buono per single player/local app;
* più veloce.

Contro:

* undo/replay più difficili;
* test meno puliti;
* multiplayer più delicato.

---

## Modello B — Event sourcing

Le azioni non modificano direttamente lo stato, ma generano eventi/comandi.

```text
MoveAction
    → produce PawnMovedEvent
    → il reducer aggiorna GameState
```

Pro:

* ottimo per replay;
* ottimo per log;
* ottimo per multiplayer;
* facile salvare la storia;
* undo più gestibile.

Contro:

* più complesso;
* più verboso;
* richiede disciplina.

Per una libreria generica io valuterei almeno una via intermedia:

```text
Action → modifica stato + produce eventi
```

All’inizio va benissimo.

---

# 9. Validazione azioni

Distinguierei almeno tre livelli.

```text
1. Validazione di flusso
   Il giocatore può agire ora?

2. Validazione generica
   L’azione è ammessa nel contesto attuale?

3. Validazione specifica di gioco
   Secondo le regole del gioco, questa azione è legale?
```

Esempio:

```cpp
enum class ValidationError {
    None,
    NotPlayerTurn,
    PhaseDoesNotAllowAction,
    ActionWindowClosed,
    InvalidTarget,
    NotEnoughResources,
    RuleViolation
};
```

Risultato:

```cpp
class ValidationResult {
public:
    static ValidationResult ok();
    static ValidationResult fail(ValidationError error, std::string message);

    bool valid() const;
};
```

Questo è molto utile lato UI, perché puoi dire all’utente perché un’azione non è possibile.

---

# 10. Gestione priorità

Per azioni asincrone, simultanee e fuori turno, ti servirà una priorità.

Esempio:

```text
Priorità alta:
- annulla azione
- interrupt
- reazione obbligatoria

Priorità normale:
- azione di turno

Priorità bassa:
- effetti di fine fase
- cleanup
```

Potresti modellarla così:

```cpp
enum class ActionPriority {
    Immediate,
    Reaction,
    Normal,
    Deferred
};
```

E avere una coda:

```cpp
class ActionQueue {
public:
    void push(std::unique_ptr<IAction> action, ActionPriority priority);
    std::unique_ptr<IAction> popNext();

private:
    // priority queue o code separate
};
```

---

# 11. Stato delle azioni

Un’azione multi-step dovrebbe avere uno stato esplicito.

```cpp
enum class ActionStatus {
    Created,
    Submitted,
    Validating,
    WaitingForInput,
    WaitingForReaction,
    Executing,
    Completed,
    Failed,
    Cancelled,
    Suspended
};
```

Questo ti permette di gestire:

* azioni non ancora completate;
* UI che aspetta scelta utente;
* salvataggio nel mezzo di una scelta;
* rientro nell’app;
* multiplayer asincrono;
* reazioni fuori turno;
* errori;
* annullamento.

---

# 12. Non hardcodare il concetto di “giocatore”

Un giocatore può essere:

* umano locale;
* umano remoto;
* bot;
* master/game master;
* sistema;
* squadra;
* fazione;
* spettatore con poteri limitati.

Quindi userei qualcosa tipo:

```cpp
class Actor {
public:
    ActorId id() const;
    ActorType type() const;
};

enum class ActorType {
    Player,
    Bot,
    System,
    Team,
    GameMaster
};
```

Oppure mantieni `Player`, ma internamente prevedi già `SystemPlayer`.

Molte azioni di gioco sono generate dal sistema:

```text
inizio round
fine round
pesca automatica
danno da veleno
scadenza timer
evento campagna
```

Non sono “azioni del giocatore”, ma azioni del sistema.

---

# 13. Configurabilità

Per renderla generica, non mettere tutto in ereditarietà.

Usa anche oggetti di configurazione.

Esempio:

```cpp
struct TurnPolicy {
    bool allowSimultaneousTurns = false;
    bool allowAsyncActions = false;
    bool requireAllPlayersToPass = true;
};

struct RoundPolicy {
    bool enabled = true;
    int maxRounds = -1;
};

struct PhasePolicy {
    bool allowPhaseSkipping = false;
};
```

Però attenzione: troppe policy diventano ingestibili. Usale per comportamenti comuni, non per ogni regola specifica.

---

# 14. Strategia consigliata per il design

Io partirei da un nucleo minimale:

```text
GameSession
GameContext
GameState
IAction
IActionStep
IPhase
IGameFlowController
ActionWindow
ActionQueue
EventBus
```

Poi aggiungerei dopo:

```text
Campaign
Persistence
Replay
Undo/Redo
Multiplayer
AI/Bot
Timer
Rule scripting
```

Non partire subito dalla campagna. Prima fai funzionare bene:

```text
fase → round → turno → azione → step → evento → avanzamento
```

---

# 15. Possibile struttura dei namespace

```cpp
namespace boardflow {

    using PlayerId = std::string;
    using ActionId = std::string;
    using PhaseId  = std::string;
    using TurnId   = std::string;
    using RoundId  = std::string;

    class GameSession;
    class GameContext;
    class GameState;

    class IAction;
    class IActionStep;
    class IPhase;
    class IGameFlowController;

    class ActionWindow;
    class ActionQueue;
    class EventBus;

}
```

Oppure, se vuoi una struttura più seria:

```text
boardflow/
    core/
        ids.hpp
        result.hpp
        game_context.hpp
        game_state.hpp

    actions/
        action.hpp
        action_step.hpp
        action_queue.hpp
        action_result.hpp
        validation_result.hpp

    flow/
        phase.hpp
        turn.hpp
        round.hpp
        flow_controller.hpp
        action_window.hpp

    events/
        event.hpp
        event_bus.hpp

    campaign/
        campaign.hpp
        campaign_state.hpp

    persistence/
        serializer.hpp
```

---

# 16. API ideale lato gioco specifico

La libreria dovrebbe permettere a un gioco specifico di fare una cosa simile:

```cpp
class MyGameFlow : public boardflow::IGameFlowController {
public:
    void start(GameContext& ctx) override;
    void process(GameContext& ctx) override;
    bool canPlayerAct(const GameContext& ctx, const PlayerId& player) const override;
    void onActionCompleted(GameContext& ctx, const ActionResult& result) override;
    bool isSessionComplete(const GameContext& ctx) const override;
};
```

E poi:

```cpp
class MovePawnAction : public boardflow::IAction {
public:
    bool canExecute(const GameContext& ctx) const override;
    ActionResult execute(GameContext& ctx) override;
};
```

L’app finale fa:

```cpp
GameSession session;
session.start();

session.submitAction(player1, std::make_unique<MovePawnAction>(...));
session.tick();
```

---

# 17. Direttive pratiche C++17

## Usa ownership chiara

Per oggetti polimorfici:

```cpp
std::unique_ptr<IAction>
std::unique_ptr<IPhase>
std::unique_ptr<IActionStep>
```

Evita raw pointer proprietari.

---

## Evita template eccessivi all’inizio

La tentazione sarà fare tutto generico con template. Io eviterei.

Meglio interfacce virtuali chiare. C++17 va benissimo.

---

## Usa `std::variant` solo dove ha senso

Per eventi o risultati potresti usare:

```cpp
std::variant<ActionCompletedEvent, TurnStartedEvent, PhaseChangedEvent>
```

Però se vuoi estensibilità da parte dei giochi, il polimorfismo classico è più semplice.

---

## Mantieni il core indipendente dalla UI

La libreria non deve sapere nulla di:

* Qt;
* Android;
* iOS;
* Unity;
* Unreal;
* rendering;
* bottoni;
* animazioni.

Deve solo emettere eventi.

La UI ascolta gli eventi e aggiorna la vista.

---

## Pensa già alla serializzazione

Per un’app board game, il salvataggio è quasi obbligatorio.

Quindi evita oggetti impossibili da serializzare. Per esempio:

* non salvare lambda ovunque;
* non salvare puntatori a oggetti esterni;
* usa ID invece di riferimenti diretti;
* mantieni lo stato separato dal comportamento.

Esempio:

```cpp
struct ActionSnapshot {
    ActionId id;
    std::string type;
    PlayerId owner;
    ActionStatus status;
    std::size_t currentStep;
};
```

---

## Ogni azione deve essere testabile

Un buon test dovrebbe poter fare:

```cpp
GameState state = createInitialState();
MovePawnAction action(player, from, to);

auto result = action.execute(ctx);

ASSERT_TRUE(result.success());
ASSERT_EQ(state.positionOf(pawn), to);
```

Se un’azione richiede UI, rete, animazioni o database, è troppo accoppiata.

---

# 18. Domande necessarie

Per disegnarla bene, servono queste risposte.

## A. Obiettivo della libreria

1. Vuoi una libreria **solo per uso tuo**, oppure una libreria riutilizzabile anche da altri sviluppatori?

2. Deve essere un framework completo o una libreria leggera di supporto?

3. Vuoi che il gioco specifico venga implementato in C++ puro, oppure prevedi binding verso app/mobile/engine esterni?

4. L’app finale sarà desktop, mobile, web tramite backend, oppure cross-platform?

---

## B. Tipo di giochi da supportare

5. Hai già uno o più giochi target concreti su cui modellare la libreria?

6. Vuoi supportare giochi molto diversi tra loro, tipo:

   * eurogame;
   * deckbuilding;
   * dungeon crawler;
   * cooperativo;
   * legacy/campagna;
   * wargame;
   * party game;
   * giochi simultanei a programmazione azioni?

7. Il sistema deve supportare giochi con informazione nascosta? Per esempio carte in mano, ruoli segreti, obiettivi privati.

8. Deve supportare casualità? Dadi, mazzi, sacchetti, pesca casuale.

9. Deve supportare componenti fisici digitalizzati? Carte, plance, pedine, token, risorse, miniature.

---

## C. Turni e flusso

10. Il modello principale è:

* un giocatore alla volta;
* più giocatori contemporaneamente;
* tutti scelgono in simultanea e poi si risolve;
* tempo reale;
* misto?

11. I turni simultanei devono essere davvero paralleli o solo “tutti scelgono, poi il sistema risolve”?

12. Ti serve una gestione di timer/scadenze?

13. Le azioni fuori turno sono:

* reazioni a eventi;
* azioni libere sempre disponibili;
* interrupt;
* effetti automatici;
* azioni asincrone perché il giocatore non è online?

14. Vuoi supportare stack/priority simili a giochi di carte, dove più effetti si accumulano e poi si risolvono in ordine?

---

## D. Azioni

15. Le azioni devono essere atomiche o quasi sempre multi-step?

16. Gli step devono poter richiedere input dalla UI?

17. Un’azione può essere sospesa e ripresa più tardi?

18. Un’azione può essere annullata?

19. Ti serve undo/redo?

20. Ti serve replay completo della partita?

21. Le azioni devono poter generare nuove azioni automatiche?

Esempio:

```text
Giocatore attacca
→ il sistema genera tiro dado
→ il risultato genera danno
→ il danno genera morte unità
→ la morte unità genera effetto di fine turno
```

---

## E. Multiplayer

22. La libreria deve supportare multiplayer online?

23. Se sì: sincrono, asincrono, o entrambi?

24. Il motore gira:

* sul client;
* su un server autorevole;
* su entrambi;
* peer-to-peer?

25. Devi gestire disconnessioni?

26. Devi gestire riconciliazione stato client/server?

27. Devi impedire cheating lato client?

---

## F. Persistenza

28. Vuoi salvare e caricare una partita?

29. Vuoi salvare anche azioni pendenti e step intermedi?

30. Vuoi una serializzazione JSON?

31. Vuoi essere indipendente dalla libreria JSON, oppure puoi accettare una dipendenza tipo `nlohmann/json`?

32. La campagna deve salvare modifiche permanenti tra sessioni?

---

## G. Regole specifiche

33. Vuoi che le regole siano scritte in C++ compilato?

34. Oppure vuoi un sistema più data-driven, con JSON/YAML/script?

35. Vuoi permettere modding o giochi configurabili senza ricompilare?

36. Le condizioni di vittoria/fine partita devono essere plugin esterni?

37. Le fasi devono essere definite via codice o configurazione?

---

## H. Architettura interna

38. Preferisci un modello a:

* oggetti mutabili;
* eventi;
* command pattern;
* state machine;
* ECS;
* misto?

39. Vuoi minimizzare le dipendenze esterne?

40. Hai vincoli di performance particolari?

41. Hai vincoli embedded/mobile?

42. Vuoi compatibilità solo C++17 o puoi valutare C++20 in futuro?

---

## I. UI e integrazione

43. Che tipo di UI immagini?

44. La UI deve poter chiedere: “quali azioni sono disponibili per questo giocatore ora?”

45. La UI deve ricevere una lista di step/input richiesti?

46. Vuoi che la libreria produca descrizioni leggibili delle azioni per log e storico?

47. Vuoi supportare localizzazione/testi multilingua?

---

## J. Campagna

48. La campagna è solo una sequenza di partite o una struttura ramificata?

49. Le scelte di una sessione modificano le sessioni successive?

50. Vuoi gestire personaggi, progressione, inventario, achievement, unlock?

51. Una sessione può fallire ma la campagna prosegue?

52. Vuoi supportare salvataggi multipli della stessa campagna?

---

# 19. Scelta architetturale che ti suggerisco ora

Per partire, sceglierei questa impostazione:

```text
Core C++17
Polimorfismo tramite interfacce
GameState mutabile
ActionResult + EventBus
ActionQueue con priorità
ActionWindow per turni normali, simultanei e reazioni
FlowController pluggabile
Serializzazione prevista ma non implementata subito
Campagna come layer separato
```

Quindi il primo MVP dovrebbe supportare:

```text
1 sessione
N giocatori
fasi
round
turni singoli
azioni multi-step
eventi
azioni fuori turno tramite reaction window
```

Solo dopo aggiungerei:

```text
turni simultanei avanzati
multiplayer online
persistenza
campagna
replay
undo/redo
```

La classe chiave da progettare bene non è `Turn`, ma `ActionWindow`.

Perché con `ActionWindow` puoi modellare:

```text
turno classico
turno simultaneo
fase di scelta segreta
fase di reazione
azione fuori turno
interrupt
passaggio di priorità
```

Quindi io farei ruotare l’architettura attorno a questi tre concetti:

```text
GameFlowController
ActionWindow
IAction / IActionStep
```

Da lì il resto diventa componibile.
