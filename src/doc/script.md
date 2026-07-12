# Script console module

Developer reference for embedded Lua and Python consoles. Headers: [`scr_lua_console.h`](../scr_lua_console.h), [`scr_python_console.h`](../scr_python_console.h), [`scr_python_remote.h`](../scr_python_remote.h).

Maintainers: update this file when script bindings, console UI, remote listen, or startup script loading change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing scripting guide: [`docs/scripting.md`](../../docs/scripting.md).

## Purpose

The `scr_*` translation units embed **interactive script consoles** in the ImGui UI. They expose live `ezy.*` and `view.*` APIs to Lua and Python so users can inspect the document, add primitives, tweak the camera, and log messages while EzyCad runs.

| Class                    | File                     | Runtime                                    |
| ------------------------ | ------------------------ | ------------------------------------------ |
| `Lua_console`            | `scr_lua_console.cpp`    | Lua C API (native + Emscripten)            |
| `Python_console`         | `scr_python_console.cpp` | pybind11 embed (`EZYCAD_HAVE_PYTHON` only) |
| `Python_remote_server`   | `scr_python_remote.cpp`  | TCP `--listen` (native + Python only)      |
| `Python_execution_queue` | `scr_python_remote.cpp`  | Main-thread marshal for remote exec        |

Consoles are owned by `GUI` and rendered from `GUI::render_gui()` when feature tier allows (`show_lua_console_effective` / `show_python_console_effective`). Remote listen can force-create the Python console via `GUI::ensure_python_console()` without opening the UI.

## Requirements and invariants

### Lifetime and threading

| Object                           | Notes                                                              |
| -------------------------------- | ------------------------------------------------------------------ |
| `Lua_console` / `Python_console` | Constructed with `GUI*`; must not outlive `GUI`                    |
| `lua_State* m_L`                 | Created in ctor, closed in dtor                                    |
| Python runtime                   | `Ezycad_python_runtime` (pybind11) initialized in `init_python_()` |

Script callbacks run on the **main UI thread** during console input handling. Do not call OCCT or ImGui from worker threads without existing app synchronization.

Remote TCP accept/read runs on a **background thread**; it only enqueues work. `Python_execution_queue::process_pending()` runs on the main loop and calls `Python_console::execute_captured()`.

### Script file locations

| Platform   | Lua                                  | Python                    |
| ---------- | ------------------------------------ | ------------------------- |
| Native     | `res/scripts/lua/*.lua`              | `res/scripts/python/*.py` |
| Emscripten | `/res/scripts/lua/*.lua` (preloaded) | Not built                 |

On startup, **Lua** files in that directory are loaded into tabbed `TextEditor` buffers and **executed once** to define helpers. Python tabs load file contents but are not auto-run on startup (run manually or from input line).

### Index conventions

| API                        | Index base                                    |
| -------------------------- | --------------------------------------------- |
| Lua `view.get_shape(i)`    | **1-based**; returns `nil` out of range       |
| Python `view.get_shape(i)` | **0-based**; raises `IndexError` out of range |

Document this when adding parallel bindings.

## Architecture

```
GUI::render_gui()
  +-- lua_console_()   -> Lua_console::render
  +-- python_console_() -> Python_console::render (if EZYCAD_HAVE_PYTHON)

main (native, --listen)
  +-- GUI::ensure_python_console()
  +-- Python_remote_server (accept thread)
  +-- each frame: Python_execution_queue::process_pending(console)
        -> Python_console::execute_captured(code)

Lua_console
  +-- lua_State + register_bindings()
  +-- m_script_editors (path, filename, TextEditor)
  +-- REPL input (m_input_buf) + m_history

Python_console
  +-- pybind11 embedded module ezycad_native
  +-- exec bootstrap sets ezy / view on __main__
  +-- m_script_editors + REPL input
  +-- execute_captured returns {ok, output, result, error}
```

CMake IDE group: `src\scr`.

## Remote Python (`--listen`)

