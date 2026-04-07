# EzyCad scripting (Lua and Python)

EzyCad can embed **Lua** and (on supported builds) **Python** consoles. They share the same idea: run short snippets against live **`ezy.*`** and **`view.*`** bindings while the app is running.

## Availability

| | Lua | Python |
| --- | --- | --- |
| **Desktop (Windows)** | Always (built-in Lua library) | Only if the app was **built with Python enabled** (`EZYCAD_HAVE_PYTHON` in CMake; needs Python development libraries). If the **View -> Python Console** item appears and works, your build includes it. |
| **WebAssembly** | Yes | No (not shipped in the browser build). |

## Opening the consoles

- **View -> Lua Console** - Toggles the Lua console and script editor tabs.
- **View -> Python Console** - Same layout for Python when the feature is compiled in.

Visibility is remembered with other panes (see **usage.md** -> *Help and Settings*).

## Layout

- **Output / history** - Results and errors from executed lines.
- **Input line** - Enter a fragment and run it (same as a one-off at the interactive prompt).
- **Script tabs** - Files under **`res/scripts/lua`** (`*.lua`) or **`res/scripts/python`** (`*.py`) are loaded as editable buffers; Lua scripts are also **run on startup** so you can define helpers (see `basic.lua` / `basic.py`).

Paths are relative to the app working directory on desktop; on WASM, Lua scripts are under the preloaded **`/res/scripts/lua`** tree.

## Bindings overview

Use **`ezy.help()`** or **`help()`** in either console for the built-in reminder text.

**`ezy`**

| Method | Purpose |
| --- | --- |
| `ezy.log(msg)` | Print to the console and the main **Log** pane |
| `ezy.msg(text)` | Show a status message (same style as GUI messages) |
| `ezy.get_mode()` | Current application mode name (string) |
| `ezy.set_mode(name)` | Switch mode by name |
| `ezy.help()` | Print binding summary |

**`view`**

| Method | Purpose |
| --- | --- |
| `view.sketch_count()` | Number of sketches |
| `view.shape_count()` | Number of 3D shapes |
| `view.curr_sketch_name()` | Name of the current sketch |
| `view.add_box(ox, oy, oz, w, l, h)` / `view.add_sphere(ox, oy, oz, r)` | Add primitive shapes |
| `view.get_shape(i)` | Shape by **1-based** index: Lua userdata or `nil`; Python `Shp` with `name`, `visible`, `set_visible`, etc. |

**Lua only:** global **`print`** is routed to **`ezy.log`**.  
**Python only:** **`print`** is similarly redirected to **`ezy.log`** after bootstrap.

## Sample scripts

- **`res/scripts/lua/basic.lua`** - Defines **`kv(obj)`** to dump tables or list userdata methods via **`ezy.log`**.
- **`res/scripts/python/basic.py`** - Example **`dump_view()`** using **`ezy.log`** and **`view`**.

Copy these as templates for your own files in the same folders.

## Limitations

- The API is **small** and may change between versions; prefer **`ezy.help()`** after upgrades.
- Scripting is for **automation and inspection**, not a full replacement for the GUI workflow.
- Heavy work in the console thread can **block the UI**; keep snippets short.

For general application behavior, see **[usage.md](usage.md)**.
