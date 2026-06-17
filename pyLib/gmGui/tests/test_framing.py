"""Tests for engine_bridge.framing wire protocol.

Full implementation: Phase 2.
Phase 1: all tests are skipped (stubs not yet implemented).
"""
from __future__ import annotations

import pytest


@pytest.mark.skip(reason="Phase 2: framing not yet implemented")
def test_send_recv_roundtrip_small() -> None:
    """send_frame + recv_frame round-trip with a minimal JSON payload."""


@pytest.mark.skip(reason="Phase 2: framing not yet implemented")
def test_send_recv_roundtrip_large() -> None:
    """send_frame + recv_frame round-trip with a 100 KB payload."""


@pytest.mark.skip(reason="Phase 2: framing not yet implemented")
def test_recv_exact_raises_on_closed_socket() -> None:
    """_recv_exact raises ConnectionError when the socket closes mid-frame."""


@pytest.mark.skip(reason="Phase 2: framing not yet implemented")
def test_length_prefix_is_big_endian() -> None:
    """The 4-byte length prefix uses network (big-endian) byte order."""