| Item         | Notes                                                                                                                  |
| ------------ | ---------------------------------------------------------------------------------------------------------------------- |
| CLI          | `EzyCad --listen [host:]port` (port-only binds `127.0.0.1`)                                                            |
| Protocol     | `uint32` BE length + UTF-8 JSON request/response                                                                       |
| Request      | `{"id": int, "code": str}`                                                                                             |
| Response     | `{"id", "ok", "output", "result", "error"}`                                                                            |
| Client       | [`scripts/ezycad/`](../../scripts/ezycad/) (`import ezycad`); CLI [`ezycad_remote.py`](../../scripts/ezycad_remote.py) |
| Out of scope | `--headless`, auth/TLS, wasm                                                                                           |

Shared interpreter with the ImGui console: same `__main__.ezy` / `view`.

## Binding surface

Implementation lives in the `.cpp` files; [`docs/scripting.md`](../../docs/scripting.md) is the user-facing **Python-first** API reference. Summary for developers:

### Public package shape

| Path                                    | Role                                                  |
| --------------------------------------- | ----------------------------------------------------- |
| `ezy.*`                                 | App / session (log, mode, settings, help)             |
| `ezy.view.*`                            | Document shapes, counts, camera                       |
| `ezy.view.curr_sketch` / `ezy.sketch.*` | Current sketch inspect / create / edges (same object) |
| `ezy.Shp`                               | Shape wrapper type                                    |

Aliases: global `view` == `ezy.view`; `Shp` == `ezy.Shp`; `view.add_sketch` / `finish_sketch_edges` forward to `view.curr_sketch.*`.

### `ezy` (root)

| Method                          | C++ delegate                           |
| ------------------------------- | -------------------------------------- |
| `ezy.log(msg)`                  | `GUI::log_message` + console history   |
| `ezy.msg(text)`                 | `GUI::show_message`                    |
| `ezy.get_mode()`                | `GUI::get_mode()` -> mode name string  |
| `ezy.set_mode(name)`            | `GUI::set_mode(mode_from_string(...))` |
| `ezy.save_occt_view_settings()` | `GUI::save_occt_view_settings()`       |
| `ezy.occt_view_settings_json()` | `GUI::occt_view_settings_json()`       |
| `ezy.help()` / global `help()`  | Prints binding summary to console      |

Lua overrides global `print` to call `ezy.log`. Python bootstrap assigns `builtins.print` similarly.

### `ezy.view`

| Method                                   | Notes                                                   |
| ---------------------------------------- | ------------------------------------------------------- |
| `sketch_count()`                         | `Occt_view::get_sketches().size()`                      |
| `shape_count()`                          | `Occt_view::get_shapes().size()`                        |
| `add_box(...)` / `add_sphere(...)`       | `Occt_view` helpers; return `Shp` / Lua userdata        |
| `fuse(s1, s2, ...)`                      | `Shp_fuse::fuse`; deletes inputs; returns `Shp`         |
| `cut(body, tool, ...)`                   | `Shp_cut::cut`; first is body, rest are tools           |
| `common(s1, s2, ...)`                    | `Shp_common::common`; deletes inputs; returns `Shp`     |
| `delete(s1, ...)`                        | `Occt_view::delete_shapes`                              |
| `get_camera()`                           | `{ eye, center, up }` tables / dicts                    |
| `set_camera(ex,ey,ez,cx,cy,cz,ux,uy,uz)` | `Occt_view::set_camera`                                 |
| `get_shape(i)`                           | Returns `Shp` wrapper                                   |
| `get_selected()`                         | Selected document `Shp` list / Lua table (may be empty) |
| `get_selected_indices()`                 | Indices into `get_shapes` (Python 0-based; Lua 1-based) |

### `ezy.view.curr_sketch` / `ezy.sketch`

