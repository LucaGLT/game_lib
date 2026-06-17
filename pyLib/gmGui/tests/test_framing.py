"""Tests for engine_bridge.framing wire protocol.

All tests use ``socket.socketpair()`` (loopback — no real network required)
to exercise ``send_frame``, ``recv_frame``, and ``_recv_exact``.

Large-payload tests spin up a reader thread to avoid deadlocking on kernel
socket buffers that may be smaller than the payload.
"""
from __future__ import annotations

import json
import socket
import struct
import threading
from typing import Any

import pytest

from gmGui.engine_bridge.framing import _recv_exact, recv_frame, send_frame


# ── helpers ───────────────────────────────────────────────────────────────────

def _make_pair() -> tuple[socket.socket, socket.socket]:
    """Returns a connected socket pair (writer, reader)."""
    return socket.socketpair()


# ── framing round-trip ────────────────────────────────────────────────────────

def test_send_recv_roundtrip_small() -> None:
    """send_frame + recv_frame round-trip with a minimal JSON payload."""
    writer, reader = _make_pair()
    try:
        payload = '{"typeId":"engine.tick","source":"CoreEngine"}'
        send_frame(writer, payload)
        result = recv_frame(reader)
        assert result == payload
    finally:
        writer.close()
        reader.close()


def test_send_recv_roundtrip_json_dict() -> None:
    """Serialised dict survives a send/recv round-trip unchanged."""
    writer, reader = _make_pair()
    try:
        original = {
            "typeId": "gmActor.actor.hp_changed",
            "source": "CoreEngine",
            "headers": {"data": json.dumps({"actor_id": "p1", "new_hp": 42})},
        }
        payload = json.dumps(original)
        send_frame(writer, payload)
        result = json.loads(recv_frame(reader))
        assert result == original
    finally:
        writer.close()
        reader.close()


def test_send_recv_roundtrip_large() -> None:
    """send_frame + recv_frame round-trip with a ~100 KB payload."""
    writer, reader = _make_pair()
    try:
        payload = json.dumps({"data": "x" * (100 * 1024)})
        result: list[Any] = []

        def _receive() -> None:
            try:
                result.append(recv_frame(reader))
            except Exception as exc:  # noqa: BLE001
                result.append(exc)

        # Start reader thread first (it will block until data arrives).
        t = threading.Thread(target=_receive, daemon=True)
        t.start()

        # Now send — kernel buffers won't deadlock because the reader is running.
        send_frame(writer, payload)
        t.join(timeout=10.0)

        assert not t.is_alive(), "recv_frame thread timed out"
        assert len(result) == 1
        assert isinstance(result[0], str), f"recv_frame raised: {result[0]}"
        assert result[0] == payload
    finally:
        writer.close()
        reader.close()


def test_send_multiple_frames_sequentially() -> None:
    """Three consecutive frames are received in the correct order."""
    writer, reader = _make_pair()
    try:
        payloads = ['{"n":1}', '{"n":2}', '{"n":3}']
        for p in payloads:
            send_frame(writer, p)
        for p in payloads:
            assert recv_frame(reader) == p
    finally:
        writer.close()
        reader.close()


# ── length prefix format ──────────────────────────────────────────────────────

def test_length_prefix_is_big_endian() -> None:
    """The 4-byte length prefix uses network (big-endian) byte order."""
    writer, reader = _make_pair()
    try:
        send_frame(writer, "A")  # 1 byte payload in UTF-8
        raw_prefix = reader.recv(4)
        assert raw_prefix == struct.pack(">I", 1), (
            "Length prefix must be 4-byte big-endian uint32"
        )
    finally:
        writer.close()
        reader.close()


def test_length_prefix_matches_utf8_byte_length() -> None:
    """The length prefix encodes the *byte* length, not the character count."""
    writer, reader = _make_pair()
    try:
        # '€' is U+20AC — 3 bytes in UTF-8 but 1 character
        send_frame(writer, "€")
        raw_prefix = reader.recv(4)
        assert struct.unpack(">I", raw_prefix)[0] == 3
    finally:
        writer.close()
        reader.close()


# ── _recv_exact ───────────────────────────────────────────────────────────────

def test_recv_exact_raises_connection_error_on_closed_socket() -> None:
    """_recv_exact raises ConnectionError when the socket closes mid-frame."""
    writer, reader = _make_pair()
    try:
        # Announce 100 bytes are coming, then close without sending them.
        writer.send(struct.pack(">I", 100))
        writer.close()
        writer = None  # type: ignore[assignment]

        with pytest.raises(ConnectionError):
            # recv_frame reads the 4-byte header, then tries to read 100 bytes
            # but the socket is already closed.
            recv_frame(reader)
    finally:
        reader.close()
        if writer is not None:
            writer.close()


def test_recv_exact_zero_bytes_returns_empty() -> None:
    """_recv_exact(sock, 0) returns b'' without touching the socket."""
    writer, reader = _make_pair()
    try:
        result = _recv_exact(reader, 0)
        assert result == b""
    finally:
        writer.close()
        reader.close()


def test_recv_exact_partial_delivery() -> None:
    """_recv_exact reassembles data delivered across multiple recv() calls."""
    writer, reader = _make_pair()
    try:
        # Send 6 bytes in two bursts of 3.
        writer.send(b"abc")
        writer.send(b"def")
        result = _recv_exact(reader, 6)
        assert result == b"abcdef"
    finally:
        writer.close()
        reader.close()
