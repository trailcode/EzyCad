"""Low-level length-prefixed JSON transport for EzyCad --listen."""

from __future__ import annotations

import json
import socket
import struct
from dataclasses import dataclass
from typing import Any, Optional


@dataclass
class Result:
    ok: bool
    output: str
    result: str
    error: str
    id: int = 0

    def __str__(self) -> str:
        parts = []
        if self.output:
            parts.append(self.output)
        if self.result:
            parts.append(self.result)
        if self.error:
            parts.append(self.error)
        return "\n".join(parts) if parts else ("" if self.ok else "error")


class Session:
    """Raw remote console session (send Python source, get structured reply)."""

    def __init__(self, sock: socket.socket) -> None:
        self._sock = sock
        self._next_id = 1

    @classmethod
    def connect(cls, host: str = "127.0.0.1", port: int = 8765, timeout: float = 30.0) -> "Session":
        sock = socket.create_connection((host, port), timeout=timeout)
        sock.settimeout(timeout)
        return cls(sock)

    def close(self) -> None:
        sock = getattr(self, "_sock", None)
        self._sock = None
        if sock is None:
            return
        try:
            sock.close()
        except OSError:
            pass

    def __enter__(self) -> "Session":
        return self

    def __exit__(self, *args: Any) -> None:
        self.close()

    def _require_sock(self) -> socket.socket:
        sock = self._sock
        if sock is None:
            raise ConnectionError(
                "EzyCad session is closed; call ezycad.connect() again "
                "(do not reuse app after close() or exiting a with-block)"
            )
        return sock

    def _send_frame(self, payload: bytes) -> None:
        sock = self._require_sock()
        try:
            sock.sendall(struct.pack(">I", len(payload)) + payload)
        except OSError as e:
            raise ConnectionError(
                "lost connection to EzyCad --listen (is the app still running?). "
                "Reconnect with ezycad.connect()"
            ) from e

    def _recv_exact(self, n: int) -> bytes:
        sock = self._require_sock()
        buf = bytearray()
        while len(buf) < n:
            try:
                chunk = sock.recv(n - len(buf))
            except OSError as e:
                raise ConnectionError(
                    "lost connection to EzyCad --listen (is the app still running?). "
                    "Reconnect with ezycad.connect()"
                ) from e
            if not chunk:
                raise ConnectionError("connection closed while reading; reconnect with ezycad.connect()")
            buf.extend(chunk)
        return bytes(buf)

    def _recv_frame(self) -> bytes:
        (length,) = struct.unpack(">I", self._recv_exact(4))
        if length > 16 * 1024 * 1024:
            raise ValueError(f"frame too large: {length}")
        if length == 0:
            return b""
        return self._recv_exact(length)

    def execute(self, code: str) -> Result:
        req_id = self._next_id
        self._next_id += 1
        req = {"id": req_id, "code": code}
        self._send_frame(json.dumps(req).encode("utf-8"))
        raw = self._recv_frame()
        data = json.loads(raw.decode("utf-8"))
        return Result(
            ok=bool(data.get("ok", False)),
            output=str(data.get("output", "") or ""),
            result=str(data.get("result", "") or ""),
            error=str(data.get("error", "") or ""),
            id=int(data.get("id", req_id)),
        )

    def eval(self, expr: str) -> Result:
        """Send an expression (same wire path as execute; result field may be filled)."""
        return self.execute(expr)
