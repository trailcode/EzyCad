# GUI module

Developer reference for EzyCad's Dear ImGui shell and input routing. The public C++ entry point is [`gui.h`](../gui.h).

Maintainers: update this file when GUI input routing, mode/options behavior, settings keys, or pane layout change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing UI guides live in [`docs/usage.md`](../../docs/usage.md), [`docs/usage-settings.md`](../../docs/usage-settings.md), and [`docs/usage-sketch.md`](../../docs/usage-sketch.md).

## Purpose

`GUI` owns the application window chrome (menus, toolbar, lists, options, settings, consoles) and **routes GLFW input** to the 3D viewer (`Occt_view` in [`gui_occt_view.h`](../gui_occt_view.h)) and to sketch/shape subsystems.

Typical responsibilities:

- ImGui frame: menu bar, dock space (passthrough central node for 3D input), toolbar, Sketch List, Shape List, Options, Settings, dist/angle popups.
- Mode switching (`Mode` enum in [`mode.h`](../mode.h)) and parent-mode Esc behavior.
- Persisted preferences (`ezycad_settings.json` via [`gui_settings.cpp`](../gui_settings.cpp)).
- Project I/O (`.ezy` load/save, import/export dialogs).
- Contextual help links (`doc_urls` in `gui.h`).

## Requirements and invariants

### Lifetime and ownership

| Object | Owner | Notes |
| --- | --- | --- |
| `GUI` | `main` / Emscripten singleton | Constructs `std::unique_ptr<Occt_view> m_view` in constructor |
| `Occt_view` | `GUI::m_view` | Holds sketches, shapes, undo stacks, shape operation subobjects |
| `GLFWwindow*` | Passed to `GUI::init` | Also wrapped by `Occt_glfw_win` inside the view |

`GUI` and `Occt_view` reference each other (`Occt_view(GUI& gui)`, `m_view->gui()`). Both must outlive the session.

### Mode changes

`GUI::set_mode` (in `gui_mode.cpp`):

| Step | Action |
| --- | --- |
| 1 | `cancel_underlay_calib_()` |
| 2 | Set `m_mode`, call `m_view->on_mode()` |
| 3 | `sync_sketch_add_mid_pt_edges_if_applicable_()` |
| 4 | Update toolbar active state |

`set_parent_mode()` maps each tool mode back to `Normal` or `Sketch_inspection_mode` (see parent map in `gui_mode.cpp`).

### UI verbosity

`gui.ui_verbosity` gates panes and help (Settings slider). Derived tiers on `GUI`:

| API | Meaning |
| --- | --- |
| `ui_feature_tier()` | `(verbosity + 1) / 2` -- Options, lists, log at tier 1+ |
| `ui_help_tier()` | `verbosity / 2` -- contextual help depth |
| `ui_show_contextual_help()` | `verbosity >= 5` -- `?` buttons and doc links |
| `show_*_effective()` | Pane flag AND feature tier |

Constants and ranges live in `gui.h` (`k_gui_ui_verbosity_*`, dimension defaults, view roll/zoom ranges).

### Dist / angle edit popups

Tab and Shift+Tab in the 3D view open numeric entry via `GUI::set_dist_edit` / `set_angle_edit`. While active (`is_dist_or_angle_edit_active()`), keys route to `on_key()` instead of ImGui text fields (`main` checks this).

## Architecture

