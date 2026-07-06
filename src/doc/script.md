# Script console module

Developer reference for embedded Lua and Python consoles. Headers: [`scr_lua_console.h`](../scr_lua_console.h), [`scr_python_console.h`](../scr_python_console.h).

Maintainers: update this file when script bindings, console UI, or startup script loading change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing scripting guide: [`docs/scripting.md`](../../docs/scripting.md).

## Purpose

The `scr_*` translation units embed **interactive script consoles** in the ImGui UI. They expose live `ezy.*` and `view.*` APIs to Lua and Python so users can inspect the document, add primitives, tweak the camera, and log messages while EzyCad runs.

| Class | File | Runtime |
| --- | --- | --- |
| `Lua_console` | `scr_lua_console.cpp` | Lua C API (native + Emscripten) |
| `Python_console` | `scr_python_console.cpp` | pybind11 embed (`EZYCAD_HAVE_PYTHON` only) |

Both are owned by `GUI` and rendered from `GUI::render_gui()` when feature tier allows (`show_lua_console_effective` / `show_python_console_effective`).

## Requirements and invariants

### Lifetime and threading

| Object | Notes |
| --- | --- |
| `Lua_console` / `Python_console` | Constructed with `GUI*`; must not outlive `GUI` |
| `lua_State* m_L` | Created in ctor, closed in dtor |
| Python runtime | `Ezycad_python_runtime` (pybind11) initialized in `init_python_()` |

Script callbacks run on the **main UI thread** during console input handling. Do not call OCCT or ImGui from worker threads without existing app synchronization.

### Script file locations

| Platform | Lua | Python |
| --- | --- | --- |
| Native | `res/scripts/lua/*.lua` | `res/scripts/python/*.py` |
| Emscripten | `/res/scripts/lua/*.lua` (preloaded) | Not built |

On startup, **Lua** files in that directory are loaded into tabbed `TextEditor` buffers and **executed once** to define helpers. Python tabs load file contents but are not auto-run on startup (run manually or from input line).

### Index conventions

| API | Index base |
| --- | --- |
| Lua `view.get_shape(i)` | **1-based**; returns `nil` out of range |
| Python `view.get_shape(i)` | **0-based**; raises `IndexError` out of range |

Document this when adding parallel bindings.

## Architecture

```
GUI::render_gui()
  +-- lua_console_()   -> Lua_console::render
  +-- python_console_() -> Python_console::render (if EZYCAD_HAVE_PYTHON)

Lua_console
  +-- lua_State + register_bindings()
  +-- m_script_editors (path, filename, TextEditor)
  +-- REPL input (m_input_buf) + m_history

Python_console
  +-- pybind11 embedded module ezycad_native
  +-- exec bootstrap sets ezy / view on __main__
  +-- m_script_editors + REPL input
```

CMake IDE group: `src\scr`.

## Binding surface

Implementation lives in the `.cpp` files; [`docs/scripting.md`](../../docs/scripting.md) is the user-facing API reference. Summary for developers:

### `ezy` table (both runtimes)

| Method | C++ delegate |
| --- | --- |
| `ezy.log(msg)` | `GUI::log_message` + console history |
| `ezy.msg(text)` | `GUI::show_message` |
| `ezy.get_mode()` | `GUI::get_mode()` -> mode name string |
| `ezy.set_mode(name)` | `GUI::set_mode(mode_from_string(...))` |
| `ezy.save_occt_view_settings()` | `GUI::save_occt_view_settings()` |
| `ezy.occt_view_settings_json()` | `GUI::occt_view_settings_json()` |
| `ezy.help()` / global `help()` | Prints binding summary to console |

Lua overrides global `print` to call `ezy.log`. Python bootstrap assigns `builtins.print` similarly.

### `view` table

| Method | Notes |
| --- | --- |
| `sketch_count()` | `Occt_view::get_sketches().size()` |
| `shape_count()` | `Occt_view::get_shapes().size()` |
| `curr_sketch_name()` | Current sketch display name |
| `add_box(...)` / `add_sphere(...)` | `Occt_view` primitive helpers |
| `get_camera()` | `{ eye, center, up }` tables / dicts |
| `set_camera(ex,ey,ez,cx,cy,cz,ux,uy,uz)` | `Occt_view::set_camera` |
| `get_shape(i)` | Returns `Shp` wrapper |

**Python only** (current sketch inspection):

| Method | Returns |
| --- | --- |
| `curr_sketch_node_count()` | Node count |
| `curr_sketch_node(i)` | `(x, y)` plane coords |
| `curr_sketch_dim_count()` | Length dimension count |
| `curr_sketch_dim(i)` | `(lo, hi, visible, offset, name, distance)` |

### `Shp` object

| Method | Lua | Python |
| --- | --- | --- |
| `name()` / `set_name(s)` | userdata metatable | `ezycad_native.Shp` methods |
| `visible()` / `set_visible(v)` | same | same |

Lua stores `Shp_ptr` in userdata with `__gc`; Python wraps `Ezy_shp{ Shp_ptr shp }`.

## Console UI flow

| UI region | Behavior |
| --- | --- |
| Output history | `m_history` lines (errors flagged) |
| Input line | `ImGui::InputTextWithHint`; Enter runs `execute()` |
| Command history | Up/down browses prior input lines |
| Script tabs | One tab per file in scripts dir; Save writes buffer to disk |

`Lua_console::text_edit_callback` / `Python_console::text_edit_callback` hook editor keyboard shortcuts (shared pattern with dist/angle edit).

## Adding a new binding

| Step | Lua | Python |
| --- | --- | --- |
| 1 | Add `l_*` C function using `get_gui(L)` | Add `m.def("view_foo", ...)` in `PYBIND11_EMBEDDED_MODULE` |
| 2 | Register in `register_bindings()` on `ezy` or `view` table | Expose on `view` in bootstrap `exec()` string |
| 3 | Update help text in `l_ezy_help` / `ezy_help` | same |
| 4 | Update [`docs/scripting.md`](../../docs/scripting.md) | same |

Prefer thin wrappers: validate args, call `GUI` / `Occt_view` / `Shp` methods, return simple types.

## Build flags

| Flag | Effect |
| --- | --- |
| (default) | Lua console always linked (bundled Lua sources) |
| `EZYCAD_HAVE_PYTHON` | Enables Python console, pybind11 embed, `python3.dll` copy on Windows |

If Python init fails, `Python_console` shows an error in history and disables execution.

## Testing

| Item | Notes |
| --- | --- |
| Automated API tests | Limited; bindings exercised manually and via `res/scripts/*` |
| Regression | Run sample lines from `docs/scripting.md` after binding changes |
| Lua on WASM | Verify `/res/scripts/lua` preload paths |

## Related code outside `src/scr*`

| Location | Role |
| --- | --- |
| [`gui.cpp`](../gui.cpp) | Owns consoles, `lua_console_()` / `python_console_()` |
| [`gui.h`](../gui.h) | `get_view()`, settings JSON, mode API used by bindings |
| [`mode.h`](../mode.h) | `mode_from_string`, `c_mode_strs` |
| [`gui_occt_view.h`](../gui_occt_view.h) | Document access from `view.*` |
| [`shp.h`](../shp.h) | Shape wrapper type |
