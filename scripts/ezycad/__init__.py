"""EzyCad remote client: importable API for a running EzyCad --listen instance.

Quick start (put ``scripts/`` on ``PYTHONPATH``, or)::

    import sys
    sys.path.insert(0, r"C:\\src\\EzyCad\\scripts")  # or cd / repo setup
    import ezycad
    app = ezycad.connect()          # keep this; do not use `with` in IPython if you reuse app
    app.ezy.log("hello")
    n = app.view.sketch_count()     # tab-completes in IPython / IDEs

Requires EzyCad launched with ``--listen`` (see docs/scripting.md).
"""

from __future__ import annotations

from typing import Optional

from ezycad._session import Result, Session
from ezycad.api import Ezy, EzyCadError, Shp, Sketch, View

__all__ = [
    "EzyCad",
    "Ezy",
    "View",
    "Sketch",
    "Shp",
    "Session",
    "Result",
    "EzyCadError",
    "connect",
]


class EzyCad:
    """Connected remote EzyCad application."""

    def __init__(self, session: Session) -> None:
        self._session = session
        self.ezy = Ezy(session)
        self.view = self.ezy.view
        self.sketch = self.ezy.sketch

    @classmethod
    def connect(cls, host: str = "127.0.0.1", port: int = 8765, timeout: float = 30.0) -> "EzyCad":
        return cls(Session.connect(host=host, port=port, timeout=timeout))

    def close(self) -> None:
        self._session.close()

    def __enter__(self) -> "EzyCad":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()

    def execute(self, code: str) -> Result:
        """Send raw Python source to the in-app console."""
        return self._session.execute(code)

    def eval(self, expr: str) -> Result:
        """Send an expression; Result.result may hold the repr."""
        return self._session.eval(expr)


def connect(host: str = "127.0.0.1", port: int = 8765, timeout: float = 30.0) -> EzyCad:
    """Connect to EzyCad --listen and return a typed client."""
    return EzyCad.connect(host=host, port=port, timeout=timeout)
