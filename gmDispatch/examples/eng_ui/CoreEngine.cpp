/**
 * @file examples/eng_ui/CoreEngine.cpp
 * @brief Implementazione di CoreEngine.
 */

#include "CoreEngine.hpp"

#include <iostream>

// ── Costruttore ───────────────────────────────────────────────────────────────

CoreEngine::CoreEngine(gmDispatch::GmDispatcher& ui_eng_bus)
	: _bus(ui_eng_bus)
{
	// Canale nominato "CoreEngine" — la UI può usarlo come target
	channel_ = std::make_shared<gmDispatch::EventBusChannel>("CoreEngine");

	channel_->add_handler([this](const gmDispatch::Envelope& env) {
		onEnvelope(env);
	});

	// PatternRouter: "ui.*" copre tutti i comandi della UI
	_bus.subscribe("ui.*", channel_);

	std::cout << "[Eng] CoreEngine online — in ascolto su \"ui.*\"\n";
}

// ── Router interno ────────────────────────────────────────────────────────────

void CoreEngine::onEnvelope(const gmDispatch::Envelope& env)
{
	if      (env.typeId == TypeId::LOAD_GAME)       handleLoadGame(env);
	else if (env.typeId == TypeId::SAVE_GAME)        handleSaveGame(env);
	else if (env.typeId == TypeId::START_GAME)       handleStartGame(env);
	else if (env.typeId == TypeId::GET_GAME_STATE)   handleGetGameState(env);
	else if (env.typeId == TypeId::GET_PLAYER_INFO)  handleGetPlayerInfo(env);
	else {
		std::cout << "[Eng] Messaggio sconosciuto ignorato: " << env.typeId << "\n";
	}
}

// ── Helper risposta ───────────────────────────────────────────────────────────

void CoreEngine::reply(gmDispatch::Envelope& resp,
					   const gmDispatch::Envelope& request)
{
	resp.source  = "CoreEngine";
	resp.targets = { request.source };  // risponde solo al mittente
	_bus.dispatch(resp);
}

// ═══════════════════════════════════════════════════════════════════════════
//  Handler
// ═══════════════════════════════════════════════════════════════════════════

// ── ui.load_game ──────────────────────────────────────────────────────────────

void CoreEngine::handleLoadGame(const gmDispatch::Envelope& env)
{
	LoadGameRequest req = std::any_cast<LoadGameRequest>(env.payload);
	gmDispatch::Envelope resp;

	if (req.filePath.empty()) {
		resp.typeId  = TypeId::ENG_ERROR;
		resp.payload = ErrorResponse{ TypeId::LOAD_GAME,
									  "Percorso file vuoto" };
	} else {
		// Simulazione caricamento: accetta qualsiasi path non vuoto
		currentFile_ = req.filePath;
		std::cout << "[Eng] File caricato: " << currentFile_ << "\n";

		resp.typeId  = TypeId::ENG_LOADED;
		resp.payload = LoadedResponse{ req.filePath };
	}

	reply(resp, env);
}

// ── ui.save_game ──────────────────────────────────────────────────────────────

void CoreEngine::handleSaveGame(const gmDispatch::Envelope& env)
{
	SaveGameRequest req = std::any_cast<SaveGameRequest>(env.payload);
	gmDispatch::Envelope resp;

	if (!gameStarted_) {
		resp.typeId  = TypeId::ENG_ERROR;
		resp.payload = ErrorResponse{ TypeId::SAVE_GAME,
									  "Nessuna partita in corso" };
	} else {
		currentFile_ = req.filePath;
		std::cout << "[Eng] Partita salvata in: " << currentFile_ << "\n";

		resp.typeId  = TypeId::ENG_SAVED;
		resp.payload = SavedResponse{ req.filePath };
	}

	reply(resp, env);
}

// ── ui.start_game ─────────────────────────────────────────────────────────────

void CoreEngine::handleStartGame(const gmDispatch::Envelope& env)
{
	StartGameRequest req = std::any_cast<StartGameRequest>(env.payload);
	gmDispatch::Envelope resp;

	if (req.playerIds.empty()) {
		resp.typeId  = TypeId::ENG_ERROR;
		resp.payload = ErrorResponse{ TypeId::START_GAME,
									  "Lista giocatori vuota" };
		reply(resp, env);
		return;
	}

	// ── Inizializza i giocatori ───────────────────────────────────────────
	players_.clear();
	for (std::size_t i = 0; i < req.playerIds.size(); ++i) {
		const std::string& pid = req.playerIds[i];
		PlayerInfo pi;
		pi.playerId = pid;
		pi.name     = "Giocatore " + std::to_string(i + 1);
		pi.score    = 0;
		pi.hand     = { "card_A", "card_B", "card_C" };
		pi.isActive = true;
		players_[pid] = pi;
	}

	// ── Inizializza lo stato di gioco ─────────────────────────────────────
	state_.round        = 1;
	state_.currentTurn  = req.playerIds[0];
	state_.deckId       = "deck_1";
	state_.currentEvent = "event_card_5";
	state_.nextPlayer   = (req.playerIds.size() > 1)
							  ? req.playerIds[1]
							  : req.playerIds[0];
	state_.prevPlayer   = "";   // primo turno — nessun precedente

	gameStarted_ = true;
	std::cout << "[Eng] Nuova partita iniziata con "
			  << req.playerIds.size() << " giocatori\n";

	resp.typeId  = TypeId::ENG_GAME_STATE;
	resp.payload = state_;
	reply(resp, env);
}

// ── ui.get_game_state ─────────────────────────────────────────────────────────

void CoreEngine::handleGetGameState(const gmDispatch::Envelope& env)
{
	gmDispatch::Envelope resp;

	if (!gameStarted_) {
		resp.typeId  = TypeId::ENG_ERROR;
		resp.payload = ErrorResponse{ TypeId::GET_GAME_STATE,
									  "Nessuna partita in corso" };
	} else {
		resp.typeId  = TypeId::ENG_GAME_STATE;
		resp.payload = state_;
	}

	reply(resp, env);
}

// ── ui.get_player_info ────────────────────────────────────────────────────────

void CoreEngine::handleGetPlayerInfo(const gmDispatch::Envelope& env)
{
	GetPlayerInfoRequest req = std::any_cast<GetPlayerInfoRequest>(env.payload);
	gmDispatch::Envelope resp;

	std::map<std::string, PlayerInfo>::iterator it = players_.find(req.playerId);
	if (it == players_.end()) {
		resp.typeId  = TypeId::ENG_ERROR;
		resp.payload = ErrorResponse{ TypeId::GET_PLAYER_INFO,
									  "Giocatore sconosciuto: " + req.playerId };
	} else {
		resp.typeId  = TypeId::ENG_PLAYER_INFO;
		resp.payload = it->second;
	}

	reply(resp, env);
}
