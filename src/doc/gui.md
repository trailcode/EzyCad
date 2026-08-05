# GUI module

Developer reference for EzyCad's Dear ImGui shell and input routing. The public C++ entry point is [`gui.h`](../gui.h).

Maintainers: update this file when GUI input routing, mode/options behavior, settings keys, or pane layout change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing UI guides live in [`docs/usage.md`](../../docs/usage.md), [`docs/usage-settings.md`](../../docs/usage-settings.md), and [`docs/usage-sketch.md`](../../docs/usage-sketch.md).

## Purpose

`GUI` owns the application window chrome (menus, toolbar, lists, options, settings, consoles) and **routes GLFW input** to the 3D viewer (`Occt_view` in [`gui_occt_view.h`](../gui_occt_view.h)) and to sketch/shape subsystems.

Typical responsibilities:

- ImGui frame: menu bar, dock space (passthrough central node for 3D input), toolbar, Sketch List, Shape List, Options, Settings, dist/angle popups.
- Mode switching (`Mode` enum in [`mode.h`](../mode.h)) and parent-mode Esc behavior.
- Persisted preferences (`ezycad_settings.json` via [`gui_settings.cpp`](../gui_settings.cpp)).
- Project I/O (`.ezy` load/save, import/export dialogs; **File -> Import** confirms STEP/PLY with **Import as** for hierarchy / flat / union). STEP **Import into project** shows an Importing modal; desktop uses `Atomic_progress_indicator` + background Transfer + Cancel; WASM paints the modal for two frames then runs Transfer on the main thread (no Cancel).
- CAD/mesh interchange scales about the origin: project display lengths follow **File -> Project units** (`Project_unit`; Inch or Millimeter). Model space stays inch-scaled (`model = inches * dimension_scale`). STEP import converts OCCT cascade **mm** into model space; PLY import treats coords as inches. **File -> Export** asks for **Inches** or **Millimeters** (STEP/IGES declare that unit; STL/PLY write unitless coords in that scale). `.ezy` persists `projectUnit`. **Settings -> New project defaults** stores `gui.default_project_unit` and inch-based default 2D framing for **File -> New**.
- Contextual help links (`doc_urls` in `gui.h`).

## Requirements and invariants

### Lifetime and ownership

| Object        | Owner                         | Notes                                                           |
| ------------- | ----------------------------- | --------------------------------------------------------------- |
| `GUI`         | `main` / Emscripten singleton | Constructs `std::unique_ptr<Occt_view> m_view` in constructor   |
| `Occt_view`   | `GUI::m_view`                 | Holds sketches, shapes, undo stacks, shape operation subobjects |
| `GLFWwindow*` | Passed to `GUI::init`         | Also wrapped by `Occt_glfw_win` inside the view                 |

`GUI` and `Occt_view` reference each other (`Occt_view(GUI& gui)`, `m_view->gui()`). Both must outlive the session.

### Mode changes

`GUI::set_mode` (in `gui_mode.cpp`):

| Step | Action                                          |
| ---- | ----------------------------------------------- |
| 1    | `cancel_underlay_calib_()`                      |
| 2    | Set `m_mode`, call `m_view->on_mode()`          |
| 3    | `sync_sketch_add_mid_pt_edges_if_applicable_()` |
| 4    | Update toolbar active state                     |