```
GUI (gui.h / gui.cpp)
  |
  +-- gui_mode.cpp       set_mode, on_key, Options panel per Mode
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

| Platform | Flags | Behavior |
| --- | --- | --- |
| Native | `DockingEnable`, `ViewportsEnable` | In-canvas dock/tab/split; panels may detach to OS windows |
| WASM | `DockingEnable` | In-canvas dock/tab/split only (no multi-viewport OS windows) |

### WASM HiDPI / canvas sizing

Shared `#canvas` is used by ImGui and OCCT. Model follows the physical-pixel approach from
[imgui#7519](https://github.com/ocornut/imgui/issues/7519#issuecomment-2629628233):

| Piece | Role |
| --- | --- |
| HTML CSS (`width`/`height: 100%`) | On-screen (CSS) size of the canvas |
| `ImGui_ImplGlfw_OnCanvasSizeChange` (vendored patch) | Sets `canvas.width/height` and GLFW window to `CSS * devicePixelRatio` |
| ImGui `main_scale` | `devicePixelRatio` via `ScaleAllSizes` / `FontScaleDpi` so widgets keep CSS-logical on-screen size |
| `io.DisplayFramebufferScale` | Stays `1` (window size == framebuffer size in physical px) - fonts/icons are not upscaled |
| OCCT `Wasm_Window("#canvas", false)` + `SetDevicePixelRatio(1)` | Shares the ImGui-sized backing store; mouse/view coords match GLFW |

Do **not** set `GLFW_SCALE_TO_MONITOR` on wasm: Emscripten then forces canvas CSS size to the GLFW window size (`!important`), which fights the 100% CSS layout.

Startup: dispatch a couple of `resize` events so the CSS*DPR sync runs after the browser CSS size settles.

Initialization in [`main.cpp`](../main.cpp): config flags, native-only `UpdatePlatformWindows` / `RenderPlatformWindowsDefault` after the main draw pass.

With `ViewportsEnable`, ImGui `MousePos` is in screen coordinates; OCCT picking uses GLFW client-area coordinates via `GUI::cursor_screen_coords()` (see `on_mouse_button` and the `main` cursor callback). Do not pass ImGui `MousePos` to `Occt_view` on native builds.

In [`main.cpp`](../main.cpp), GLFW **mouse-move** callbacks always forward to `GUI` (sketch rubber-band and OCCT hover must not stop when a float edit or docked panel is hovered). **Mouse-button** and **scroll** callbacks forward only when the cursor is in the dock central passthrough region and no ImGui window is hovered (so toolbar clicks do not clear OCCT selection).

Each frame, [`gui.cpp`](../gui.cpp) `dock_space_()` sets the OCCT passthrough rectangle via `DockSpaceOverViewport` with `ImGuiDockNodeFlags_PassthruCentralNode`, then reads the central node bounds. `dock_space_()` calls `SetNextFrameWantCaptureMouse(false)` when the cursor is over the passthrough region.

Default dock layout (left: Shape/Sketch lists tabbed, right: Options, bottom: Log) is seeded once via `DockBuilder*` when loaded `imgui_ini` has no `[Docking]` section (`m_seed_default_dock_layout` in `gui_settings.cpp`). The Toolbar uses `ImGuiWindowFlags_NoDocking` so it cannot occupy the central passthrough node.

Overlay popups (`FloatEdit`, `AngleEdit`, `MessageStatus`, modals) keep `NoSavedSettings` and do not participate in docking.

## Input routing (GLFW -> `GUI` -> downstream)

`main` forwards GLFW callbacks to `GUI`. The view receives mouse events first in some paths (`on_mouse_button` calls `m_view->on_mouse_button` before `on_left_click_`).

### Keyboard (`GUI::on_key` in `gui_mode.cpp`)

| Input | Condition | Handler |
| --- | --- | --- |
| `+` / `-` / numpad +/- | No Ctrl/Alt | `Occt_view::zoom_view_wheel_notches` |
| Shift + 4/6 / arrows / numpad 4/6 | No Ctrl/Alt | `Occt_view::roll_view_z_deg` |
| Numpad 5 | No modifiers | `Occt_view::snap_view_to_nearest_standard_axis` |
| Numpad 2/4/6/8 | No modifiers | `Occt_view::orbit_view_screen_step_deg` |
| Ctrl+N/O/S | | `new_project_` / `open_file_dialog_` / `save_file_dialog_` |
| Ctrl+Z / Ctrl+Shift+Z / Ctrl+Y | | `Occt_view::undo` / `redo` |
| `1`-`9` / numpad `1`-`9` | `Mode::Normal` only | `set_shp_selection_mode` (TopAbs enum index) |
| Esc | | `cancel_underlay_calib_`, `Occt_view::cancel`, hide dist/angle edit |
| Tab | | `Occt_view::dimension_input` |
| Shift+Tab | | `Occt_view::angle_input` |
| Enter | | hide edits, `Occt_view::on_enter` |
| D | | `Mode::Sketch_dim_anno` |
| Shift+D / Delete / Backspace | | `Occt_view::delete_selected` |
| G / R / E / S / C / F | | Move / Rotate / Extrude / Scale / Chamfer / Fillet modes |
| Move-mode keys | `Mode::Move` | `on_key_move_mode_` (axis constraints X/Y/Z) |
| Rotate-mode keys | `Mode::Rotate` | `on_key_rotate_mode_` (axis pick, Tab angle) |

See also [`src/doc/sketch.md`](sketch.md) and [`src/doc/shape.md`](shape.md) for per-mode mouse routing after `GUI` delegates to `Occt_view`.

### Mouse move (`GUI::on_mouse_pos`)

| `Mode` | Delegate |
| --- | --- |
| `Move` | `shp_move().move_selected` |
| `Rotate` | `shp_rotate().rotate_selected` |
| `Scale` | `shp_scale().scale_selected` |
| `Shape_polar_duplicate` | `shp_polar_dup().move_point` |
| Sketch tool modes (line, arc, rect, dim, axis, ...) | `curr_sketch().sketch_pt_move` |
| `Sketch_face_extrude` | `sketch_face_extrude(..., true)` |

Always calls `m_view->on_mouse_move(screen_coords)` first.

### Mouse buttons (`GUI::on_mouse_button` + `on_left_click_`)

| Event | Handler |
| --- | --- |
| LMB (underlay calib active) | `try_underlay_calib_click_` (early return) |
| LMB | `m_view->on_mouse_button` then `on_left_click_` |
| RMB press | `finalize_elm` for line / multi-line sketch modes |
| LMB in `on_left_click_` | Mode-specific: transform finalize, sketch `add_sketch_pt`, fillet/chamfer click, polar dup `add_point`, extrude pick |

Tests use `sketch_left_click` to simulate sketch LMB without ImGui mouse position.

### Scroll / resize

| Callback | Delegate |
| --- | --- |
| `on_mouse_scroll` | `Occt_view::on_mouse_scroll` (Shift = finer zoom) |
| `on_resize` | `Occt_view::on_resize` |

## Options panel dispatch

`GUI::options_()` switches on `get_mode()`:

| `Mode` | Options function |
| --- | --- |
| `Normal` | `options_normal_mode_` (selection filter, orthographic) |
| `Move` / `Rotate` / `Scale` | `options_*_mode_` (constraints, axis, material) |
| `Shape_chamfer` / `Shape_fillet` | mode + radius/distance |
| `Shape_polar_duplicate` | angle, count, rotate/combine, **Dup** button |
| `Sketch_inspection_mode` | `options_sketch_common_` |
| Each sketch tool mode | Matching `options_sketch_*_mode_` |
| `Sketch_operation_axis` | Mirror / Revolve / Clear axis |
| `Sketch_face_extrude` | Both sides, material |

Shared sketch controls (snap, midpoint nodes, place-from-center) live in `options_sketch_common_` and helpers in `gui_mode.cpp`.

## ImGui frame order (`render_gui`)

| Order | Function | Purpose |
| --- | --- | --- |
| 1 | `flush_view_events` | Sync camera before UI uses projection |
| 2 | `menu_bar_`, `toolbar_` | File / View / mode tools |
| 3 | `dist_edit_`, `angle_edit_` | Floating numeric entry |
| 4 | `sketch_list_`, `sketch_properties_dialog_` | Sketch List + underlay/properties |
| 5 | `shape_list_`, `shape_info_dialog_` | Shape List + info popup |
| 6 | `options_` | Mode-specific Options pane |
| 7 | `message_status_window_`, `about_dialog_` | Status + About |
| 8 | `add_*_dialog_` | Primitive / sketch creation popups |
| 9 | `log_window_`, consoles, `settings_`, `dbg_` | Log, Lua/Python, Settings |

3D redraw: `render_occt()` -> `Occt_view::do_frame()` (separate from ImGui pass).

## Settings and persistence

| File | Role |
| --- | --- |
| `gui_settings.cpp` | Settings dialog UI; read/write `ezycad_settings.json` |
| `save_occt_view_settings` | Persists `gui.*`, `occt_view.*`, pane visibility, last project path |
| `load_occt_view_settings_` | Called from `GUI::init` |
| `occt_view_settings_json()` | Scripting API for settings blob |

User-visible key tables: [`docs/usage-settings.md`](../../docs/usage-settings.md). When adding a Settings control, follow [agents/conventions/user-docs-sync.md](../../agents/conventions/user-docs-sync.md).

## Toolbar and one-shot commands

Toolbar buttons hold `std::variant<Mode, Command>`. `Command` (`Shape_cut`, `Shape_fuse`, `Shape_common`) runs immediately on click via `shp_cut` / `shp_fuse` / `shp_common` (no persistent mode).

Mode buttons call `set_mode`. Active state tracks `m_mode`.

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

| Item | Notes |
| --- | --- |
| Sketch tests | `GUI_access` friend in `tests/sketch_tests.cpp` |
| Headless / partial GUI | `sketch_left_click`, message getters on `GUI_access` |
| Full UI | Manual smoke; no dedicated `gui_tests` target |

## Related code outside `src/gui*`

| Location | Role |
| --- | --- |
| [`mode.h`](../mode.h) | `Mode`, `Fillet_mode`, `Chamfer_mode`, sketch/shape mode helpers |
| [`main.cpp`](../main.cpp) | GLFW callbacks -> `GUI`; dist/angle edit key guard |
| [`src/doc/sketch.md`](sketch.md) | Sketch methods called from `GUI` / `Occt_view` |
| [`src/doc/shape.md`](shape.md) | Shape operations invoked from toolbar, Options, mouse |
| [`scr_lua_console.cpp`](../scr_lua_console.cpp) / [`scr_python_console.cpp`](../scr_python_console.cpp) | Script consoles embedded in `render_gui` |
| [`utl_settings.cpp`](../utl_settings.cpp) | User settings file path and I/O helpers |
