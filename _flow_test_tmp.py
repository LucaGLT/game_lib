import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, r"c:\_GLT_\Qt Prj\game_lib\pyLib")

from PySide6.QtWidgets import QApplication

from gmGui.modules.gm_flow_module import GmFlowModule

app = QApplication([])
m = GmFlowModule()
m.widget()

print("=== TEST LOGIC ===\n")

# Session 1
print("Session 1 started")
m.on_envelope({"typeId": "gmFlow.session.started", "data": {"session_id": "tris"}})
print(f"  Session counter: {m._lbl_session.text()}")
print(f"  Round label: {m._lbl_round.text()}")
print(f"  Turn label: {m._lbl_turn.text()}")

# Round 1
print("\nRound 1 started")
m.on_envelope({"typeId": "gmFlow.round.started", "data": {}})
print(f"  Round label: {m._lbl_round.text()}")

# Turns: X, O, X, O, X (5 turni in Tris)
print("\nTurns:")
for i in range(5):
    actor = "PLAYER_X" if i % 2 == 0 else "PLAYER_O"
    actor_short = "X" if i % 2 == 0 else "O"
    m.on_envelope({"typeId": "gmFlow.turn.started", 
                   "data": {"turn_id": actor, "active_actors": [actor_short]}})
    print(f"  Turn {i+1}: {actor_short}")
    print(f"    Label: {m._lbl_turn.text()}")
    print(f"    Turn actors list: {m._turn_actors}")

print(f"\nSession 1 final state:")
print(f"  Session: {m._session_count}")
print(f"  Round: {m._round_count}")
print(f"  Turn: {m._turn_count}")
print(f"  Turn actors: {m._turn_actors}")

# Session 2
print("\n\nSession 2 started")
m.on_envelope({"typeId": "gmFlow.session.started", "data": {"session_id": "tris"}})
print(f"  Session counter: {m._lbl_session.text()}")
print(f"  Round label: {m._lbl_round.text()}")
print(f"  Turn label: {m._lbl_turn.text()}")
print(f"  Turn actors reset: {m._turn_actors}")

# Round 1 (nuova partita)
print("\nRound 1 started (partita 2)")
m.on_envelope({"typeId": "gmFlow.round.started", "data": {}})
print(f"  Round label: {m._lbl_round.text()}")

# Turni: X, O, X (3 turni)
print("\nTurns (partita 2):")
for i in range(3):
    actor = "PLAYER_X" if i % 2 == 0 else "PLAYER_O"
    actor_short = "X" if i % 2 == 0 else "O"
    m.on_envelope({"typeId": "gmFlow.turn.started", 
                   "data": {"turn_id": actor, "active_actors": [actor_short]}})
    print(f"  Turn {i+1}: {actor_short}")

print(f"\nFinal state:")
print(f"  Session: {m._session_count}")
print(f"  Round: {m._round_count}")
print(f"  Turn: {m._turn_count}")
print(f"  Turn actors (partita 2): {m._turn_actors}")

ok = (
    m._session_count == 2
    and m._round_count == 1
    and m._turn_count == 3
    and m._turn_actors == ["X", "O", "X"]
    and "Session: 2" in m._lbl_session.text()
    and "Round: 1" in m._lbl_round.text()
    and "Turn: 3" in m._lbl_turn.text()
)

print("\nRESULT:", "PASS ✓" if ok else "FAIL ✗")
sys.exit(0 if ok else 1)
