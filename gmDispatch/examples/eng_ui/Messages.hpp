#ifndef EXAMPLE_ENGUI_MESSAGES_HPP
#define EXAMPLE_ENGUI_MESSAGES_HPP

/**
 * @file examples/eng_ui/Messages.hpp
 * @brief Tutti i typeId, le strutture di richiesta e risposta per la
 *        comunicazione CoreEngine ↔ UIController via gmDispatch.
 *
 * Convenzione dei typeId
 * ──────────────────────
 *  "ui.*"   — messaggi spediti dalla UI verso il CoreEngine
 *  "eng.*"  — risposte / eventi spediti dal CoreEngine verso la UI
 *
 *  UI subscribe a "eng.*"  (PatternRouter)
 *  Eng subscribe a "ui.*"  (PatternRouter)
 *
 * Targeted delivery
 * ─────────────────
 *  Il CoreEngine imposta sempre  Envelope::targets = { request.source }
 *  in modo che la risposta raggiunga solo il canale con name() == "UI"
 *  (o qualsiasi client abbia inviato la richiesta).
 */

#include <string>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════
//  TypeId constants
// ═══════════════════════════════════════════════════════════════════════════

namespace TypeId {

// UI → CoreEngine requests
constexpr char LOAD_GAME[]        = "ui.load_game";
constexpr char SAVE_GAME[]        = "ui.save_game";
constexpr char START_GAME[]       = "ui.start_game";
constexpr char GET_GAME_STATE[]   = "ui.get_game_state";
constexpr char GET_PLAYER_INFO[]  = "ui.get_player_info";

// CoreEngine → UI responses
constexpr char ENG_LOADED[]       = "eng.loaded";
constexpr char ENG_SAVED[]        = "eng.saved";
constexpr char ENG_ERROR[]        = "eng.error";
constexpr char ENG_GAME_STATE[]   = "eng.game_state";
constexpr char ENG_PLAYER_INFO[]  = "eng.player_info";

} // namespace TypeId

// ═══════════════════════════════════════════════════════════════════════════
//  Request payloads  (UI → CoreEngine)
// ═══════════════════════════════════════════════════════════════════════════

/** Chiede al motore di caricare una sessione dal file indicato. */
struct LoadGameRequest {
    std::string filePath;   ///< Percorso del file di salvataggio.
};

/** Chiede al motore di salvare la sessione corrente su file. */
struct SaveGameRequest {
    std::string filePath;   ///< Percorso di destinazione.
};

/** Chiede al motore di iniziare una nuova partita con la lista di giocatori. */
struct StartGameRequest {
    std::vector<std::string> playerIds;  ///< Es. {"player_1", "player_2", …}
};

/** Chiede lo stato corrente del gioco (nessun campo aggiuntivo). */
struct GetGameStateRequest {};

/** Chiede le informazioni di un giocatore specifico. */
struct GetPlayerInfoRequest {
    std::string playerId;   ///< Identificatore del giocatore.
};

// ═══════════════════════════════════════════════════════════════════════════
//  Response payloads  (CoreEngine → UI)
// ═══════════════════════════════════════════════════════════════════════════

/** Conferma che il file è stato caricato correttamente. */
struct LoadedResponse {
    std::string filePath;   ///< Path del file caricato.
};

/** Conferma che il salvataggio è andato a buon fine. */
struct SavedResponse {
    std::string filePath;   ///< Path del file salvato.
};

/**
 * @brief Risposta di errore generica.
 *
 * Inviata in risposta a qualsiasi richiesta che fallisce.
 */
struct ErrorResponse {
    std::string originTypeId;  ///< TypeId della richiesta che ha causato l'errore.
    std::string message;       ///< Descrizione leggibile dell'errore.
};

/**
 * @brief Stato completo del gioco.
 *
 * Inviato in risposta a StartGame e GetGameState.
 */
struct GameState {
    int         round        = 0;  ///< Numero del round corrente (1-based).
    std::string currentTurn;       ///< playerId del giocatore di turno.
    std::string deckId;            ///< Id del mazzo eventi attivo (es. "deck_1").
    std::string currentEvent;      ///< Id della carta evento corrente.
    std::string nextPlayer;        ///< playerId del prossimo giocatore.
    std::string prevPlayer;        ///< playerId del giocatore precedente; "" al primo turno.
};

/**
 * @brief Informazioni e stato di un singolo giocatore.
 *
 * Inviato in risposta a GetPlayerInfo.
 */
struct PlayerInfo {
    std::string              playerId;           ///< Identificatore univoco.
    std::string              name;               ///< Nome visualizzato.
    int                      score    = 0;       ///< Punteggio corrente.
    std::vector<std::string> hand;               ///< Id delle carte in mano.
    bool                     isActive = true;    ///< false = eliminato / fuori gioco.
};

#endif // EXAMPLE_ENGUI_MESSAGES_HPP
