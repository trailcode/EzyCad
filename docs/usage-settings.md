# EzyCad settings

This guide covers the **Settings** pane (what is on screen), the **View** menu (pane visibility is separate from Settings), where preferences are saved, the **startup project**, and the JSON settings schema (some keys are not edited in the **Settings** pane). For the rest of the application, see the [main usage guide](usage.md).

## Table of contents

1. [View menu](#view-menu)
2. [Settings pane](#settings-pane)
3. [Options panel](#options-panel)
4. [Where settings are stored](#where-settings-are-stored)
5. [Startup project](#startup-project)
6. [Settings file reference](#settings-file-reference)

## View menu

**View** toggles panes and opens **Settings**:

- **Settings** - Opens the [Settings pane](#settings-pane).
- **Options** - Show or hide the Options panel.
- **Sketch List** - Show or hide the Sketch List pane.
- **Shape List** - Show or hide the Shape List pane.
- **Log** - Show or hide the Log window.
- **Lua Console** - Show or hide the interactive Lua prompt and `res/scripts/lua` editors. See [Scripting](scripting.md).
- **Python Console** - Same for Python when the app is built with embedded Python (native only; not in the WebAssembly build).
- **Debug** - Show or hide the debug pane (debug builds only).

Visibility of these panes (and related flags) is persisted with the rest of your settings.

**Help menu**

- **About** - Opens the [project README](README.md) in the browser.
- **Usage Guide** - Opens the [online usage guide](https://ezycad.readthedocs.io/en/latest/usage.html) (Read the Docs).

## Settings pane

Open **View -> Settings**. The window title is **Settings**.

- **Dark mode** — checkbox at the top (not inside a collapsible section).
- **UI verbosity** — stepper at the top (default **6**). Values **5** and **6** show contextual **?** help buttons (hover for tooltip, click to open Read the Docs where linked). Lower values hide those buttons and some explanatory text.
- At the bottom: **Defaults** — reloads bundled defaults from the app `res/` tree (including ImGui layout from that file).

Between those, the pane has **six** collapsible sections. Expand a section to see its controls; when collapsed, only the section title bar is visible.

1. **3D view navigation** — **View rotation step** (degrees per key press for <kbd>NumPad 8</kbd>/<kbd>2</kbd>/<kbd>4</kbd>/<kbd>6</kbd> orbit and <kbd>Shift</kbd>+<kbd>NumPad 4</kbd>/<kbd>6</kbd> roll; default **45**; **`?`** opens [view roll](https://ezycad.readthedocs.io/en/latest/usage.html#view-roll) on Read the Docs). **Zoom scroll scale** (multiplier for wheel and **+**/**-** zoom; default **4**). Hold <kbd>Shift</kbd> while zooming for a Blender-style finer step (multiply by **0.1**). Numpad shortcuts are documented with <kbd>Num Lock</kbd> off; with <kbd>Num Lock</kbd> on, use main-row alternatives in [usage.md -> View navigation](usage.md#view-navigation). Stored as **`gui.view_roll_step_deg`** and **`gui.view_zoom_scroll_scale`**. See **[usage-occt-view.md](usage-occt-view.md)**.

2. **UI corner rounding** — Sliders **0** to **16** for **Windows, frames, popups**; **Scrollbars and sliders** (has **?**); **Tabs**.

3. **View presentation** — **Background color 1** and **Background color 2** (float RGB fields and swatches). **Gradient blend** — combo: **Horizontal**, **Vertical**, **Diagonal 1**, **Diagonal 2**, **Corner 1** … **Corner 4**. **Element hover color** — highlight for rows hovered in the **Shape List** or **Sketch List** (**Dimensions** table); stored as **`gui.elm_list_hover_color`**.

4. **3D view grid** — **Show grid** (checkbox; grid is drawn on the **active sketch** plane, behind coplanar geometry). **Fine grid lines** and **Major grid lines** (dense lines vs every-tenth emphasis). **Grid step**, **Grid padding** (margin around sketch content when sizing the grid), and **Grid display Z offset** in the Settings pane use the **same length scale as sketch length dimensions** (display value = model value / internal `dimension_scale`, default **100**). Saved JSON (`occt_view`) stores padding in model units (`grid_padding`).

5. **Sketch** — Expand **Dimensions** (nested, open by default) for length-dimension appearance and behavior (most rows have **?** help). Other sketch rows stay in the parent **Sketch** section:
   - **Dimension line width** — slider **0.5** to **8.0**
   - **Dimension arrow size** — slider **1.0** to **24.0**
   - **Dimension color** — RGB for dimension lines, arrow heads, and value text
   - **Dimension text scale** — slider **0.5** to **3.0** (multiplier on label height)
   - **Label rendering** — *Opaque 2D text*, *SetCommonColor*, *2D screen text*, *3D text*, *Z-layer Top*, *Z-layer Topmost* (default)
   - **Length value placement** — combo: *Near first point*, *Near second point*, *Center on dimension line*, *Automatic* (persisted as `edge_dim_label_h` **0**–**3**; updates existing dimensions live)
   - **Arrow style** — *Standard*, *Sharp*, *Wide*, or *3D shaded*
   - **Arrow orientation** — *Automatic*, *Internal*, or *External*
   - **Show sketch dimensions** — global on/off for length dimensions on all sketches (tool mode may still limit which sketch shows dims when on)
   - **Permanent node annotation size** (scale for **+** markers on the sketch **Origin** and [Add node](usage-sketch.md#add-node-tool) points; see [Sketch origin](usage-sketch.md#sketch-origin)), **Add midpoints to new linear edges** (default off; only affects Line Edge and Multi-Line Edge tools), **Underlay highlight color**, **Snap guide color (node)**, **Snap guide color (axis)**, **Snap guide line width** (slider **0.5** to **8.0**; default **1.0**), **Snap guide mode**, **All co-axial nodes** (directly under **Sketch**, not inside **Dimensions**)

6. **Startup project** — **Desktop only:** **Load last opened on startup** (checkbox, with **?**), then **Last opened path:** … or **(No path saved yet.)** Then **Save current as startup project**, **Clear saved startup** (with **?**). **WebAssembly:** no load-last row; only the two buttons and **?**. See [Startup project](#startup-project).

**Not in this pane**

- **View** menu items such as **Options**, **Sketch List**, **Lua Console** — they only show or hide panes; they are not rows inside **Settings**. Their visibility is still saved under `gui.*` in the settings file (see [Settings file reference](#settings-file-reference)).
**Saving** — On desktop, settings are written when you change options that save, and on exit. On **Emscripten**, use **File -> Save settings** so the browser persists (see [Where settings are stored](#where-settings-are-stored)).

## Options panel

Open **View -> Options**. 

The top of the panel shows the name of the current tool/mode followed by a small **"?"** button. The "?" provides a direct link to the relevant section of the user guide for that tool (matching the contextual help added for Options panes).

Content depends on the active tool mode. Related controls are grouped under headings (for example **Sketch options**, **Extrude**, **Selection**, **Material**).

If you resize the pane narrower than its controls, a **horizontal scrollbar** appears so long labels (for example **Orthographic projection**) stay readable.

### Normal mode (Inspection)

Under **Selection**:

- **Selection Mode** — combo for the 3D pick filter (vertices, edges, faces, solids, and combinations). The **?** button links to [shape selection filter hotkeys](usage.md#shape-selection-filter-normal-mode-only) in the usage guide.
- **Orthographic projection** — checkbox (appears in the Options panel for all non-sketch modes) toggling an orthographic camera (sketch modes still force orthographic). Persisted as **`gui.inspection_orthographic`**.

Under **Material**:

- Document preset for new solids that do not inherit from a clicked shape (toolbar **Box**, **polar duplicate** output, and similar). To change material on an existing solid, use the [Shape List](usage.md#shape-list).

For other non-sketch Options content (for example **Polar duplicate**), see [usage.md -> User Interface](usage.md#user-interface) (Options Panel).

### Sketch tools

Sketch-related preferences are edited in the **Options** panel while you use a sketch tool, not in the **Settings** pane:

- **Sketch options** (all sketch tools): **Snap dist**, **Snap guide mode** (*Traditional*, *Fullscreen*, *Both*), and **All co-axial nodes** (global co-axial grid vs closest-per-axis only). See [How sketch snap works](usage-sketch.md#sketch-snapping). **Snap guide color (node)**, **Snap guide color (axis)**, **Snap guide line width**, **Snap guide mode**, and **All co-axial nodes** are also in **Settings -> Sketch** (persisted in `gui.*` keys below).
- **Extrude sketch face**: under **Extrude**, **Both sides** and **Material** for the new solid (same document preset as **Normal** mode Options **Material**). Other modes that still show **Material** in Options use that same preset when relevant (for example **Sketch from planar face**).
- **Add edge** / **Add node** (and similar): a **Shortcuts** line documents TAB / Shift+TAB typing behavior.
- **Add line edge**: under **Options**, **Add midpoint nodes** ([usage-sketch.md#line-edge-option-add-midpoint-nodes](usage-sketch.md#line-edge-option-add-midpoint-nodes)) and **Place from center** ([usage-sketch.md#line-edge-option-place-from-center](usage-sketch.md#line-edge-option-place-from-center)); each has a **?** link. See also [Line edge Options](usage-sketch.md#line-edge-options). The midpoint setting is also in **Settings -> Sketch** (persisted).
- **Sketch operation** (mirror / revolve axis): mirror, revolve, angle, and clear-axis actions (see [usage-sketch.md](usage-sketch.md#operation-axis-tool)).

Global length-dimension style (line width, arrows, color, text) is in **Settings -> Sketch**. Per-dimension visibility, name, and offset remain in **Sketch List -> Dimensions** (saved in the project `.ezy` file).

## Where settings are stored

EzyCad reads and writes a single JSON file named **`ezycad_settings.json`**.

- **Windows** — `%APPDATA%\EzyCad\`
- **Linux** — `~/.config/EzyCad/`
- **macOS** — `~/Library/Application Support/EzyCad/`

If the user-specific file is missing, the app may fall back to a legacy copy in the current working directory (next to the executable when launched that way).

**WebAssembly** — Settings are kept in the browser under the `localStorage` key `ezycad_settings`. Use **File -> Save settings** to persist after changes.

Bundled defaults for **Defaults** and first-run live under the **`res/`** folder in the install or build output (same basename as above).

## Startup project

Similar to Blender's startup file: EzyCad can load a **default document** when it starts, including geometry, camera/view (stored in the `.ezy`), and **current tool mode**.

- **First launch / no custom startup** - The app loads the bundled template `res/default.ezy` from the install or build output.
- **Save your own startup** - Set up the scene and mode the way you want, open **Settings**, expand **Startup project**, and click **Save current as startup project**. On desktop, this writes `startup.ezy` under your user config folder (same base paths as above). On the web build, it is stored in the browser (localStorage).
- **Next runs** - If a saved startup exists, it is loaded instead of the bundled file. The session starts **untitled** (so **Save** does not overwrite your startup file until you pick a path).
- **Clear saved startup** - In **Settings -> Startup project**, click **Clear saved startup**; the next launch uses the bundled `res/default.ezy` again.

## Settings file reference

The on-disk (or localStorage) document is JSON with a **version**, optional **ImGui** layout blob, an **`occt_view`** object for the 3D view, and a **`gui`** object. **`gui`** includes pane visibility and other flags that are **not** all exposed as controls in the **Settings** pane (for example **show sketch list** comes from **View**, not from a row inside **Settings**).

### `version`

String **`"1"`**. If the version is missing or does not match, the app replaces the file with bundled defaults (see `gui_settings.cpp`).

### `imgui_ini`

String: ImGui `.ini` text for window layout and docking saved with **SaveIniSettingsToMemory**. Loaded with **LoadIniSettingsFromMemory** on startup. Includes dock-node data when panels have been docked or tabbed.

Panels (Sketch List, Shape List, Options, Log, consoles, and similar) can be dragged to dock edges, tabbed together, or split. On desktop, panels can also be dragged outside the main window into separate OS windows. The web build supports the same in-canvas docking inside the browser canvas only. The 3D view stays in the central passthrough region of the main window; it is not moved into dock nodes.

If saved layout text has no `[Docking]` section (older installs), a default dock layout is applied once on next launch.

### `occt_view`

| Key | Type | Meaning |
| --- | --- | --- |
| `bg_color1` | array of 3 numbers | Background gradient color 1 (float RGB, 0 to 1). |
| `bg_color2` | array of 3 numbers | Background gradient color 2. |
| `bg_gradient_method` | integer | Gradient mode: 0 horizontal, 1 vertical, 2 to 3 diagonals, 4 to 7 corners (same order as the Settings pane combo). |
| `grid_color1` | array of 3 numbers | Fine (dense) grid lines (`Aspect_Grid` main color). |
| `grid_color2` | array of 3 numbers | Major (sparse / every-tenth) grid lines (`Aspect_Grid` tenth-line color). |
| `grid_visible` | boolean | When **true**, the OCCT reference grid is drawn in the 3D view (default **true**). |
| `grid_step` | number | Grid line spacing in **model** units (default **10**). Legacy `grid_x_step` / `grid_y_step` load as this value (X preferred). |
| `grid_padding` | number | Margin around active sketch content when sizing the grid, in **model** units (default **1000**; Settings shows display units). Legacy `grid_graphic_x_size` loads as padding if `grid_padding` is absent. |
| `grid_graphic_z_offset` | number | Grid plane offset along Z in **model** units. |

### `gui`

| Key | Type | Meaning |
| --- | --- | --- |
| `dark_mode` | boolean | Light/dark theme. |
| `show_options` | boolean | Options panel visible. |
| `show_sketch_list` | boolean | Sketch List pane visible. |
| `show_shape_list` | boolean | Shape List pane visible. |
| `log_window_visible` | boolean | Log window visible. |
| `show_settings_dialog` | boolean | Whether the Settings pane was open when last saved (usually false). |
| `show_lua_console` | boolean | Lua console pane visible. |
| `show_python_console` | boolean | Python console pane visible (native builds with Python). |
| `show_dbg` | boolean | Debug pane visible (debug builds only). |
| `inspection_orthographic` | boolean | Non-sketch modes Options: orthographic camera when true (default false). Sketch modes always use orthographic. |
| `edge_dim_label_h` | integer | Length dimension label placement: **0** near first point, **1** near second, **2** center, **3** automatic. |
| `edge_dim_line_width` | number | Sketch length dimension line width (**0.5** to **8.0**). |
| `edge_dim_arrow_size` | number | Arrow head length (**1.0** to **24.0**). |
| `edge_dim_color` | array of 3 numbers | Dimension line, arrow, and text RGB (**0** to **1** per channel; default olive **0.54**, **0.54**, **0.21**). |
| `edge_dim_text_scale` | number | Label height multiplier (**0.5** to **3.0**; default **1.0**). |
| `edge_dim_text_render_mode` | integer | **0** opaque 2D, **1** SetCommonColor, **2** 2D screen, **3** 3D text, **4** Z Top, **5** Z Topmost (default). |
| `edge_dim_arrow_style` | integer | **0** standard, **1** sharp, **2** wide, **3** 3D shaded. |
| `edge_dim_arrow_orientation` | integer | **0** automatic, **1** internal, **2** external. |
| `show_sketch_dimensions` | boolean | When false, hides length dimensions on all sketches. |
| `permanent_node_anno_scale` | number | Scale for permanent **+** markers: the sketch **Origin** and user-placed Add node points ([Sketch origin](usage-sketch.md#sketch-origin); **0.25** to **3.0**; default **1.0**). |
| `origin_marker_color` | array of 3 numbers | RGB color for the **active** sketch's Origin marker (+ with circle; **0** to **1** per channel; default cyan **0.0**, **0.75**, **1.0**). |
| `snap_guide_color_node` | array of 3 numbers | RGB for snap guides when both axes lock to the same node (float **0** to **1**; default lavender **0.82**, **0.55**, **0.95**). Legacy `snap_guide_color` loads here when `snap_guide_color_node` is absent. |
| `snap_guide_color_axis` | array of 3 numbers | RGB for snap guides when aligned on X or Y only (float **0** to **1**; default magenta **0.96**, **0.06**, **0.54**). Legacy `snap_guide_color` sets both node and axis colors. |
| `snap_guide_mode` | integer | **0** *Traditional* (local markers), **1** *Fullscreen* (view-spanning axis lines), **2** *Both* (default **2**). |
| `snap_guide_line_width` | number | Open CASCADE line width for snap guides (axis lines, markers, co-axial overlay; **0.5** to **8.0**; default **1.0**). |
| `annotate_all_coaxial_nodes` | boolean | When true (default), show axis guides and markers for *all* co-axial nodes (current sketch plus other visible sketches). When false, only the closest node per active axis is annotated. Also in sketch **Options**. |
| `imgui_rounding_general` | number | Window/child/frame/popup rounding (**0** to **32** clamped in code; sliders stop at 16 in the UI). |
| `imgui_rounding_scroll` | number | Scrollbar and grab rounding (same clamp). |
| `imgui_rounding_tabs` | number | Tab rounding (same clamp). |
| `underlay_highlight_color` | array of 3 numbers | Default underlay tint (float RGB **0** to **1** per channel; default **0.64**, **0.56**, **0.31**). |
| `elm_list_hover_color` | array of 4 numbers | RGBA highlight for rows hovered in the **Shape List** or **Sketch List** dimensions table (float **0** to **1** per channel; default purple **0.40**, **0.10**, **0.47**, **1**). |
| `view_roll_step_deg` | number | Degrees per **NumPad 8**/**2**/**4**/**6** orbit and **Shift+NumPad 4**/**6** roll (allowed range **0.1** to **180** in code; default **45**). |
| `view_zoom_scroll_scale` | number | Multiplier for `UpdateZoom` scroll delta from wheel and keyboard zoom (allowed range **0.25** to **64** in code; default **4**). With **Shift** held, the effective step is multiplied by **0.1** (Blender-style finer zoom). |
| `load_last_opened_on_startup` | boolean | Desktop: open the last `.ezy` on launch. **Legacy:** `load_last_saved_on_startup` is read as a fallback if the newer key is absent. |
| `last_opened_project_path` | string | Path of the last opened project for the option above. **Legacy:** `last_saved_project_path` is accepted if the newer key is missing. |

Scripting API **`ezy.occt_view_settings_json()`** returns a JSON string with **`occt_view`** plus selected **`gui`** keys (including dimension and snap keys above, **`gui.permanent_node_anno_scale`**, **`gui.inspection_orthographic`**, **`gui.view_roll_step_deg`**, **`gui.view_zoom_scroll_scale`** when saved). See [scripting.md](scripting.md).

---

For general workflows and tools, see [usage.md](usage.md). For the 3D viewer (Open CASCADE), see **[usage-occt-view.md](usage-occt-view.md)**. For 2D sketching, see [usage-sketch.md](usage-sketch.md).
