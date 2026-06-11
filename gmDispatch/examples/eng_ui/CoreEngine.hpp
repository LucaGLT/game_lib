#ifndef EXAMPLE_ENGUI_COREENGINE_HPP
#define EXAMPLE_ENGUI_COREENGINE_HPP

/**
 * @file examples/eng_ui/CoreEngine.hpp
 * @brief Motore di gioco che riceve comandi dalla UI tramite gmDispatch.
 *
 * CoreEngine si iscrive al pattern "ui.*" su un PatternRouter.
 * Per ogni richiesta ricevuta calcola la risposta e la rispedisce
 * con targets = { request.source } — la risposta arriva solo al mittente.
 *
 * ### Messaggi gestiti
 * | typeId               | Handler                  |
 * |----------------------|--------------------------|
 * | "ui.load_game"       | handleLoadGame()         |
 * | "ui.save_game"       | handleSaveGame()         |
 * | "ui.start_game"      | handleStartGame()        |
 * | "ui.get_game_state"  | handleGetGameState()     |
 * | "ui.get_player_info" | handleGetPlayerInfo()    |
 */

#include "Messages.hpp"
#include "gmDispatch/Dispatcher.hpp"
#include "gmDispatch/channels/EventBusChannel.hpp"

#include <map>
#include <memory>
#include <string>

class CoreEngine {
public:
    /**
     * @brief Costruisce il motore e si iscrive a "ui.*" sul ui_eng_bus fornito.
     *
     * @param ui_eng_bus Dispatcher condiviso con la UI.  Deve essere costruito
     *            con createPatternDispatcher() per supportare "ui.*".
     */
    explicit CoreEngine(GmDispatch::Dispatcher& ui_eng_bus);

    ~CoreEngine() = default;

    CoreEngine(const CoreEngine&)            = delete;
    CoreEngine& operator=(const CoreEngine&) = delete;

private:
    // ── Routing interno ───────────────────────────────────────────────────
    void onEnvelope        (const GmDispatch::Envelope& env);

    // ── Handler per ogni richiesta ────────────────────────────────────────
    void handleLoadGame     (const GmDispatch::Envelope& env);
    void handleSaveGame     (const GmDispatch::Envelope& env);
    void handleStartGame    (const GmDispatch::Envelope& env);
    void handleGetGameState (const GmDispatch::Envelope& env);
    void handleGetPlayerInfo(const GmDispatch::Envelope& env);

    // ── Helper di risposta ────────────────────────────────────────────────
    /** Imposta source="CoreEngine", targets={request.source}, poi dispatcha. */
    void reply(GmDispatch::Envelope& resp, const GmDispatch::Envelope& request);

    // ── Infrastruttura ────────────────────────────────────────────────────
    GmDispatch::Dispatcher&                      bus_;
    std::shared_ptr<GmDispatch::EventBusChannel> channel_;

    // ── Stato interno del gioco ───────────────────────────────────────────
    bool        gameStarted_{ false };
    std::string currentFile_;
    GameState   state_;
    std::map<std::string, PlayerInfo> players_;
};

#endif // EXAMPLE_ENGUI_COREENGINE_HPP