| Method                          | Notes                                                      |
| ------------------------------- | ---------------------------------------------------------- |
| `name()` / `curr_name()`        | Current sketch display name                                |
| `node_count()` / `node(i)`      | `(x, y)` plane coords (Lua: 1-based; Python: 0-based)      |
| `dim_count()` / `dim(i)`        | Length dim tuple (Lua: 1-based)                            |
| `add(plane, offset, base_name)` | `Occt_view::add_sketch_on_ref_plane`; plane `XY`/`XZ`/`YZ` |
| `add_edge(x1,y1,x2,y2)`         | `Occt_view::curr_sketch_add_edge`                          |
| `finish_edges()`                | `Occt_view::curr_sketch_rebuild_faces`                     |

### `Shp` object

| Method                         | Lua                | Python                      |
| ------------------------------ | ------------------ | --------------------------- |
| `name()` / `set_name(s)`       | userdata metatable | `ezycad_native.Shp` methods |
| `visible()` / `set_visible(v)` | same               | same                        |

Lua stores `Shp_ptr` in userdata with `__gc`; Python wraps `Ezy_shp{ Shp_ptr shp }`.

## Console UI flow

| UI region       | Behavior                                                    |
| --------------- | ----------------------------------------------------------- |
| Output history  | `m_history` lines (errors flagged)                          |
| Input line      | `ImGui::InputTextWithHint`; Enter runs `execute()`          |
| Command history | Up/down browses prior input lines                           |
| Script tabs     | One tab per file in scripts dir; Save writes buffer to disk |

`Lua_console::text_edit_callback` / `Python_console::text_edit_callback` hook editor keyboard shortcuts (shared pattern with dist/angle edit).

## Adding a new binding

Supported scripting APIs must appear under **`ezy`** (Python canonical) and be documented in [`docs/scripting.md`](../../docs/scripting.md). GUI-only features stay out of this surface.

| Step | Action                                                                                     |
| ---- | ------------------------------------------------------------------------------------------ |
| 1    | Add or use a **public** C++ entry (`GUI` / `Occt_view` / `Shp` - not private `_*` helpers) |
| 2    | Define the **Python** method under `ezy` / `ezy.view` / `ezy.sketch` in bootstrap          |
| 3    | Mirror the same nested path in Lua (`register_bindings`)                                   |
| 4    | Add a flat alias only when replacing an old name                                           |
| 5    | Update `ezy.help()` / `l_ezy_help` and [`docs/scripting.md`](../../docs/scripting.md)      |

Prefer thin wrappers: validate args, call `GUI` / `Occt_view` / `Shp` methods, return simple types.

## Build flags

| Flag                 | Effect                                                                |
| -------------------- | --------------------------------------------------------------------- |
| (default)            | Lua console always linked (bundled Lua sources)                       |
| `EZYCAD_HAVE_PYTHON` | Enables Python console, pybind11 embed, `python3.dll` copy on Windows |

If Python init fails, `Python_console` shows an error in history and disables execution.

## Testing

| Item                | Notes                                                           |
| ------------------- | --------------------------------------------------------------- |
| Automated API tests | Limited; bindings exercised manually and via `res/scripts/*`    |
| Regression          | Run sample lines from `docs/scripting.md` after binding changes |
| Lua on WASM         | Verify `/res/scripts/lua` preload paths                         |

## Related code outside `src/scr*`

| Location                                | Role                                                                             |
| --------------------------------------- | -------------------------------------------------------------------------------- |
| [`gui.cpp`](../gui.cpp)                 | Owns consoles, `lua_console_()` / `python_console_()`, `ensure_python_console()` |
| [`gui.h`](../gui.h)                     | `get_view()`, settings JSON, mode API used by bindings                           |
| [`main.cpp`](../main.cpp)               | `--listen` parse, remote start, `process_pending()`                              |
| [`mode.h`](../mode.h)                   | `mode_from_string`, `c_mode_strs`                                                |
| [`gui_occt_view.h`](../gui_occt_view.h) | Document access from `view.*`                                                    |
| [`shp.h`](../shp.h)                     | Shape wrapper type                                                               |