`set_parent_mode()` maps each tool mode back to `Normal` or `Sketch_inspection_mode` via `GUI::parent_mode_of` (see parent map in `gui_mode.cpp`). Undo/redo uses the same map so stored `Move` / `Rotate` / `Scale` restore their parent instead of re-entering the free-drag tool (see [undo-redo.md](undo-redo.md#mode-restoration)).

### New mode or toolbar command (hotkeys)

When adding a `Mode` to [`mode.h`](../mode.h) (`EZY_MODE_LIST`), a toolbar button, or a one-shot `Command`, update remappable hotkeys in the **same change**. Skip only for modes that must stay toolbar-only (document that choice).

| Step | Touch                                                                                                                                                                                                                          |
| ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 1    | `Gui_action` + default chord in [`gui_hotkeys.h`](../gui_hotkeys.h) / [`gui_hotkeys.cpp`](../gui_hotkeys.cpp) (`c_actions`; enum order)                                                                                        |
| 2    | `GUI::dispatch_hotkey_action_` in [`gui_mode.cpp`](../gui_mode.cpp)                                                                                                                                                            |
| 3    | `sync_toolbar_hotkey_tooltips_` in [`gui.cpp`](../gui.cpp)                                                                                                                                                                     |
| 4    | `gui.hotkeys` entry in [`res/ezycad_settings.json`](../../res/ezycad_settings.json)                                                                                                                                            |
| 5    | User docs: [usage.md](../../docs/usage.md#hotkeys) Modeling table; sketch tools also [usage-sketch.md](../../docs/usage-sketch.md#hotkeys); [usage-settings.md](../../docs/usage-settings.md) if labels change; `CHANGELOG.md` |

Pick a default that does not collide with existing `c_actions` chords or fixed keys (Esc, Enter, Tab, digits, unmodified X/Y/Z axis toggles — reserved via `is_reserved_chord`). Also wire parent-mode / Options / doc URL maps as usual for new modes.

`Occt_view::on_mode` also sets `AIS_ViewController::SetAllowHighlight(false)` for `Move` / `Rotate` / `Scale` (and `ClearDetected`) so idle mouse moves do not run dynamic `MoveTo` while transform preview leaves selection BVHs at the pre-transform pose; other modes restore highlight. Orbit/pan still receive `UpdateMousePosition` when buttons are held. When LMB finalizes an active transform (operands loaded), `on_mouse_button` skips `PressMouseButton` / `ReleaseMouseButton` for that click so AIS `SelectDetected` on release cannot replace the restored multi-selection with the single shape under the cursor.

Because the faint/selection-mode redisplay `Erase`s shapes (dropping the AIS selection), `on_mode` snapshots the entered selection for `Move` / `Rotate` / `Scale` / `Shape_cross_section` and restores it via `Occt_view::set_selected_shps` after `sync_sketch_shape_faint_style()`. The same helper restores the operands when a transform tool finishes (see `restore_operation_selection_` in [shape.md](shape.md)); `Occt_view::cancel` therefore treats `Move` / `Rotate` / `Scale` as already handled instead of switching mode again. Transform tools are also seeded via `Shp_move` / `Shp_rotate` / `Shp_scale::begin(enter_selection)` so multi-select operands do not depend on AIS selection surviving the mode switch (`ensure_operation_shps_` still falls back to the AIS selection when the seed was empty). Move/Rotate/Scale use `TopAbs_SHAPE` (whole-object) selection mode.

`Occt_view::on_mode` ends with `sync_sketch_shape_faint_style()`: while `is_sketch_mode`, document shapes follow **Options/Settings** `gui.sketch_shape_faint_enabled` (master) plus `gui.sketch_shape_faint_style` (**0** hide / **1** ghost / **2** wire) and `gui.sketch_shape_faint_opacity` (ghost). Outside sketch mode, faint overrides clear and shapes show at full strength (unless Shape List **Hide all**). New shapes call the same sync from `add_shp_` / `insert_shape_rec`. The **Faint shapes** checkbox lives in `options_sketch_common_` (every sketch tool).

### UI verbosity

`gui.ui_verbosity` gates panes and help (Settings slider). Derived tiers on `GUI`:

| API                                  | Meaning                                                             |
| ------------------------------------ | ------------------------------------------------------------------- |
| `ui_feature_tier()`                  | `(verbosity + 1) / 2` -- Options, lists, log at tier 1+             |
| `ui_help_tier()`                     | `verbosity / 2` -- contextual help depth                            |
| `ui_show_contextual_help()`          | `verbosity >= 5` -- `?` buttons and doc links                       |
| `show_*_effective()`                 | Pane flag AND feature tier                                          |
| `ui_show_occt_line_width_settings()` | `false` on wasm (GLES ignores OCCT `SetWidth` / line-width sliders) |

Constants and ranges live in `gui.h` (`k_gui_ui_verbosity_*`, dimension defaults, view roll/zoom ranges, default 2D view size).

### Dist / angle edit popups

Tab and Shift+Tab in the 3D view open numeric entry via `GUI::set_dist_edit` / `set_angle_edit` (sketch / extrude). Move, Rotate, and Align shafts (`Shape_shaft_align`) skip that path and handle Tab in their mode key handlers (`show_dist_edit` / `show_angle_edit` / depth and twist edits). The angle popup shows a `deg` suffix. While active (`is_dist_or_angle_edit_active()`), keys route to `on_key()` instead of ImGui text fields (`main` checks this).

## Architecture

```
GUI (gui.h / gui.cpp)
  |
  +-- gui_mode.cpp       set_mode, on_key, Options panel per Mode
  +-- gui_hotkeys.*      remappable Gui_action <-> Key_chord map
  +-- gui_add.cpp        Add menu dialogs (primitives, new sketch)
  +-- gui_settings.cpp   Settings dialog, load/save ezycad_settings.json
  |
  +-- Occt_view (gui_occt_view.h / gui_occt_view.cpp / .inl)
  |     +-- gui_occt_glfw_win.*   GLFW Aspect_Window wrapper
  |     +-- sketch*, shp_* ops, document I/O, undo
  |
  +-- scr_lua_console / scr_python_console (scripting UI)
```

CMake IDE group: `src\gui` (files matching `gui*` or `occt*` prefix).

## ImGui docking and viewports

Dear ImGui **docking branch** (`third_party/imgui`, tag `v1.92.7-docking`) is vendored with `IMGUI_HAS_DOCK`.

| Platform | Flags                              | Behavior                                                     |
| -------- | ---------------------------------- | ------------------------------------------------------------ |
| Native   | `DockingEnable`, `ViewportsEnable` | In-canvas dock/tab/split; panels may detach to OS windows    |
| WASM     | `DockingEnable`                    | In-canvas dock/tab/split only (no multi-viewport OS windows) |

### WASM HiDPI / canvas sizing

Shared `#canvas` is used by ImGui and OCCT. Model follows the physical-pixel approach from
[imgui#7519](https://github.com/ocornut/imgui/issues/7519#issuecomment-2629628233):

| Piece                                                           | Role                                                                                               |
| --------------------------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| HTML CSS (`width`/`height: 100%`)                               | On-screen (CSS) size of the canvas                                                                 |
| `ImGui_ImplGlfw_OnCanvasSizeChange` (vendored patch)            | Sets `canvas.width/height` and GLFW window to `CSS * devicePixelRatio`                             |
| ImGui `main_scale`                                              | `devicePixelRatio` via `ScaleAllSizes` / `FontScaleDpi` so widgets keep CSS-logical on-screen size |
| `io.DisplayFramebufferScale`                                    | Stays `1` (window size == framebuffer size in physical px) - fonts/icons are not upscaled          |
| OCCT `Wasm_Window("#canvas", false)` + `SetDevicePixelRatio(1)` | Shares the ImGui-sized backing store; mouse/view coords match GLFW                                 |

Do **not** set `GLFW_SCALE_TO_MONITOR` on wasm: Emscripten then forces canvas CSS size to the GLFW window size (`!important`), which fights the 100% CSS layout.

Startup: dispatch a couple of `resize` events so the CSS*DPR sync runs after the browser CSS size settles.

Initialization in [`main.cpp`](../main.cpp): config flags, native-only `UpdatePlatformWindows` / `RenderPlatformWindowsDefault` after the main draw pass.

With `ViewportsEnable`, ImGui `MousePos` is in screen coordinates; OCCT picking uses GLFW client-area coordinates via `GUI::cursor_screen_coords()` (see `on_mouse_button` and the `main` cursor callback). Do not pass ImGui `MousePos` to `Occt_view` on native builds.

In [`main.cpp`](../main.cpp), GLFW **mouse-move** callbacks always forward to `GUI` (sketch rubber-band and OCCT hover must not stop when a float edit or docked panel is hovered). **Mouse-button press** and **scroll** forward only when the cursor is in the dock central passthrough region and no ImGui window is hovered (so toolbar clicks do not clear OCCT selection). A press that is forwarded sets per-button capture so the matching **release** is still sent to `GUI` / OCCT even if the cursor is over a pane (ends view orbit / AIS button state). Releases with no matching view press stay ImGui-only.

Each frame, [`gui.cpp`](../gui.cpp) `dock_space_()` sets the OCCT passthrough rectangle via `DockSpaceOverViewport` with `ImGuiDockNodeFlags_PassthruCentralNode`, then reads the central node bounds. `dock_space_()` calls `SetNextFrameWantCaptureMouse(false)` when the cursor is over the passthrough region.

Default dock layout (left: Shape/Sketch lists tabbed, right: Options, bottom: Log) is seeded once via `DockBuilder*` when loaded `imgui_ini` has no `[Docking]` section (`m_seed_default_dock_layout` in `gui_settings.cpp`). The Toolbar uses `ImGuiWindowFlags_NoDocking` so it cannot occupy the central passthrough node.

Overlay popups (`FloatEdit`, `AngleEdit`, `MessageStatus`, modals) keep `NoSavedSettings` and do not participate in docking.

## Input routing (GLFW -> `GUI` -> downstream)

`main` forwards GLFW callbacks to `GUI`. The view receives mouse events first in some paths (`on_mouse_button` calls `m_view->on_mouse_button` before `on_left_click_`).

### Keyboard (`GUI::on_key` in `gui_mode.cpp`)

Remappable chords live in `Gui_hotkeys` (`gui_hotkeys.h` / `.cpp`), owned by `GUI::m_hotkeys`. Stable action ids (e.g. `mode.move`, `mode.add_edge`, `cmd.shape_cut`, `edit.undo`) map to `Key_chord { key, mods }`. Persistence: `gui.hotkeys` in `ezycad_settings.json` as human-readable strings (`"G"`, `"Shift+L"`, `"Ctrl+Shift+C"`); missing keys merge to built-in defaults. On load, `merge_from_json` drops reserved/invalid chords, then resolves duplicate chords: later actions reset to factory; if that factory chord is still held by an earlier remap, that earlier row is also restored to factory (defaults are unique, so `action_for` never keeps a silent collision). Settings **Keyboard shortcuts** captures the next `GLFW_PRESS` (Esc cancels; `set_chord` rejects conflicts and **reserved** fixed chords via `is_reserved_chord`). Per-row **Reset** calls `reset_action` (factory chord via `set_chord`, so duplicates are rejected with the same inline conflict message). Capture is cleared when Settings closes. Toolbar tooltips for remappable modes and boolean commands are rebuilt via `sync_toolbar_hotkey_tooltips_()`.

| Input                             | Condition                    | Handler                                                                                                     |
| --------------------------------- | ---------------------------- | ----------------------------------------------------------------------------------------------------------- |
| `+` / `-` / numpad +/-            | No Ctrl/Alt                  | `Occt_view::zoom_view_wheel_notches`                                                                        |
| Shift + 4/6 / arrows / numpad 4/6 | No Ctrl/Alt                  | `Occt_view::roll_view_z_deg`                                                                                |
| Numpad 5                          | No modifiers                 | `Occt_view::snap_view_to_nearest_standard_axis`                                                             |
| Numpad 2/4/6/8                    | No modifiers                 | `Occt_view::orbit_view_screen_step_deg`                                                                     |
| Hotkey capture active             | Settings                     | `try_capture_hotkey_press_` (assign / Esc cancel / conflict message)                                        |
| `1`-`9` / numpad `1`-`9`          | `Mode::Normal` only          | `set_shp_selection_mode` (TopAbs enum index); fixed                                                         |
| Esc                               |                              | cancel capture if listening; else `cancel_underlay_calib_`, `Occt_view::cancel`, hide dist/angle edit       |
| Tab                               | not Move/Rotate/Align shafts | `Occt_view::dimension_input`; those modes: `break` into mode handlers                                       |
| Shift+Tab                         | not Move/Rotate/Align shafts | `Occt_view::angle_input`; those modes: `break` into mode handlers (cyl-align: clock/angle)                  |
| Enter                             | not Rotate/Align shafts      | hide edits, `Occt_view::on_enter`; those modes: `break` into mode handlers (finalize)                       |
| Delete / Backspace                |                              | `Occt_view::delete_selected` (fixed aliases; remapping `edit.delete` does not remove these)                 |
| Ctrl+Shift+Z                      |                              | `Occt_view::redo` (fixed second redo; remappable `edit.redo` defaults to Ctrl+Y)                            |
| Remappable chord                  | `m_hotkeys` hit              | `dispatch_hotkey_action_` (`Gui_action`: sketch/shape modes, booleans, delete, copy/paste, file, undo/redo) |
| Move-mode keys                    | `Mode::Move`                 | `on_key_move_mode_` (axis constraints X/Y/Z); hardcoded                                                     |
| Rotate-mode keys                  | `Mode::Rotate`               | `on_key_rotate_mode_` (axis pick, Tab angle); hardcoded                                                     |
| Align-shafts keys                 | `Mode::Shape_shaft_align`      | `on_key_cyl_align_mode_` (Tab depth, Shift+Tab clock/angle, Enter finalize); hardcoded                      |

Default remappable chords include G/R/S/J/E/C/F/D shape tools; sketch tools N/L/A/Q/B/O/U/I/P and Shift variants; Shift+P polar, Shift+X cross-section; Ctrl+Shift+C/F/M booleans; Shift+D delete; Ctrl+C / Ctrl+V copy/paste (in-app shape clipboard); Ctrl+N/O/S; Ctrl+Z / Ctrl+Y. Unmodified X/Y/Z are reserved for Move/Rotate axis toggles (`is_reserved_chord`); Shift+X remains free for cross-section. Remappable keys must pass `is_bindable_key` (letters, digits, Space, and named keys that round-trip in settings JSON); punctuation such as `,` / `.` and numpad keys are rejected. Settings **Keyboard shortcuts** has a `?` to `doc_urls::k_hotkeys` ([usage-settings.md#keyboard-shortcuts](../../docs/usage-settings.md#keyboard-shortcuts)).

**In-app shape clipboard** (`edit.copy` / `edit.paste`): `Occt_view::copy_selected_shapes` / `paste_clipboard_shapes` hold a session `Shape_rec` forest (not the OS clipboard). Copy roots are selected solids, or the current group when its descendant solids match the selection exactly (Shape List group click). Paste deep-copies under `current_group_id` with new ids, uniquified names, and a single `Shape_add_delta`. When the current group is still a copied group root, paste inserts as a **sibling** of that group (not nested under it). Survives `new_file` (copy -> New -> paste); live source-root ids are cleared on New. Sketch geometry is out of scope; ImGui text widgets keep OS text clipboard via `WantTextInput`.

See also [`src/doc/sketch.md`](sketch.md) and [`src/doc/shape.md`](shape.md) for per-mode mouse routing after `GUI` delegates to `Occt_view`.

### Mouse move (`GUI::on_mouse_pos`)

| `Mode`                                              | Delegate                                    |
| --------------------------------------------------- | ------------------------------------------- |
| `Move`                                              | `shp_move().move_selected`                  |
| `Rotate`                                            | `shp_rotate().rotate_selected`              |
| `Scale`                                             | `shp_scale().scale_selected`                |
| `Shape_shaft_align`                                   | `shp_cyl_align().drag_depth` / `drag_twist` |
| `Shape_polar_duplicate`                             | `shp_polar_dup().move_point`                |
| Sketch tool modes (line, arc, rect, dim, axis, ...) | `curr_sketch().sketch_pt_move`              |
| `Sketch_face_extrude`                               | `sketch_face_extrude(..., true)`            |

Always calls `m_view->on_mouse_move(screen_coords)` first.

### Mouse buttons (`GUI::on_mouse_button` + `on_left_click_`)

| Event                       | Handler                                                                                                                                                             |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| LMB (underlay calib active) | `try_underlay_calib_click_` (early return)                                                                                                                          |
| LMB                         | `m_view->on_mouse_button` then `on_left_click_` (skipped when extrude LMB already advanced/finalized the session)                                                   |
| RMB press                   | `finalize_elm` for line / multi-line sketch modes                                                                                                                   |
| LMB in `on_left_click_`     | Mode-specific: transform finalize, cyl-align face pick / depth->clock / finalize, sketch `add_sketch_pt`, fillet/chamfer click, polar dup `add_point`, extrude pick |

Tests use `sketch_left_click` to simulate sketch LMB without ImGui mouse position.

### Scroll / resize

| Callback          | Delegate                                          |
| ----------------- | ------------------------------------------------- |
| `on_mouse_scroll` | `Occt_view::on_mouse_scroll` (Shift = finer zoom) |
| `on_resize`       | `Occt_view::on_resize`                            |

## Options panel dispatch

`GUI::options_()` switches on `get_mode()`:

| `Mode`                           | Options function                                                                                                    |
| -------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `Normal`                         | `options_normal_mode_` (selection filter, orthographic)                                                             |
| `Move` / `Rotate` / `Scale`      | `options_*_mode_` (constraints, axis, material)                                                                     |
| `Shape_shaft_align`                | `options_Shape_shaft_align_mode_` (Flip direction, Clock rotation; pick / depth / clock help)                         |
| `Shape_chamfer` / `Shape_fillet` | mode + radius/distance                                                                                              |
| `Shape_polar_duplicate`          | angle, count, rotate/combine, **Dup** button                                                                        |
| `Shape_cross_section`            | local XY/XZ/YZ, invert normal, hide back side, show section outline, bbox-ranged offset, Clip, Cross section sketch |
| `Sketch_inspection_mode`         | `options_sketch_common_`                                                                                            |
| Each sketch tool mode            | Matching `options_sketch_*_mode_`                                                                                   |
| `Sketch_operation_axis`          | Mirror / Revolve / Clear axis                                                                                       |
| `Sketch_face_extrude`            | Both sides, Twist, material; help mentions Settings fast preview                                                    |

Shared sketch controls (snap, midpoint nodes, place-from-center) live in `options_sketch_common_` and helpers in `gui_mode.cpp`.

## ImGui frame order (`render_gui`)

| Order | Function                                                      | Purpose                               |
| ----- | ------------------------------------------------------------- | ------------------------------------- |
| 1     | `flush_view_events`                                           | Sync camera before UI uses projection |
| 2     | `menu_bar_`, `toolbar_`                                       | File / View / mode tools              |
| 3     | `dist_edit_`, `angle_edit_`                                   | Floating numeric entry                |
| 4     | `sketch_list_`, `sketch_properties_dialog_`                   | Sketch List + underlay/properties     |
| 5     | `shape_list_`, `shape_info_dialog_`, `file_inspector_dialog_` | Shape List + info + Import dialog     |
| 6     | `options_`                                                    | Mode-specific Options pane            |
| 7     | `message_status_window_`, `about_dialog_`                     | Status + About                        |
| 8     | `add_*_dialog_`                                               | Primitive / sketch creation popups    |
| 9     | `log_window_`, consoles, `settings_`, `dbg_`                  | Log, Lua/Python, Settings             |

`GUI::show_message` drives the transient status toast (`message_status_window_`) and also appends via `log_message`. `show_error_dialog` logs `title: message` once and toasts the title only.

Sketch List expand **Faces**: each face row supports **`E`** and right-click **Extrude** via `GUI::sketch_list_extrude_face_` (`set_mode(Sketch_face_extrude)` + `Occt_view::begin_sketch_face_extrude` / `Shp_extrude::begin_face_extrude`). Hovering a **Faces**, **Edges**, or **Nodes** row calls `Occt_view::set_sketch_list_hover_{face,edge,node}` (temporarily displays the AIS when hidden outside sketch modes; uses `Graphic3d_ZLayerId_Topmost` so solids do not occlude the highlight).

**Shape List outliner:** `shape_list_` draws a tree of document shapes/groups via `shape_children(0)` and recursive `TreeNodeEx` rows. Fixed-width vis/disp/mat columns are on the left; the name column stretches on the right with tree indent (`IndentEnable` on name only). An empty pad row after the last item is a drag-drop target for document root (`reparent_shape(..., 0)`); it shows a "Move to root" hint while dragging. Groups support expand/collapse (`ui.shapeList.expanded`), drag-drop reparent (`EZY_SHAPE_ID` payload), Group / New group / Ungroup, and cascade delete. Clicking a group sets `Occt_view::current_group_id` (including empty groups) and selects descendant solids; clicking a solid selects it and sets current group to its parent. New primitives/extrudes/revolves parent under the current group. Ctrl+click multi-selects. Copy/paste (Ctrl+C/V) deep-copies the current group subtree when the selection matches that group's descendant solids. Context menu **Zoom to** calls `Occt_view::fit_shapes_in_view` (solid or group descendant solids; keeps camera orientation). Hover uses `set_shape_list_hover` on leaf solids only. `ui.shapeList.currentGroupId` is persisted in `.ezy`.

**Sketch List UI in the project file:** `GUI::serialized_project_json_` writes `ui.sketchList` (scroll Y plus per-sketch `rows` keyed by sketch `id`: `expanded`, `dimensions`, `nodes`, `edges`, `faces`). `GUI::on_file` restores via `apply_sketch_list_ui_from_json_`. Subsection open state is app-owned (`Sketch_list_row_ui` + `SetNextItemOpen`), not ImGui ini storage.

3D redraw: `render_occt()` -> `Occt_view::do_frame()` (separate from ImGui pass).

## Settings and persistence

| File                        | Role                                                                |
| --------------------------- | ------------------------------------------------------------------- |
| `gui_settings.cpp`          | Settings dialog UI; read/write `ezycad_settings.json`               |
| `save_occt_view_settings`   | Persists `gui.*`, `occt_view.*`, pane visibility, last project path |
| `load_occt_view_settings_`  | Called from `GUI::init`                                             |
| `occt_view_settings_json()` | Scripting API for settings blob                                     |

Sketch edge/face display colors live under `gui.sketch_edge_*` / `gui.sketch_face_*` and are applied live via `Sketch_annotation_refresh::edge_face_style`. Sketch-mode shape ghost/wire uses `gui.sketch_shape_faint_style` / `gui.sketch_shape_faint_opacity` via `Occt_view::sync_sketch_shape_faint_style`. 3D shape selection highlight uses `gui.shape_selection_color` applied through `Occt_view::apply_shape_selection_style` (`AIS_InteractiveContext::SelectionStyle`). Settings collapsing-header open state is stored in `gui.settings_headers` (Sketch nests **Appearance**, **Dimensions**, **Nodes**, **Snap**, **Underlay**; also **Keyboard shortcuts** / `hotkeys`). Remappable chords: `gui.hotkeys` object via `Gui_hotkeys::to_json` / `merge_from_json`.

User-visible key tables: [`docs/usage-settings.md`](../../docs/usage-settings.md). When adding a Settings control, follow [agents/conventions/user-docs-sync.md](../../agents/conventions/user-docs-sync.md).

## Toolbar and one-shot commands

Toolbar buttons hold `std::variant<Mode, Command>`. `Command` (`Shape_cut`, `Shape_fuse`, `Shape_common`) runs immediately on click via `shp_cut` / `shp_fuse` / `shp_common` (no persistent mode).

Mode buttons call `set_mode`. Active state tracks `m_mode`. Entering Move / Rotate / Scale / cross-section snapshots selected solids before selection-mode and sketch-faint redisplay (those Erase AIS selection) and restores them afterward so pre-selection is honored.

The cross-section toolbar button enters `Mode::Shape_cross_section`. After the shared selection restore, it calls `Shp_cross_section::preview` (blocking) with the snapshot. While the mode is active, Options updates the yellow plane annotation immediately on plane/offset/hide-back changes (`request_preview`), then `poll`s a background section job (desktop `std::async`; WASM chunks one solid per frame). At most one running job plus one pending (latest only); moving the slider cancels/coalesces work so the UI stays responsive. **Hide back side** attaches a temporary per-shape `Graphic3d_ClipPlane` for display only. **Clip** runs a half-space `BRepAlgoAPI_Common`, deletes the input solids, and adds clipped replacements (`Shape_replace_delta`). **Cross section sketch** imports cached section line/circle edges into a new sketch (`Sketch_struct_delta::Add`) and switches to sketch inspection. Temporary AIS and jobs are cleared when the mode is left, when the selection becomes empty, after a successful **Clip**, or on `clear()`.

## Typical developer usage

### Drive sketch input from tests

```cpp
GUI_access::sketch_left_click(gui, ScreenCoords(dvec2(x, y)));
// or GUI::sketch_left_click when GUI is accessible
```

### Open dist edit from a tool

```cpp
gui.set_dist_edit(dist, [view](float v, bool is_final) {
  // apply v; if (is_final) finalize
});
```

### Read current mode / view

```cpp
Mode m = gui.get_mode();
Occt_view* view = gui.get_view();
```

## Testing

| Item                   | Notes                                                |
| ---------------------- | ---------------------------------------------------- |
| Sketch tests           | `GUI_access` friend in `tests/skt_test_fixture.*`    |
| Headless / partial GUI | `sketch_left_click`, message getters on `GUI_access` |
| Full UI                | Manual smoke; no dedicated `gui_tests` target        |

## Related code outside `src/gui*`

| Location                                                                                                | Role                                                             |
| ------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- |
| [`mode.h`](../mode.h)                                                                                   | `Mode`, `Fillet_mode`, `Chamfer_mode`, sketch/shape mode helpers |
| [`main.cpp`](../main.cpp)                                                                               | GLFW callbacks -> `GUI`; dist/angle edit key guard               |
| [`src/doc/sketch.md`](sketch.md)                                                                        | Sketch methods called from `GUI` / `Occt_view`                   |
| [`src/doc/shape.md`](shape.md)                                                                          | Shape operations invoked from toolbar, Options, mouse            |
| [`scr_lua_console.cpp`](../scr_lua_console.cpp) / [`scr_python_console.cpp`](../scr_python_console.cpp) | Script consoles embedded in `render_gui`                         |
| [`utl_settings.cpp`](../utl_settings.cpp)                                                               | User settings file path and I/O helpers                          |
| [`utl_cad_file_info.h`](../utl_cad_file_info.h)                                                         | CAD/mesh file metadata for **File -> Import**                    |
