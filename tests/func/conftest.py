# SPDX-License-Identifier: MIT
"""Functional test harness: starts mini-upf on a free localhost port and
gives tests a UDP socket that plays the gNB side of N3.

No root needed: packets are built with Scapy and sent as plain UDP payloads.
"""
import json
import os
import pathlib
import signal
import socket
import subprocess
import time

import pytest

ROOT = pathlib.Path(__file__).resolve().parents[2]
UPF_BIN = os.environ.get("MINI_UPF_BIN", str(ROOT / "build" / "mini-upf"))


def _free_udp_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class Upf:
    """Handle to a running mini-upf process."""

    def __init__(self, port: int):
        self.addr = ("127.0.0.1", port)
        self.proc = subprocess.Popen(
            [UPF_BIN, "-a", "127.0.0.1", "-p", str(port)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self._wait_ready()

    def _wait_ready(self, timeout: float = 3.0) -> None:
        # The UPF prints "N3 listening" on stderr once the socket is bound.
        deadline = time.monotonic() + timeout
        line = ""
        while time.monotonic() < deadline:
            if self.proc.poll() is not None:
                raise RuntimeError(f"mini-upf exited early: {self.proc.stderr.read()}")
            line = self.proc.stderr.readline()
            if "N3 listening" in line:
                return
        raise RuntimeError(f"mini-upf did not start, last line: {line!r}")

    def stop(self) -> dict:
        """Stop with SIGTERM and return the JSON counters it prints."""
        if self.proc.poll() is None:
            self.proc.send_signal(signal.SIGTERM)
        out, _ = self.proc.communicate(timeout=5)
        lines = [l for l in out.splitlines() if l.startswith("{")]
        return json.loads(lines[-1]) if lines else {}


@pytest.fixture
def upf():
    u = Upf(_free_udp_port())
    yield u
    if u.proc.poll() is None:
        u.stop()


@pytest.fixture
def gnb():
    """UDP socket acting as the gNB N3 endpoint."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", 0))
    s.settimeout(1.0)
    yield s
    s.close()
