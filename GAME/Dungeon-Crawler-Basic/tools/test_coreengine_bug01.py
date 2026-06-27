#!/usr/bin/env python3
"""
BUG_01 CoreEngine isolation test.

Drives the C++ CoreEngine directly (no GUI), simulating what the GUI *should*
send, and verifies that playing a 2-cost card ("Pugno di Ferro") while the hero
has 2/2 actions is NOT rejected by the engine.

Architecture recap:
- CoreEngine listens on port 9201 for commands (TCP server).
- CoreEngine connects to port 9200 to push events (TCP client).
  So this test acts as:
    * event SERVER on 9200 (accept the engine's outbound connection)
    * command CLIENT on 9201 (send commands to the engine)

Wire format (both directions): 4-byte big-endian length prefix + UTF-8 JSON.

Commands sent: {"typeId": "...", "data": {...}}
Events received: {"typeId": "...", "source": "...", "headers": {"data": "<json string>"}}

Exit code 0 = BUG_01 NOT present in CoreEngine.
Exit code 1 = BUG_01 present in CoreEngine (engine wrongly rejected the play).
"""
from __future__ import annotations

import json
import socket
import struct
import subprocess
import sys
import threading
import time
from pathlib import Path
from queue import Empty, Queue

# ── Configuration ─────────────────────────────────────────────────────────────
EVENTS_PORT = 9200    # this test listens here; engine connects out to push events
COMMANDS_PORT = 9201  # engine listens here; this test connects to send commands
HOST = "127.0.0.1"

_REPO = Path(__file__).resolve().parents[3]
ENGINE_EXE = _REPO / "build" / "GAME" / "Dungeon-Crawler-Basic" / "CoreEngine" / "Debug" / "dungeon_engine.exe"


# ── Framing helpers ───────────────────────────────────────────────────────────
def send_frame(sock: socket.socket, type_id: str, data: dict) -> None:
    """Send a command frame to the engine: 4-byte length + JSON."""
    payload = json.dumps({"typeId": type_id, "data": data}).encode("utf-8")
    sock.sendall(struct.pack(">I", len(payload)) + payload)


def recv_exact(sock: socket.socket, n: int) -> bytes | None:
    """Receive exactly n bytes or None on EOF."""
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            return None
        buf += chunk
    return buf


def recv_frame(sock: socket.socket) -> dict | None:
    """Receive one framed JSON event and normalise headers['data']."""
    head = recv_exact(sock, 4)
    if head is None:
        return None
    (length,) = struct.unpack(">I", head)
    if length == 0:
        return {}
    body = recv_exact(sock, length)
    if body is None:
        return None
    msg = json.loads(body.decode("utf-8"))
    # Normalise: real payload lives in headers["data"] as a JSON string.
    headers = msg.get("headers", {})
    if isinstance(headers, dict) and "data" in headers:
        try:
            msg["data"] = json.loads(headers["data"])
        except (json.JSONDecodeError, TypeError, ValueError):
            msg["data"] = headers["data"]
    elif "data" not in msg:
        msg["data"] = {}
    return msg


# ── Event server (accepts the engine's outbound connection) ───────────────────
class EventServer(threading.Thread):
    """Accepts the engine connection on EVENTS_PORT and queues every event."""

    def __init__(self, queue: Queue) -> None:
        super().__init__(daemon=True)
        self._queue = queue
        self._running = True
        self._srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._srv.bind((HOST, EVENTS_PORT))
        self._srv.listen(1)
        self._srv.settimeout(1.0)
        self._client: socket.socket | None = None

    def run(self) -> None:
        while self._running:
            if self._client is None:
                try:
                    self._client, _ = self._srv.accept()
                    self._client.settimeout(1.0)
                except socket.timeout:
                    continue
                except OSError:
                    break
            try:
                msg = recv_frame(self._client)
            except socket.timeout:
                continue
            except (ConnectionError, OSError):
                self._client = None
                continue
            if msg is None:
                self._client = None
                continue
            self._queue.put(msg)
        self._cleanup()

    def stop(self) -> None:
        self._running = False
        self._cleanup()

    def _cleanup(self) -> None:
        for s in (self._client, self._srv):
            if s is not None:
                try:
                    s.close()
                except OSError:
                    pass
        self._client = None


