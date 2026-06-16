/**
 * @file examples/eng_ui/UIController.cpp
 * @brief Implementazione di UIController.
 */

#include "UIController.hpp"

// ── Costruttore ───────────────────────────────────────────────────────────────

UIController::UIController(gmDispatch::GmDispatcher& ui_eng_bus)
	: _bus(ui_eng_bus)
{
	// Canale nominato "UI" — il CoreEngine usa questo nome come target
	channel_ = std::make_shared<gmDispatch::EventBusChannel>("UI");

	channel_->add_handler([this](const gmDispatch::Envelope& env) {
		onEnvelope(env);
	});

	// PatternRouter: "eng.*" copre tutte le risposte del motore
	_bus.subscribe("eng.*", channel_);
}

// ── Handler risposte ──────────────────────────────────────────────────────────

void UIController::onEnvelope(const gmDispatch::Envelope& env)
{
	if (env.typeId == TypeId::ENG_LOADED) {
		lastLoaded    = std::any_cast<LoadedResponse>(env.payload);

	} else if (env.typeId == TypeId::ENG_SAVED) {
		lastSaved     = std::any_cast<SavedResponse>(env.payload);

	} else if (env.typeId == TypeId::ENG_GAME_STATE) {
		lastGameState = std::any_cast<GameState>(env.payload);

	} else if (env.typeId == TypeId::ENG_PLAYER_INFO) {
		lastPlayerInfo = std::any_cast<PlayerInfo>(env.payload);

	} else if (env.typeId == TypeId::ENG_ERROR) {
		lastError     = std::any_cast<ErrorResponse>(env.payload);
	}
}

// ═══════════════════════════════════════════════════════════════════════════
//  Comandi
// ═══════════════════════════════════════════════════════════════════════════

void UIController::sendLoadGame(const std::string& filePath)
{
	lastLoaded.reset();
	lastError.reset();

	gmDispatch::Envelope env;
	env.typeId  = TypeId::LOAD_GAME;
	env.source  = "UI";
	env.payload = LoadGameRequest{ filePath };
	_bus.dispatch(env);
}

void UIController::sendSaveGame(const std::string& filePath)
{
	lastSaved.reset();
	lastError.reset();

	gmDispatch::Envelope env;
	env.typeId  = TypeId::SAVE_GAME;
	env.source  = "UI";
	env.payload = SaveGameRequest{ filePath };
	_bus.dispatch(env);
}

void UIController::sendStartGame(const std::vector<std::string>& playerIds)
{
	lastGameState.reset();
	lastError.reset();

	gmDispatch::Envelope env;
	env.typeId  = TypeId::START_GAME;
	env.source  = "UI";
	env.payload = StartGameRequest{ playerIds };
	_bus.dispatch(env);
}

void UIController::sendGetGameState()
{
	lastGameState.reset();
	lastError.reset();

	gmDispatch::Envelope env;
	env.typeId  = TypeId::GET_GAME_STATE;
	env.source  = "UI";
	env.payload = GetGameStateRequest{};
	_bus.dispatch(env);
}

void UIController::sendGetPlayerInfo(const std::string& playerId)
{
	lastPlayerInfo.reset();
	lastError.reset();

	gmDispatch::Envelope env;
	env.typeId  = TypeId::GET_PLAYER_INFO;
	env.source  = "UI";
	env.payload = GetPlayerInfoRequest{ playerId };
	_bus.dispatch(env);
}
