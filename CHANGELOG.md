# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Consistent **Options pane header** (the current tool/mode name from the toolbar) followed immediately by a small **"?"** contextual documentation button for every mode that renders an Options pane. This includes pure Normal (Inspection) and Sketch inspection modes (previously only active tool modes had the header + button).
- Centralized `get_doc_url_for_mode(Mode)` map (with `EZY_ASSERT_MSG` ensuring its size matches `Mode::_count`) that drives the "?" help buttons to open the exact user-guide section for the active tool (e.g. `#operation-axis-tool`, `#slot-creation-tool`, `#add-node-tool`, `#rectangle-tool-center-point`, etc.).
- Unit test (`tests/doc_link_tests.cpp`) that validates the per-mode documentation URLs (including fragment anchors where present) are reachable on the live docs site.
- Generic issue and PR drafting templates (`agents/issues/issue.md`, `agents/prs/PR.md`) plus updates to `agents/README.md` to list them.

### Changed

- **Documentation** (`usage-sketch.md`): every sketch creation tool now fully documents both Tab (distance / length / radius / dimension) and Shift+Tab (angle / orientation) during placement rubber-bands:
  - Circle (Center-Radius)
  - Square Tool
  - Rectangle Tool (Two Points)
  - Rectangle Tool (Center Point)
  - Slot Creation Tool (with explicit note that the first edge defines slot length/orientation while the radius point defines the arc radius / "second edge dim")
  - Operation Axis Tool
  Each tool received updates to its Features table, "How to use" steps (with ordering notes), Shortcuts table, a dedicated **Angle Constraint:** subsection, Tips, and (where relevant) workflow or point-order notes. The general "Hotkeys" summary and common sketch hotkeys table were kept in sync.
- **Documentation** (`usage.md`, `usage-settings.md`, `ezycad_doc_style.md`): the Options Panel description now explicitly calls out the new tool/mode-name header + "?" documentation button that appears after the header in every Options pane. Cross-references and "Links from the application" guidance were expanded to cover the new in-app help buttons.
- **Agent / AI documentation** (`agents/ezycad-ascii-source.md`, `agents/README.md`, `agents/dev.md`, `agents.md`): the ASCII-only source rules now explicitly state they apply to `tests/` (EzyCad_tests sources) in addition to `src/`.
- Various minor cross-references, consistency fixes, and agent template improvements tied to the options-pane and documentation work.

## [0.1.0] - 2026-04-17

### Added

- **Orthographic projection** checkbox is now visible in the Options panel for all non-sketch modes (not just pure Normal/Inspection). The setting controls the camera projection for any non-sketch mode (persisted as `gui.inspection_orthographic`). Sketch modes still force orthographic.
- **Options** pane: horizontal scrollbar when the window is resized narrower than its controls.
- **Shape List**: right-click a shape **name** to **Delete**; right-click the **M** button for **Delete** as well (click **M** for the material popup).
- **Settings** (3D view grid) and saved **`occt_view`** JSON: configure Open CASCADE rectangular grid **step** (uniform X/Y), plus **graphic display extent X/Y** and **Z offset** (`V3d_RectangularGrid::SetGraphicValues`). Bundled **`res/ezycad_settings.json`** includes defaults for those keys.

- **Keyboard zoom:** **NumPad +** / **NumPad -**, main **-**, and **Shift+=** (US **+**) zoom the 3D view in steps equal to **one mouse wheel tick** at the cursor (same internal scaling as scroll).
- **NumPad 8 / 2 / 4 / 6:** orbit the 3D view in fixed steps (same axes as LMB orbit); step is **View rotation step** in Settings (shared with Shift+NumPad roll; default 45 degrees; `gui.view_roll_step_deg`).
- **NumPad 5:** snap the 3D view to the nearest orthographic world-axis orientation (top/bottom/front/back/left/right) with roll reset; keeps eye-target distance.
- **View roll:** **Shift+NumPad 4** and **Shift+NumPad 6** roll the 3D view (Blender-style). **Settings** has **View rotation step** (degrees per key press for orbit and roll; default 45, stored as `gui.view_roll_step_deg`).
- Help > About: reads bundled `res/about.md` via [imgui_markdown](https://github.com/enkisoftware/imgui_markdown), shows version text, splash image (`res/AI-gen-splashscreen_05_01_2026_512.png`), and clickable links; assets are copied next to the executable (and preloaded for Emscripten).
- **Sketch length dimensions (Dimension tool):** dimensions are stored as an unordered pair of sketch nodes (not attached to a single edge). They can be selected in sketch mode and removed with **Delete**.
- **Project JSON:** root-level `ezyFormat` (current value `2`) on save. Sketches serialize `length_dimensions` as dense node-index pairs alongside linear edges as `[a, b, mid]` only.
- **Legacy load:** older sketch JSON that stored a per-edge dimension flag on indexed edges (`[a, b, mid, dim]`) or on legacy coordinate edges (`[pt_a, pt_b, dim]`) migrates those flags into `length_dimensions` when loading.

### Fixed

- Ship `res/default.ezy` (empty sketch template) so native copy steps and Emscripten `--preload-file` resolve the path; startup loading could already fall back if the file was absent.

### Changed

- **Sketch snap:** unified axis-guide feedback in sketch mode; direct snap to nodes on **other visible sketches** (projected onto the current plane) restored; dimension-tool node hover no longer mutates stored node coordinates.
- **Documentation:** [usage-sketch.md](usage-sketch.md#sketch-snapping) adds a **Sketch snapping** section (axis alignment, vertex lock, cross-sketch targets, add-node edge interior). [usage.md](usage.md#sketch-list) clarifies Sketch List **Nodes** vs topology vertices. [usage-settings.md](usage-settings.md#sketch-tools) links to the snap section.
- **View roll** (**Shift**+**NumPad 4**/**6**, main **4**/**6**, or **Left**/**Right** arrow): same roll as **Shift**+**4**/**6**; helps when Num Lock makes the numpad send arrows. Handled on key **repeat** as well as press; **Shift**+main **4**/**6** no longer fall through to the selection filter.
- **Keyboard zoom** (**NumPad +/-**, **Shift+=**, **-**): each repeated OS key event zooms again so holding the key zooms continuously (uses GLFW key repeat).
- **Zoom:** **Zoom scroll scale** in Settings replaces the hard-coded wheel multiplier (**4**); stored as **`gui.view_zoom_scroll_scale`**. Hold **Shift** while scrolling or using +/- for Blender-style finer zoom (**x0.1** on the delta).
- **View roll** default step is **45** degrees (was 15).
- Window title shows the current project file name (e.g. after Open or Save), or `untitled` when there is no path yet; **File > New** clears the name so the title matches an empty document.
- **Dimension tool behavior:** click a straight edge to toggle a length dimension between its endpoints, or click two nodes to toggle a dimension between them; clicks away from nodes and edges do not create spurious dimensions. Node picks take precedence over edge picks when both could apply; moving the mouse updates node snap feedback in this mode.
- **Documentation:** `usage-sketch.md` and `usage.md` describe the tool as the **Dimension tool** and document the node-pair model. **Num Lock off** is documented as recommended for numpad view shortcuts (orbit, roll, zoom, snap); **Num Lock on** may remap the keypad on Windows and other OSes (`usage.md`, `usage-occt-view.md`, `usage-settings.md`).

### Added

- Initial version tracking: `src/version.h` and this changelog.

[Unreleased]: https://github.com/trailcode/EzyCad/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/trailcode/EzyCad/releases/tag/v0.1.0
