#ifndef EXAMPLE_ENGUI_UICONTROLLER_HPP
#define EXAMPLE_ENGUI_UICONTROLLER_HPP

/**
 * @file examples/eng_ui/UIController.hpp
 * @brief Interfaccia utente che invia comandi al CoreEngine via gmDispatch.
 *
 * UIController si iscrive al pattern "eng.*" per ricevere tutte le risposte
 * del motore.  Ogni metodo `sendXxx()` dispatcha una richiesta e, grazie
 * al SyncDispatcher sincrono, al ritorno del metodo il campo `lastXxx` è
 * già popolato con la risposta (o `lastError` contiene l'errore).
 *
 * ### Flusso di una singola operazione (SyncDispatcher)
 * @code
 *   ui.sendLoadGame("save.json")
 *       → ui_eng_bus.dispatch(env{typeId="ui.load_game"})
 *           → PatternRouter → CoreEngine.onEnvelope()
 *               → handleLoadGame()
 *                   → ui_eng_bus.dispatch(env{typeId="eng.loaded"})
 *                       → PatternRouter → UIController.onEnvelope()
 *                           → lastLoaded = LoadedResponse{...}
 *       ← ritorna   (lastLoaded già valorizzato)
 * @endcode
 *
 * ### Canale e targeted delivery
 * Il canale della UI ha name() == "UI".
 * Il CoreEngine imposta targets = {"UI"} su ogni risposta,
 * così solo questo canale la riceve (grazie a PatternRouter).
 */

#include "Messages.hpp"
#include "gmDispatch/Dispatcher.hpp"
#include "gmDispatch/channels/EventBusChannel.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

class UIController {
public:
	/**
	 * @brief Costruisce il controller e si iscrive a "eng.*" sul ui_eng_bus fornito.
	 *
	 * @param ui_eng_bus GmDispatcher condiviso con il CoreEngine.
	 */
	explicit UIController(gmDispatch::GmDispatcher& ui_eng_bus);

	~UIController() = default;

	UIController(const UIController&)            = delete;
	UIController& operator=(const UIController&) = delete;

	// ═══════════════════════════════════════════════════════════════════════
	//  Comandi UI → CoreEngine
	// ═══════════════════════════════════════════════════════════════════════

	/** Chiede al motore di caricare il file di salvataggio indicato. */
	void sendLoadGame     (const std::string& filePath);

	/** Chiede al motore di salvare la sessione corrente nel file indicato. */
	void sendSaveGame     (const std::string& filePath);

	/** Avvia una nuova partita con la lista di giocatori fornita. */
	void sendStartGame    (const std::vector<std::string>& playerIds);

	/** Richiede lo stato corrente del gioco. */
	void sendGetGameState ();

	/** Richiede le informazioni sul giocatore con l'id indicato. */
	void sendGetPlayerInfo(const std::string& playerId);

	// ═══════════════════════════════════════════════════════════════════════
	//  Ultime risposte ricevute
	//  (validi dopo il corrispondente sendXxx(); reset all'inizio di ogni send)
	// ═══════════════════════════════════════════════════════════════════════

	std::optional<LoadedResponse>  lastLoaded;     ///< Set da eng.loaded
	std::optional<SavedResponse>   lastSaved;      ///< Set da eng.saved
	std::optional<GameState>       lastGameState;  ///< Set da eng.game_state
	std::optional<PlayerInfo>      lastPlayerInfo; ///< Set da eng.player_info
	std::optional<ErrorResponse>   lastError;      ///< Set da eng.error

private:
	void onEnvelope(const gmDispatch::Envelope& env);

	gmDispatch::GmDispatcher&                      _bus;
	std::shared_ptr<gmDispatch::EventBusChannel> channel_;
};

#endif // EXAMPLE_ENGUI_UICONTROLLER_HPP
