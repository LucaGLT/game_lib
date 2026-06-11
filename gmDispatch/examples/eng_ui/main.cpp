/**
 * @file examples/eng_ui/main.cpp
 * @brief Demo della comunicazione CoreEngine ↔ UIController via gmDispatch.
 *
 * Architettura del ui_eng_bus
 * ─────────────────────
 *
 *  ┌─────────────┐   "ui.*"    ┌─────────────────┐
 *  │ UIController│ ──────────► │   CoreEngine     │
 *  │ (source="UI")│ ◄────────── │ (source="CoreEngine") │
 *  └─────────────┘  "eng.*"   └─────────────────┘
 *              targets={"UI"}         targets={"UI"}
 *
 *  PatternRouter: "ui.*" → CoreEngine,  "eng.*" → UIController
 *  Targeted delivery: CoreEngine risponde solo al canale "UI"
 *
 * Sequenza dimostrativa
 * ─────────────────────
 *  1. LoadGame("savegame_01.json")   → eng.loaded
 *  2. LoadGame("")                   → eng.error  (path vuoto)
 *  3. SaveGame prima di StartGame    → eng.error  (nessuna partita)
 *  4. StartGame(3 giocatori)         → eng.game_state
 *  5. GetGameState()                 → eng.game_state
 *  6. GetPlayerInfo("player_2")      → eng.player_info
 *  7. GetPlayerInfo("player_99")     → eng.error  (sconosciuto)
 *  8. SaveGame("savegame_02.json")   → eng.saved
 *
 * Build (dalla root game_lib):
 *   clang++ -std=c++17 -I. -D_CRT_SECURE_NO_WARNINGS          \
 *     gmDispatch/examples/eng_ui/main.cpp                      \
 *     gmDispatch/examples/eng_ui/CoreEngine.cpp                \
 *     gmDispatch/examples/eng_ui/UIController.cpp              \
 *     gmDispatch/Dispatcher.cpp                                \
 *     gmDispatch/DispatcherFactory.cpp                         \
 *     gmDispatch/channels/EventBusChannel.cpp                  \
 *     gmDispatch/serializers/JsonSerializer.cpp                \
 *     gmDispatch/routers/SyncRouter.cpp                        \
 *     gmDispatch/routers/PatternRouter.cpp                     \
 *     gmDispatch/dispatchers/SyncDispatcher.cpp                \
 *     -o eng_ui_example.exe && .\eng_ui_example.exe
 */

#include "CoreEngine.hpp"
#include "UIController.hpp"
#include "gmDispatch/DispatcherFactory.hpp"

#include <iostream>
#include <string>

// ── Helpers di stampa ─────────────────────────────────────────────────────────

static void printSeparator(const std::string& label)
{
    std::cout << "\n-- " << label << " ";
    for (int i = static_cast<int>(label.size()); i < 50; ++i) std::cout << '-';
    std::cout << "\n";
}

