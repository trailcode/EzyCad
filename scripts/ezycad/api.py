"""Typed remote mirrors of the in-app ezy / ezy.view / ezy.sketch API.

Method names are explicit so IPython and IDEs can complete them.
Calls are marshaled to a running EzyCad --listen instance.
"""

from __future__ import annotations

import ast
from typing import Any, Dict, Optional, Tuple

from ezycad._session import Result, Session

Camera = Dict[str, Any]
Dim = Tuple[Any, Any, Any, Any, Any, Any]
Node = Tuple[float, float]


class EzyCadError(RuntimeError):
    def __init__(self, message: str, result: Optional[Result] = None) -> None:
        super().__init__(message)
        self.result = result


def _format_arg(value: Any) -> str:
    return repr(value)


def _format_call(expr: str, args: tuple[Any, ...], kwargs: dict[str, Any]) -> str:
    parts = [_format_arg(a) for a in args]
    parts.extend(f"{k}={_format_arg(v)}" for k, v in kwargs.items())
    return f"{expr}({', '.join(parts)})"


def _parse_result_value(text: str) -> Any:
    if text == "":
        return None
    try:
        return ast.literal_eval(text)
    except (ValueError, SyntaxError):
        return text


class _Remote:
    def __init__(self, session: Session) -> None:
        self._session = session

    def _run(self, code: str) -> Result:
        r = self._session.execute(code)
        if not r.ok:
            raise EzyCadError(r.error or "remote call failed", result=r)
        return r

    def _call(self, expr: str, *args: Any, **kwargs: Any) -> Any:
        r = self._run(_format_call(expr, args, kwargs))
        return _parse_result_value(r.result)

    def _call_void(self, expr: str, *args: Any, **kwargs: Any) -> Optional[str]:
        r = self._run(_format_call(expr, args, kwargs))
        return r.output or None


class Shp(_Remote):
    """Remote handle for a document shape (by 0-based index)."""

    def __init__(self, session: Session, index: int) -> None:
        super().__init__(session)
        self._index = int(index)

    @property
    def index(self) -> int:
        return self._index

    def _expr(self) -> str:
        return f"ezy.view.get_shape({self._index})"

    def name(self) -> str:
        return str(self._call(f"{self._expr()}.name"))

    def set_name(self, name: str) -> None:
        self._call_void(f"{self._expr()}.set_name", name)

    def visible(self) -> bool:
        return bool(self._call(f"{self._expr()}.visible"))

    def set_visible(self, visible: bool) -> None:
        self._call_void(f"{self._expr()}.set_visible", visible)

    def __repr__(self) -> str:
        try:
            return f"<ezycad.Shp index={self._index} name={self.name()!r}>"
        except EzyCadError:
            return f"<ezycad.Shp index={self._index}>"


class Sketch(_Remote):
    """ezy.sketch / ezy.view.curr_sketch - current sketch inspection and creation."""

    def name(self) -> str:
        return str(self._call("ezy.view.curr_sketch.name"))

    def curr_name(self) -> str:
        return self.name()

    def node_count(self) -> int:
        return int(self._call("ezy.view.curr_sketch.node_count"))

    def node(self, i: int) -> Node:
        value = self._call("ezy.view.curr_sketch.node", int(i))
        return (float(value[0]), float(value[1]))

    def dim_count(self) -> int:
        return int(self._call("ezy.view.curr_sketch.dim_count"))

    def dim(self, i: int) -> Dim:
        value = self._call("ezy.view.curr_sketch.dim", int(i))
        return tuple(value)  # type: ignore[return-value]

    def add(self, plane: str = "XY", offset: float = 0.0, base_name: Optional[str] = None) -> Any:
        return self._call("ezy.sketch.add", plane, offset, base_name)

    def add_edge(self, x1: float, y1: float, x2: float, y2: float) -> Any:
        return self._call("ezy.sketch.add_edge", x1, y1, x2, y2)

    def finish_edges(self) -> Any:
        return self._call("ezy.sketch.finish_edges")


class View(_Remote):
    """ezy.view - document shapes, counts, camera, and curr_sketch."""

    def __init__(self, session: Session, sketch: Sketch) -> None:
        super().__init__(session)
        self.curr_sketch = sketch

    def sketch_count(self) -> int:
        return int(self._call("ezy.view.sketch_count"))

    def shape_count(self) -> int:
        return int(self._call("ezy.view.shape_count"))

    def add_box(self, ox: float, oy: float, oz: float, w: float, l: float, h: float) -> Shp:
        self._call("ezy.view.add_box", ox, oy, oz, w, l, h)
        idx = self.shape_count() - 1
        if idx < 0:
            raise EzyCadError("add_box failed to create a shape")
        return Shp(self._session, idx)

    def add_sphere(self, ox: float, oy: float, oz: float, r: float) -> Shp:
        self._call("ezy.view.add_sphere", ox, oy, oz, r)
        idx = self.shape_count() - 1
        if idx < 0:
            raise EzyCadError("add_sphere failed to create a shape")
        return Shp(self._session, idx)

    def get_shape(self, i: int) -> Shp:
        self._call("ezy.view.get_shape", int(i))
        return Shp(self._session, int(i))

    def get_camera(self) -> Camera:
        value = self._call("ezy.view.get_camera")
        return dict(value) if isinstance(value, dict) else value

    def set_camera(
        self,
        ex: float,
        ey: float,
        ez: float,
        cx: float,
        cy: float,
        cz: float,
        ux: float,
        uy: float,
        uz: float,
    ) -> Any:
        return self._call("ezy.view.set_camera", ex, ey, ez, cx, cy, cz, ux, uy, uz)

    def add_sketch(self, plane: str = "XY", offset: float = 0.0, base_name: Optional[str] = None) -> Any:
        return self.curr_sketch.add(plane, offset, base_name)

    def add_edge(self, x1: float, y1: float, x2: float, y2: float) -> Any:
        return self.curr_sketch.add_edge(x1, y1, x2, y2)

    def finish_sketch_edges(self) -> Any:
        return self.curr_sketch.finish_edges()


class Ezy(_Remote):
    """Root public scripting API (mirrors in-app ezy)."""

    def __init__(self, session: Session) -> None:
        super().__init__(session)
        self.sketch = Sketch(session)
        self.view = View(session, self.sketch)

    def log(self, msg: Any) -> Optional[str]:
        return self._call_void("ezy.log", msg)

    def msg(self, text: str) -> None:
        self._call_void("ezy.msg", text)

    def get_mode(self) -> str:
        return str(self._call("ezy.get_mode"))

    def set_mode(self, name: str) -> None:
        self._call_void("ezy.set_mode", name)

    def save_occt_view_settings(self) -> None:
        self._call_void("ezy.save_occt_view_settings")

    def occt_view_settings_json(self) -> str:
        return str(self._call("ezy.occt_view_settings_json"))

    def help(self) -> Optional[str]:
        return self._call_void("ezy.help")
