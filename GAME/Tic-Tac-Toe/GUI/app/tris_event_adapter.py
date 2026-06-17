"""TrisEventAdapter — translates Tris engine events into the gmGui module contract.

The generic ``GmXxxModule`` dashboards (Actor / Flow / Dice) were designed for a
richer event contract than the Tic-Tac-Toe engine emits.  Rather than changing
the C++ engine (its Phase 1/2 wire contract must stay intact), this adapter maps
each native Tris envelope into zero or more envelopes shaped the way the generic
modules expect.

Mapping summary
---------------
- ``gmActor.snapshot``            → ``gmActor.snapshot`` (actors enriched with
                                     faction/name/hp/statuses-as-dict)
- ``gmActor.actor.status_added``  → ``gmActor.actor.status_added`` (+stacks)
- ``gmActor.actor.status_removed``→ ``gmActor.actor.status_removed`` (status_id)
- ``gmFlow.session.started``      → ``gmFlow.session.started`` + ``gmFlow.phase.entered``
                                     + ``gmAlea.dice.setup`` (1D2, locked)
- ``gmFlow.session.phase_changed``→ ``gmFlow.phase.entered`` (+ ``gmFlow.session.completed``
                                     when the phase is ``GAME_OVER``)
- ``gmAlea.dice_rolled``          → ``gmAlea.dice.setup`` (1D2, locked, with result)
- everything else (map / rules)   → no dashboard translation (handled natively by
                                     :class:`GmTrisBoardModule`)

Each produced envelope carries the payload both as ``data`` (dict) and as
``headers["data"]`` (JSON string), because the generic modules read the payload
inconsistently (``GmDiceModule`` reads ``headers.data`` while the others read
``data``).
"""
from __future__ import annotations

import json


def _faction_of(actor_id: str) -> str:
    """Returns the faction id (``"team_x"`` / ``"team_o"``) from an actor id."""
    return "team_x" if actor_id.endswith("X") else "team_o"


def _display_name(actor_id: str) -> str:
    """Returns a human-readable name (``"Player X"``) from an actor id."""
    return "Player X" if actor_id.endswith("X") else "Player O"


def _envelope(type_id: str, payload: dict) -> dict:
    """Builds a module-facing envelope exposing *payload* via both access paths."""
    return {
        "typeId": type_id,
        "source": "gmTris.adapter",
        "data": payload,
        "headers": {"data": json.dumps(payload)},
    }


class TrisEventAdapter:
    """Stateless translator from the native Tris event contract to gmGui modules."""

    def translate(self, msg: dict) -> list[dict]:
        """Returns the list of dashboard envelopes derived from a Tris envelope."""
        tid: str = msg.get("typeId", "")
        data: dict = msg.get("data", {}) or {}

        if tid == "gmActor.snapshot":
            return self._actor_snapshot(data)
        if tid == "gmActor.actor.status_added":
            return self._status_added(data)
        if tid == "gmActor.actor.status_removed":
            return self._status_removed(data)
        if tid == "gmFlow.session.started":
            return self._session_started(data)
        if tid == "gmFlow.session.phase_changed":
            return self._phase_changed(data)
        if tid == "gmAlea.dice_rolled":
            return self._dice_rolled(data)
        return []

    # ── gmActor translations ──────────────────────────────────────────────────

    def _actor_snapshot(self, data: dict) -> list[dict]:
        actors: list[dict] = []
        for actor in data.get("actors", []):
            actor_id: str = str(actor.get("actor_id", ""))
            if not actor_id:
                continue
            statuses: dict = {str(s): 1 for s in actor.get("statuses", [])}
            actors.append(
                {
                    "actor_id": actor_id,
                    "faction_id": _faction_of(actor_id),
                    "name": _display_name(actor_id),
                    "current_hp": 1,
                    "max_hp": 1,
                    "life_state": "ALIVE",
                    "statuses": statuses,
                    "equipment": {},
                    "area_id": "",
                }
            )
        return [_envelope("gmActor.snapshot", {"actors": actors})]

    def _status_added(self, data: dict) -> list[dict]:
        return [
            _envelope(
                "gmActor.actor.status_added",
                {
                    "actor_id": str(data.get("actor_id", "")),
                    "status_id": str(data.get("status", "")),
                    "stacks": 1,
                },
            )
        ]

    def _status_removed(self, data: dict) -> list[dict]:
        return [
            _envelope(
                "gmActor.actor.status_removed",
                {
                    "actor_id": str(data.get("actor_id", "")),
                    "status_id": str(data.get("status", "")),
                },
            )
        ]

    # ── gmFlow translations ───────────────────────────────────────────────────

    def _session_started(self, data: dict) -> list[dict]:
        phase: str = str(data.get("phase", ""))
        envelopes: list[dict] = [
            _envelope("gmFlow.session.started", {"session_id": "tris"})
        ]
        if phase:
            envelopes.append(_envelope("gmFlow.phase.entered", {"phase_id": phase}))
        # In Tris the dice are owned by the engine (1d2 starter): configure the
        # dice panel as 1D2 and lock the manual controls so the user cannot roll.
        envelopes.append(
            _envelope(
                "gmAlea.dice.setup",
                {"mode": "standard", "count": 1, "faces": 2, "locked": True},
            )
        )
        return envelopes

    def _phase_changed(self, data: dict) -> list[dict]:
        phase: str = str(data.get("phase", ""))
        envelopes: list[dict] = [
            _envelope("gmFlow.phase.entered", {"phase_id": phase})
        ]
        if phase == "GAME_OVER":
            envelopes.append(_envelope("gmFlow.session.completed", {}))
        return envelopes

    # ── gmAlea translations ───────────────────────────────────────────────────

    def _dice_rolled(self, data: dict) -> list[dict]:
        # The engine already rolled the 1d2: drive the dice panel with a single
        # setup message that both configures it (1D2, locked) and shows the
        # result, keeping the displayed dice consistent with what was rolled.
        value: int = int(data.get("value", 0))
        return [
            _envelope(
                "gmAlea.dice.setup",
                {
                    "mode": "standard",
                    "count": 1,
                    "faces": 2,
                    "locked": True,
                    "result": {"dice": [value], "total": value},
                },
            )
        ]
