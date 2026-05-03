# EzyCad settings

This guide covers the **Settings** pane (what is on screen), the **View** menu (pane visibility is separate from Settings), where preferences are saved, the **startup project**, and the JSON settings schema (some keys are not edited in the **Settings** pane). For the rest of the application, see the [main usage guide](usage.md).

## Table of contents

1. [View menu](#view-menu)
2. [Settings pane](#settings-pane)
3. [Options panel (sketch)](#options-panel-sketch)
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
- **Usage Guide** - Opens the [main usage guide](usage.md) in the browser.

## Settings pane

Open **View -> Settings**. The window title is **Settings**.

- **Dark mode** — checkbox at the top (not inside a collapsible section).
- At the bottom: **Defaults** — reloads bundled defaults from the app `res/` tree (including ImGui layout from that file).

Between those, the pane has **six** collapsible sections. Expand a section to see its controls; when collapsed, only the section title bar is visible.

1. **3D view navigation** — **View rotation step** (degrees per key press for **NumPad 8**/**2**/**4**/**6** orbit and **Shift+NumPad 4**/**6** roll; default **45**). Stored as **`gui.view_roll_step_deg`**. See [usage.md -> View navigation](usage.md#view-navigation) and **[usage-occt-view.md](usage-occt-view.md)**.

2. **UI corner rounding** — Sliders **0** to **16** for **Windows, frames, popups**; **Scrollbars and sliders** (has `(?)`); **Tabs**.

3. **3D view background** — **Background color 1** and **Background color 2** (float RGB fields and swatches). **Gradient blend** — combo: **Horizontal**, **Vertical**, **Diagonal 1**, **Diagonal 2**, **Corner 1** … **Corner 4**.

4. **3D view grid** — **Fine grid lines** and **Major grid lines** (passed to Open CASCADE `Aspect_Grid::SetColors`: dense lines vs every-tenth emphasis lines). **Grid plane fill** — tint for a large XY plane slightly below z=0 so the ground area can differ from the sky gradient (see `(?)` in app). This third color is not part of OCCT grid line rendering.

5. **Sketch** — **Dimension line width** — slider **0.5** to **8.0** (has `(?)`). **Underlay highlight color** — RGB (has `(?)`).

6. **Startup project** — **Desktop only:** **Load last opened on startup** (checkbox, with `(?)`), then **Last opened path:** … or **(No path saved yet.)** Then **Save current as startup project**, **Clear saved startup** (with `(?)`). **WebAssembly:** no load-last row; only the two buttons and `(?)`. See [Startup project](#startup-project).

**Not in this pane**

- **View** menu items such as **Options**, **Sketch List**, **Lua Console** — they only show or hide panes; they are not rows inside **Settings**. Their visibility is still saved under `gui.*` in the settings file (see [Settings file reference](#settings-file-reference)).
- **Length value placement** for edge dimensions — **Options** panel when the edge-dimension tool is active; see [Options panel (sketch)](#options-panel-sketch).

**Saving** — On desktop, settings are written when you change options that save, and on exit. On **Emscripten**, use **File -> Save settings** so the browser persists (see [Where settings are stored](#where-settings-are-stored)).

## Options panel (sketch)

Some sketch-related preferences are edited in the **Options** panel while you use a tool, not in the **Settings** pane:

- **Length value placement** (shown when the mode is **Toggle edge dimension** / length dimensions) - Combo: *Near first point*, *Near second point*, *Center on dimension line*, *Automatic*. This maps to the `edge_dim_label_h` key (integers **0** through **3**). Changing it persists like other GUI flags.

**Dimension line width** for those dimensions is in **Settings -> Sketch** (see above).

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

String: ImGui `.ini` text for window positions and docking saved with **SaveIniSettingsToMemory**. Loaded with **LoadIniSettingsFromMemory** on startup.

### `occt_view`

| Key | Type | Meaning |
| --- | --- | --- |
| `bg_color1` | array of 3 numbers | Background gradient color 1 (float RGB, 0 to 1). |
| `bg_color2` | array of 3 numbers | Background gradient color 2. |
| `bg_gradient_method` | integer | Gradient mode: 0 horizontal, 1 vertical, 2 to 3 diagonals, 4 to 7 corners (same order as the Settings pane combo). |
| `grid_color1` | array of 3 numbers | Fine (dense) grid lines (`Aspect_Grid` main color). |
| `grid_color2` | array of 3 numbers | Major (sparse / every-tenth) grid lines (`Aspect_Grid` tenth-line color). |
| `grid_fill_color` | array of 3 numbers | RGB tint for the infinite XY fill plane below the scene (separate from line colors). |

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
| `edge_dim_label_h` | integer | Length dimension label placement: **0** to **3** (see [Options panel (sketch)](#options-panel-sketch)). Values outside this range are ignored. |
| `edge_dim_line_width` | number | Sketch length dimension line width (allowed range **0.5** to **8.0** in code). |
| `imgui_rounding_general` | number | Window/child/frame/popup rounding (**0** to **32** clamped in code; sliders stop at 16 in the UI). |
| `imgui_rounding_scroll` | number | Scrollbar and grab rounding (same clamp). |
| `imgui_rounding_tabs` | number | Tab rounding (same clamp). |
| `underlay_highlight_color` | array of 3 numbers | Default underlay tint (float RGB **0** to **1** per channel). |
| `view_roll_step_deg` | number | Degrees per **NumPad 8**/**2**/**4**/**6** orbit and **Shift+NumPad 4**/**6** roll (allowed range **0.1** to **180** in code; default **45**). |
| `load_last_opened_on_startup` | boolean | Desktop: open the last `.ezy` on launch. **Legacy:** `load_last_saved_on_startup` is read as a fallback if the newer key is absent. |
| `last_opened_project_path` | string | Path of the last opened project for the option above. **Legacy:** `last_saved_project_path` is accepted if the newer key is missing. |

Scripting API **`ezy.occt_view_settings_json()`** returns a JSON string with **`occt_view`** plus selected **`gui`** keys (including **`gui.edge_dim_label_h`**, **`gui.edge_dim_line_width`**, **`gui.view_roll_step_deg`** when saved). See [scripting.md](scripting.md).

---

For general workflows and tools, see [usage.md](usage.md). For the 3D viewer (Open CASCADE), see **[usage-occt-view.md](usage-occt-view.md)**. For 2D sketching, see [usage-sketch.md](usage-sketch.md).
