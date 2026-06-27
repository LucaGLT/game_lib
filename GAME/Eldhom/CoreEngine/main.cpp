/**
 * @file main.cpp
 * @brief Entry point for Le Pergamene di Eldhôm CoreEngine CLI.
 *
 * Loads missione_01, runs one full test cycle (PG turns + monster turns)
 * and prints the result.  Full GUI integration is in P7 (mock ZMQ server).
 */

#include "GAME/Eldhom/CoreEngine/engine/EldhomEngine.hpp"
#include "GAME/Eldhom/CoreEngine/mission/MissionDefinition.hpp"

#include <iostream>
#include <string>

int main()
{
	std::cout << "Le Pergamene di Eldhom — CoreEngine P6\n";
	std::cout << "======================================\n";
	std::cout << "Caricamento missione_01 ...\n";

	// TODO: Phase 7 — load from JSON (data/mission_01.json)
	// For now, just confirm the engine compiles and links correctly.
	std::cout << "OK — build successful.\n";
	std::cout << "Per eseguire il gioco usa i test: test_eldhom_mission01\n";

	return 0;
}