static void printResult(const UIController& ui)
{
    if (ui.lastLoaded) {
        std::cout << "  [Eng→UI] [OK] Loaded: " << ui.lastLoaded->filePath << "\n";
    }
    if (ui.lastSaved) {
        std::cout << "  [Eng→UI] [OK] Saved:  " << ui.lastSaved->filePath  << "\n";
    }
    if (ui.lastGameState) {
        const GameState& gs = *ui.lastGameState;
        std::cout << "  [Eng→UI] [OK] GameState:\n"
                  << "             round        = " << gs.round        << "\n"
                  << "             currentTurn  = " << gs.currentTurn  << "\n"
                  << "             deck         = " << gs.deckId       << "\n"
                  << "             currentEvent = " << gs.currentEvent << "\n"
                  << "             nextPlayer   = " << gs.nextPlayer   << "\n"
                  << "             prevPlayer   = "
                  << (gs.prevPlayer.empty() ? "NULL" : gs.prevPlayer)  << "\n";
    }
    if (ui.lastPlayerInfo) {
        const PlayerInfo& pi = *ui.lastPlayerInfo;
        std::cout << "  [Eng→UI] [OK] PlayerInfo:\n"
                  << "             id       = " << pi.playerId << "\n"
                  << "             name     = " << pi.name     << "\n"
                  << "             score    = " << pi.score    << "\n"
                  << "             active   = " << (pi.isActive ? "si" : "no") << "\n"
                  << "             hand     = [";
        for (std::size_t i = 0; i < pi.hand.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << pi.hand[i];
        }
        std::cout << "]\n";
    }
    if (ui.lastError) {
        std::cout << "  [Eng→UI] ✖ Error  (da " << ui.lastError->originTypeId
                  << "): " << ui.lastError->message << "\n";
    }
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "+----------------------------------------------+\n"
              << "|  CoreEngine <-> UIController via gmDispatch  |\n"
              << "+----------------------------------------------+\n";

    // PatternRouter richiesto per subscription "ui.*" e "eng.*"
    GmDispatch::Dispatcher ui_eng_bus =
        GmDispatch::DispatcherFactory::createPatternDispatcher("UI_Eng_GameBus");

    CoreEngine   engine(ui_eng_bus);
    UIController ui(ui_eng_bus);

    // ── 1. LoadGame valido ────────────────────────────────────────────────
    printSeparator("1. LoadGame(\"savegame_01.json\")");
    std::cout << "  [UI→Eng] sendLoadGame(\"savegame_01.json\")\n";
    ui.sendLoadGame("savegame_01.json");
    printResult(ui);

    // ── 2. LoadGame con path vuoto ────────────────────────────────────────
    printSeparator("2. LoadGame(\"\")  ← path vuoto");
    std::cout << "  [UI→Eng] sendLoadGame(\"\")\n";
    ui.sendLoadGame("");
    printResult(ui);

    // ── 3. SaveGame prima di StartGame ────────────────────────────────────
    printSeparator("3. SaveGame prima di StartGame  ← nessuna partita");
    std::cout << "  [UI→Eng] sendSaveGame(\"early_save.json\")\n";
    ui.sendSaveGame("early_save.json");
    printResult(ui);

    // ── 4. StartGame ──────────────────────────────────────────────────────
    printSeparator("4. StartGame([player_1, player_2, player_3])");
    std::cout << "  [UI→Eng] sendStartGame({\"player_1\",\"player_2\",\"player_3\"})\n";
    ui.sendStartGame({"player_1", "player_2", "player_3"});
    printResult(ui);

    // ── 5. GetGameState ───────────────────────────────────────────────────
    printSeparator("5. GetGameState()");
    std::cout << "  [UI→Eng] sendGetGameState()\n";
    ui.sendGetGameState();
    printResult(ui);

    // ── 6. GetPlayerInfo valido ───────────────────────────────────────────
    printSeparator("6. GetPlayerInfo(\"player_2\")");
    std::cout << "  [UI→Eng] sendGetPlayerInfo(\"player_2\")\n";
    ui.sendGetPlayerInfo("player_2");
    printResult(ui);

    // ── 7. GetPlayerInfo sconosciuto ──────────────────────────────────────
    printSeparator("7. GetPlayerInfo(\"player_99\")  ← sconosciuto");
    std::cout << "  [UI→Eng] sendGetPlayerInfo(\"player_99\")\n";
    ui.sendGetPlayerInfo("player_99");
    printResult(ui);

    // ── 8. SaveGame valido ────────────────────────────────────────────────
    printSeparator("8. SaveGame(\"savegame_02.json\")");
    std::cout << "  [UI→Eng] sendSaveGame(\"savegame_02.json\")\n";
    ui.sendSaveGame("savegame_02.json");
    printResult(ui);

    std::cout << "\n==============================================\n"
              << "  Fine demo\n"
              << "==============================================\n";
    return 0;
}
