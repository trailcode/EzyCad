#!/usr/bin/env python3
"""Remote client for EzyCad --listen Python console.

Protocol: uint32 big-endian length + UTF-8 JSON
  request:  {"id": int, "code": str}
  response: {"id": int, "ok": bool, "output": str, "result": str, "error": str}

Example:
  python scripts/ezycad_remote.py --host 127.0.0.1 --port 8765
  # or import Session and use session.ezy / session.view proxies
"""

from __future__ import annotations

import argparse
import json
import socket
import struct
import sys
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


def _format_arg(value: Any) -> str:
    return repr(value)


def _format_call(expr: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> str:
    parts = [_format_arg(a) for a in args]
    parts.extend(f"{k}={_format_arg(v)}" for k, v in kwargs.items())
    return f"{expr}({', '.join(parts)})"


class _RemoteAttr:
    def __init__(self, session: "Session", expr: str) -> None:
        self._session = session
        self._expr = expr

    def __getattr__(self, name: str) -> "_RemoteAttr":
        if name.startswith("_"):
            raise AttributeError(name)
        return _RemoteAttr(self._session, f"{self._expr}.{name}")

    def __call__(self, *args: Any, **kwargs: Any) -> Result:
        return self._session.execute(_format_call(self._expr, args, kwargs))


class Session:
    def __init__(self, sock: socket.socket) -> None:
        self._sock = sock
        self._next_id = 1

    @classmethod
    def connect(cls, host: str = "127.0.0.1", port: int = 8765, timeout: float = 30.0) -> "Session":
        sock = socket.create_connection((host, port), timeout=timeout)
        sock.settimeout(timeout)
        return cls(sock)

    def close(self) -> None:
        try:
            self._sock.close()
        except OSError:
            pass

    def __enter__(self) -> "Session":
        return self

    def __exit__(self, *args: Any) -> None:
        self.close()

    def _send_frame(self, payload: bytes) -> None:
        self._sock.sendall(struct.pack(">I", len(payload)) + payload)

    def _recv_exact(self, n: int) -> bytes:
        buf = bytearray()
        while len(buf) < n:
            chunk = self._sock.recv(n - len(buf))
            if not chunk:
                raise ConnectionError("connection closed while reading")
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

    @property
    def ezy(self) -> _RemoteAttr:
        return _RemoteAttr(self, "ezy")

    @property
    def view(self) -> _RemoteAttr:
        return _RemoteAttr(self, "view")


def _print_result(r: Result) -> None:
    if r.output:
        print(r.output)
    if r.result:
        print(r.result)
    if r.error:
        print(r.error, file=sys.stderr)
    if not r.ok and not r.error:
        print("request failed", file=sys.stderr)


def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="EzyCad remote Python console client")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("-c", "--code", help="Execute one snippet and exit")
    args = parser.parse_args(argv)

    try:
        session = Session.connect(args.host, args.port)
    except OSError as e:
        print(f"connect failed: {e}", file=sys.stderr)
        return 1

    with session:
        if args.code is not None:
            r = session.execute(args.code)
            _print_result(r)
            return 0 if r.ok else 1

        print(f"Connected to {args.host}:{args.port}. Enter Python; Ctrl-D/Ctrl-Z to quit.")
        while True:
            try:
                line = input(">>> ")
            except EOFError:
                print()
                break
            except KeyboardInterrupt:
                print()
                break
            if not line.strip():
                continue
            r = session.execute(line)
            _print_result(r)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
