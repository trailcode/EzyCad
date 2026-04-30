# 3D viewer: Open CASCADE and `Occt_view`

This page summarizes how EzyCad's 3D viewport relates to **Open CASCADE Technology (OCCT)** and how the **`Occt_view`** C++ class fits in. For everyday use (mouse, view roll, settings), start with **[usage.md](usage.md)** ([View Controls](usage.md#view-controls)) and **[usage-settings.md](usage-settings.md)**.

## Stack at a glance

| Piece | Role |
| --- | --- |
| **`V3d_View`** | OCCT 3D view: camera, projection, redraw. EzyCad keeps a `V3d_View` and drives pan/orbit/zoom through the interactive layer. |
| **`AIS_InteractiveContext`** | Displays `AIS_InteractiveObject` instances (e.g. `AIS_Shape`), selection, and view updates. |
| **`AIS_ViewController`** | OCCT helper for mouse-driven navigation; **`Occt_view`** subclasses it and forwards GLFW input (`on_mouse_*`, scroll, resize). |
| **`Occt_view`** | Application facade: document shapes and sketches, modes, undo/redo serialization, import/export, dimensions, and wrappers such as **`roll_view_z_deg`** (implemented with `V3d_View::Turn(..., V3d_Z, ...)`). |

The on-screen **triedron** and **view cube** (if enabled) are standard OCCT viewer features configured during **`Occt_view::init_default()`** and related setup.

## User-facing behavior

- **Mouse:** orbit (left drag), pan (middle drag), zoom (right drag or wheel), as described in [View Controls](usage.md#view-controls).
- **View roll:** **Shift+NumPad 4** / **Shift+NumPad 6** with step stored as **`gui.view_roll_step_deg`** (default **45** degrees). See [usage.md -> View roll](usage.md#view-roll).

## Settings on disk

3D **appearance** (background gradient, grid line colors, gradient mode) lives under JSON **`occt_view`**. **View roll step** is under **`gui`** because it is edited in **Settings -> 3D view navigation**, not inside the OCCT appearance blob.

Full key lists: **[usage-settings.md -> Settings file reference](usage-settings.md#settings-file-reference)** (`occt_view` and `gui` tables). **`view_roll_step_deg`** is documented there.

The scripting helper **`ezy.occt_view_settings_json()`** returns a small JSON fragment including **`occt_view`** and selected **`gui`** keys (see [scripting.md](scripting.md)); it may not include every `gui` field.

## `Occt_view` class (developers)

Source: **`src/occt_view.h`** (implementation **`src/occt_view.cpp`**).

**Occt_view** is the main entry point from **`GUI`** for everything tied to the 3D scene:

- **Lifecycle:** `init_window`, `init_viewer`, `init_default`, `do_frame`, `flush_view_events`, `cleanup`.
- **Document:** `to_json` / `load`, undo/redo, `new_file`, import/export helpers.
- **Sketches and shapes:** accessors, add primitives, sketch extrude entry, selection and delete.
- **Tool operations:** references to `Shp_move`, `Shp_rotate`, `Shp_extrude`, booleans, chamfer/fillet, etc.
- **Camera / view:** `get_camera` / `set_camera`, **`roll_view_z_deg`**, `fit_face_in_view`, `get_view_plane`, ray-based queries for sketch and dimensions.
- **Viewer chrome:** background and grid colors (`get_*` / `set_*` for gradient and grid); headless flag.

Private helpers manage AIS display, hit testing, and OCCT event mapping; **`m_view`** is the underlying **`V3d_View`**.

For API experiments or bindings, prefer extending **`Occt_view`** or **`GUI`** so OCCT types stay localized.

---

[Back to usage guide](usage.md) | [Settings](usage-settings.md)
