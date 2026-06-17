import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, r"c:\_GLT_\Qt Prj\game_lib\pyLib")

from PySide6.QtWidgets import QApplication

from gmGui.modules.gm_flow_module import GmFlowModule

app = QApplication([])
m = GmFlowModule()
m.widget()

# Simula una sessione Tris.
m.on_envelope({"typeId": "gmFlow.session.started", "data": {"session_id": "tris"}})
m.on_envelope({"typeId": "gmFlow.phase.entered", "data": {"phase_id": "ON_GOING"}})
m.on_envelope({"typeId": "gmFlow.round.started", "data": {"index": 1}})

# Simula turni alternati X e O.
for i in range(5):
    actor = "PLAYER_X" if i % 2 == 0 else "PLAYER_O"
    m.on_envelope({"typeId": "gmFlow.turn.started", 
                   "data": {"turn_id": actor, "active_actors": [actor]}})

m.on_envelope({"typeId": "gmFlow.phase.entered", "data": {"phase_id": "GAME_OVER"}})
m.on_envelope({"typeId": "gmFlow.session.completed", "data": {}})

print("Session    :", m._lbl_session.text())
print("Phase      :", m._lbl_phase.text())
print("Round      :", m._lbl_round.text())
print("Turn       :", m._lbl_turn.text())
print("Turn count :", m._turn_count)
print("Round count:", m._round_count)
print("Log entries:", m._log.count())
print("Log visible:", m._log.isVisible())
print("Status msg :", m._status_msg.text())

ok = (m._turn_count == 5 and m._round_count == 1 
      and m._log.count() == 10
      and "Sessione terminata" in m._status_msg.text()
      and m._lbl_turn.text().startswith("⏱"))
print("RESULT:", "PASS" if ok else "FAIL")
sys.exit(0 if ok else 1)