# ── Test scenario ─────────────────────────────────────────────────────────────
def drain_until(queue: Queue, type_suffix: str, timeout: float = 6.0) -> dict | None:
    """Pull events until one whose typeId ends with type_suffix; collect all seen."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            msg = queue.get(timeout=0.3)
        except Empty:
            continue
        tid = msg.get("typeId", "")
        print(f"    « {tid}")
        if tid.endswith(type_suffix):
            return msg
    return None


def collect_for(queue: Queue, seconds: float) -> list[dict]:
    """Collect every event arriving within the given window."""
    out: list[dict] = []
    deadline = time.time() + seconds
    while time.time() < deadline:
        try:
            msg = queue.get(timeout=0.2)
        except Empty:
            continue
        out.append(msg)
        print(f"    « {msg.get('typeId', '')}")
    return out


def wait_hero_turn(queue: Queue, hero_id: str, timeout: float = 10.0,
                   cmd: socket.socket | None = None) -> int | None:
    """Wait until a turn.started arrives for hero_id; return its actions_remaining.

    Any intermediate turn that belongs to a DIFFERENT actor is ended immediately
    (by sending dungeon.end_turn) so the engine keeps advancing. Monster turns are
    auto-advanced by the engine itself.
    """
    deadline = time.time() + timeout
    while time.time() < deadline:
        msg = drain_until(queue, ".turn.started", timeout=deadline - time.time())
        if msg is None:
            return None
        actor = msg["data"].get("actor_id")
        if actor == hero_id:
            return int(msg["data"].get("actions_remaining", -1))
        # Someone else's turn (another hero waiting for input): end it.
        if cmd is not None:
            print(f"    (ending intermediate turn of {actor})")
            send_frame(cmd, "dungeon.end_turn", {})
    return None


def drive_and_attack(cmd, queue, hero_id, enemy_id, path):
    """Move hero along path on its first turn, then attack on a fresh turn.

    Returns True if the engine ACCEPTS the 2-cost attack at 2/2 actions.
    """
    # ── First hero turn: walk toward the enemy ────────────────────────────────
    print("\n[STEP 3] Wait for first hero turn")
    actions = wait_hero_turn(queue, hero_id, cmd=cmd)
    if actions is None:
        print("✗ Hero turn never started.")
        return False
    print(f"  hero turn #1, actions_remaining = {actions}")

    for room in path:
        print(f"  → move hero to {room}")
        send_frame(cmd, "dungeon.move", {"hero_id": hero_id, "destination": room})
        burst = collect_for(queue, 1.5)
        if any(m.get("typeId", "").endswith(".action.rejected") for m in burst):
            rej = next(m for m in burst if m.get("typeId", "").endswith(".action.rejected"))
            print(f"✗ Move rejected: {rej['data'].get('reason')}")
            return False

    print("  → end hero turn")
    send_frame(cmd, "dungeon.end_turn", {})

    # ── Wait for the hero's NEXT turn (fresh 2/2 actions, now adjacent) ────────
    print("\n[STEP 4] Wait for the hero's NEXT turn (fresh actions)")
    actions = wait_hero_turn(queue, hero_id, timeout=15.0, cmd=cmd)
    if actions is None:
        print("✗ Hero's second turn never started.")
        return False
    print(f"  hero turn #2, actions_remaining = {actions}")
    if actions != 2:
        print(f"⚠ Expected 2/2 actions on fresh turn, got {actions}/2")

    # Drain backlog before the critical command.
    time.sleep(0.3)
    while not queue.empty():
        queue.get_nowait()

    # ── Critical action: play 'Pugno di Ferro' (cost 2) at 2/2 actions ────────
    print("\n[STEP 5] Send dungeon.attack with card 'pugno_di_ferro' (cost 2) at 2/2 actions")
    send_frame(cmd, "dungeon.attack", {
        "attacker_id": hero_id,
        "target_id": enemy_id,
        "card_id": "pugno_di_ferro",
        "card_damage": 4,
    })

    burst = collect_for(queue, 2.5)
    type_ids = [m.get("typeId", "") for m in burst]
    rejected = [m for m in burst if m.get("typeId", "").endswith(".action.rejected")]
    declared = any(t.endswith(".attack.declared") for t in type_ids)
    defense_open = any(t.endswith(".defense.window.opened") for t in type_ids)

    print("\n[RESULT]")
    if rejected:
        reason = rejected[0]["data"].get("reason", "")
        print(f"  ✗ Engine REJECTED the 2-cost play at 2/2 actions: \"{reason}\"")
        print("  → BUG_01 IS PRESENT IN THE COREENGINE.")
        return False
    if declared or defense_open:
        print("  ✓ Engine ACCEPTED the 2-cost play at 2/2 actions.")
        print("  → BUG_01 is NOT in the CoreEngine.")
        print("  → The 'Azioni esaurite' pop-up comes from the GUI (client-side check).")
        return True
    print(f"  ⚠ Inconclusive. Events seen: {type_ids}")
    return False


def run() -> bool:
    print("=" * 72)
    print("BUG_01 CoreEngine Isolation Test (no GUI)")
    print("=" * 72)

    if not ENGINE_EXE.exists():
        print(f"✗ Engine executable not found: {ENGINE_EXE}")
        print("  Build target 'dungeon_engine' first.")
        return False

    events: Queue = Queue()
    server = EventServer(events)
    server.start()
    print(f"[SETUP] Event server listening on {HOST}:{EVENTS_PORT}")

    # Launch the engine process.
    print(f"[SETUP] Launching engine: {ENGINE_EXE.name}")
    proc = subprocess.Popen(
        [str(ENGINE_EXE)],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=str(_REPO),
        text=True,
    )
    time.sleep(1.0)  # let the CmdServer bind 9201

    cmd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    cmd.settimeout(5.0)
    try:
        cmd.connect((HOST, COMMANDS_PORT))
        print(f"[SETUP] Connected command client to {HOST}:{COMMANDS_PORT}")
    except OSError as exc:
        print(f"✗ Could not connect to engine command port: {exc}")
        proc.terminate()
        server.stop()
        return False

    try:
        # 1. Start a new game.
        print("\n[STEP 1] Send dungeon.new_game")
        send_frame(cmd, "dungeon.new_game", {})

        # 2. Wait for the actor snapshot to discover hero + enemy ids.
        print("[STEP 2] Wait for actor snapshot")
        snapshot = drain_until(events, ".actor.snapshot", timeout=8.0)
        if snapshot is None:
            print("✗ No actor snapshot received — engine did not start a session.")
            return False

        actors = snapshot["data"].get("actors", [])
        hero = next((a for a in actors if a.get("kind") == "HERO"), None)
        enemy = next((a for a in actors if a.get("kind") != "HERO"), None)
        if not hero or not enemy:
            print(f"✗ Could not find both a hero and an enemy. Actors: {actors}")
            return False
        hero_id = hero.get("id")
        enemy_id = enemy.get("id")
        enemy_room = enemy.get("location")
        print(f"  hero  = {hero_id} @ {hero.get('location')}")
        print(f"  enemy = {enemy_id} @ {enemy_room}")

        # 3. Drive the hero adjacent to the enemy, then attack on a FRESH turn
        #    (2/2 actions) with a 2-cost card. This is the exact BUG_01 scenario.
        #    Map: start → corridor_1 → corridor_2(goblin). Attack reach = same or
        #    adjacent room, so the hero must reach corridor_1.
        path_to_adjacent = ["corridor_1"]  # one move puts hero adjacent to goblin

        result = drive_and_attack(
            cmd, events, hero_id, enemy_id, path_to_adjacent,
        )
        return result

    finally:
        try:
            send_frame(cmd, "dungeon.end_turn", {})
        except OSError:
            pass
        cmd.close()
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
        server.stop()


if __name__ == "__main__":
    ok = run()
    print("=" * 72)
    print("PASS — CoreEngine is correct" if ok else "FAIL — see details above")
    print("=" * 72)
    sys.exit(0 if ok else 1)
