# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Sketch appearance

- **Settings -> Sketch -> Appearance:** **Shapes in sketch mode** — *Off (hide)*, *Ghost* (default semi-transparent shaded), or *Wire*. Optional **Ghost opacity**. Persisted as `gui.sketch_shape_faint_style` / `gui.sketch_shape_faint_opacity`. Replaces always-hiding solids while sketch tools are active.
- **Options -> Sketch options:** **Faint shapes** checkbox (all sketch tools) enables or disables that behavior; persisted as `gui.sketch_shape_faint_enabled` (default on).
- **Settings -> Sketch -> Appearance:** **Shape Faint Strength** slider (**5%**–**85%**, default **14%**) for Ghost and Wire styles (`gui.sketch_shape_faint_opacity`).
- Faint shapes in sketch mode are not selectable (no yellow hover/selection highlight).

### Sketch List

- Expanded **Faces** rows: **`E`** button and right-click **Extrude** start [extrude](docs/usage.md#extrude-sketch-face-tool-e) for that face (then drag height in the viewer).
- Hovering a **Faces** row highlights that face in the viewer with **Face highlight fill** (even outside sketch modes, when faces are normally hidden). The highlight uses the OCCT **Topmost** Z-layer so extruded solids do not cover it in inspection/Normal mode.

### Documentation

- **Usage guide workflow:** Added a dedicated step for creating sketches from planar faces in *Workflow: From 2D Sketches to 3D Shapes* ([usage.md](docs/usage.md)).
- **Usage guide order:** Moved **Scripting** to the end of the TOC and page body (after Tool Icons) so core modeling comes first for new users.

### View / New Project

- **File -> New** resets the camera to a **top view** framed to **Settings -> 3D view navigation** **Default 2D view width** / **height** (defaults **3** x **3** display units; `gui.default_2d_view_width` / `gui.default_2d_view_height`). Projects with no saved camera use the same framing.

## [0.3.0] - 2026-07-12

### WebAssembly

- **Settings -> Sketch:** **Edge thickness**, **Dimension line width**, and **Snap guide line width** are hidden in the browser build (Open CASCADE line-width controls have no visible effect on wasm GLES).

### View presentation

- **Settings -> View presentation:** **Shape selection color** for selected 3D shapes in the viewer (AIS SelectionStyle). Persisted as `gui.shape_selection_color` (default purple).
- **Bundled defaults:** Shape selection color is purple; Lua and Python consoles start hidden (`show_lua_console` / `show_python_console` default **false**).

### Scripting

- **`ezy.view.get_selected()` / `get_selected_indices()`:** Return the current 3D viewer selection as document `Shp` objects, or as shape indices (Python **0-based**, Lua **1-based**). Available in Python, Lua, and the remote `ezycad` client.
- **Public API layout:** Python-first package surface under `ezy` / `ezy.view` / `ezy.view.curr_sketch` (same object as `ezy.sketch`). Lua mirrors the same namespaces. See [docs/scripting.md](docs/scripting.md).
- **`ezy.view.fuse` / `cut` / `common` / `delete`:** Boolean union, cut (first shape is body, rest are tools), intersection, or delete one or more shapes. Available in Python, Lua, and the remote `ezycad` client.
- **Remote Python (`--listen`):** Desktop builds with embedded Python can accept console snippets over TCP (`EzyCad --listen [host:]port`). Execution is marshaled to the main UI thread. Importable client: `pip install -e .` then `import ezycad; ezycad.connect()` (typed API for IPython/IDE completion) under `scripts/ezycad/`. See [docs/scripting.md](docs/scripting.md#remote-python---listen).

### Sketch appearance

- **Settings -> Sketch -> Appearance:** configurable edge color, selection color, highlight (hover) color, and thickness (with alpha); face fill, selection, and highlight colors (with alpha). Persisted under `gui.sketch_edge_*` and `gui.sketch_face_*`. Bundled defaults match the current preferred palette (orange edge selection; mauve face fill; magenta/violet face selection/highlight).
- **Settings pane:** Open/closed state of collapsing section headers is persisted in `gui.settings_headers`.

### UI layout

- **ImGui docking:** Panels (Sketch List, Shape List, Options, Log, consoles, and similar) can be docked, tabbed, and split inside the main window. On desktop, panels can also be dragged into separate OS windows. The web build supports in-canvas docking only. Saved layout in settings includes dock nodes; older installs without a `[Docking]` section get a default layout on first launch.
- **Settings -> UI:** **Dark mode** and separate **Dark theme** / **Light theme** ImGui controls (window transparency, rounding, borders, padding, spacing). Legacy `imgui_rounding_*` keys migrate to both themes on load.

### Project format (`.ezy`)

- **Format v3:** Saved projects are a ZIP archive (`manifest.json` plus deduplicated `assets/*.rgba` underlay blobs). Sketch underlays reference assets by content hash instead of inline base64. Legacy plain-JSON `.ezy` files (v1/v2, including `rgba_b64` underlays) still load. Document snapshots for undo/redo reference shared assets rather than re-embedding pixel data.

### Shape List

- **Shape info...** on the right-click menu for a shape name or **M** button. Opens a dialog with OCCT topology and property details: root type (Solid, Shell, Face, etc.), validity, sub-shape counts, bounding box, volume, center of mass, surface area, and length where applicable. Includes document fields (name, material, display mode, visibility) and a **Refresh** button. Documented in the user guide under [Shape info](docs/usage.md#shape-info).

### Operation axis / Revolve

- Toolbar tooltip renamed to **Operational axis** (was "Define operation axis").
- The operational axis line is shown only while **Operational axis** mode is active; switching to another sketch mode hides it even if the axis remains defined for Mirror/Revolve.
- After the axis is defined (mirror/revolve phase), **permanent + node markers** and **sketch snap** (axis guides, vertex lock, snap annotations) are temporarily suppressed so edge/face selection is unobstructed. Snapping and markers return when you **Clear axis** or leave Operational axis mode. Axis placement (Phase 1) still uses normal snap.
- Operational axes are **saved and loaded** with the sketch in project JSON (`operation_axis` array of two plane points).
- After **Revolve**, when the selection is **edges** (not a face), EzyCad does its best to convert the revolved result from a shell or faces into a **solid** so chamfer, fillet, and booleans work on closed 360° profiles. Open or partial revolutions are unchanged.
- **?** help button next to the revolve angle field in the Options panel (tooltip + link to [Revolve solid conversion](docs/usage-sketch.md#revolve-solid-conversion) in the user guide).

### Sketch underlay

- When an image underlay has shear from edge calibration ("Set X from edge..." or "Set Y from edge..."), the **Transform** section in Sketch properties now provides a full 6-DOF editor: Base X/Y (bitmap 0,0 origin) + direct Ux/Uy and Vx/Vy vector components. Derived U/V lengths and the angle between them are displayed live. Edits apply immediately with undo support.
- A **Make orthogonal (keep lengths + U direction)** button projects the V vector to be perpendicular while preserving magnitudes and orientation sign; after use the familiar Center / Half width/height / Rotation sliders return.
- The original orthogonal sliders remain for the common (non-sheared) case. The cyan underlay frame continues to show the true (possibly sheared) parallelogram extent.
- Addresses the long-standing limitation noted in docs/bugs.md and usage-sketch.md. The raw basis editor lets users fine-tune or correct after calibration without losing the affine fit.

### Sketch / 2D Topology

- **Sketch origin:** Every sketch has a permanent **Origin** **+** marker at plane `(0, 0)` (reference-plane sketches) or at the bounding-box center of the face wire when created from a planar face. The origin is a snap target, is saved in project JSON (`origin` on the node), and cannot be deleted. User guide: dedicated [Sketch origin](docs/usage-sketch.md#sketch-origin) section (also linked from [Sketch List](docs/usage.md#sketch-list) and settings docs).
- **Arc segments:** Each arc is a single sketch edge (not two chained edges). New arcs split existing straight and arc edges at interior crossings; arcs are subdivided when crossed. Face walking uses arc tangents at endpoints. The Arc Segment tool shows a line rubber-band after the first click and an arc preview after the second; optional **Add midpoint nodes** in the Arc tool Options adds a geometric-mid snap node (same setting as line edges). Documented in [usage-sketch.md](docs/usage-sketch.md#arc-segment-creation-tool). **Known open bug:** slot + circle intersection splitting — see [bugs.md](docs/bugs.md).
- For the **Add line edge** and **Add multi-line edge** tools, a new setting **"Add midpoints to new linear edges"** (Settings > Sketch; also exposed in the tool Options pane) controls whether automatic midpoint nodes are created on new straight edges. Default is **off** (no midpoints), so new edges do not get mid snap targets unless the option is enabled. Existing edges with mids are unaffected; JSON load/save supports edges without mids for compatibility.
- **Add line edge** Options: **Place from center** — first click sets the edge midpoint; second click or Tab length uses the **full** edge length (symmetric placement). Documented in the user guide with **?** help links in the Options panel.

- Image underlays now render with a thin cyan bounding parallelogram (frame) around the full image region in the sketch plane. This makes newly added or low-contrast underlays (heavy white-key transparency, faint scans, dark view backgrounds) easy to locate and see their exact extent. The frame follows the current affine placement (including non-orthogonal after edge calibration) and is drawn for every visible underlay.
- Documented the automatic splitting of linear edges when they intersect other edges (the core user-visible topology behavior). When a new straight edge crosses or touches the interior of an existing straight edge, or when its endpoint snaps to an existing midpoint, the intersected edge(s) are split at the intersection and the new edge is subdivided as needed. This enables T-junctions, clean crossings (4 edges), and divided faces (e.g. square + exact mid vertical produces two rectangles) without any manual steps. See the new table row "Automatic splitting on edge intersections" and the Tips under [Sketch snapping](docs/usage-sketch.md#sketch-snapping) in the user guide.
- Added/expanded unit tests for the above (mid vertical splitter in both orders + full GUI draw-direction cases for the splitter; bridge + hole reproduction from user `bridge.ezy`).
- An internal rewrite attempt to reduce order/draw-direction sensitivity (angular sorting of adjacency, always-normalize instead of the orientation filter, etc.) was reverted after it was found to hang in some sketches. The new user documentation above reflects the stable current behavior (no internal dev details are exposed to users).

(The previous long dev-oriented bullets under this heading were condensed for user focus.)

- Added/expanded unit test coverage for mid-line splitters (square/rectangle + exact midpoint vertical in both addition orders via direct `add_edge_`, and full GUI `Sketch_add_edge` + `add_sketch_pt`/`finalize_elm` path in **both draw directions** — lower-mid-first vs top-first). Ensures two proper 5×10 rectangular faces (never a square + rect, and bbox checks for left/right halves). New tests: `AddSquareThenMidEdge_ProducesTwoRectangles_BothOrders` and `AddSquareThenMidEdge_GUIPath_BothDrawDirections_ProducesTwoRectangles`.
- Added regression test `UpdateFaces_BridgeEzy_ProducesTwoFaces_OneWithHole` reproducing the user-provided `bridge.ezy` (bridge edge + triangular inner loop) and asserting the desired structure (two faces, one with hole). Includes debug dump of current walker output (currently produces 3 faces; documents the gap).
- Experimented with a substantial rewrite of `Sketch::update_faces_()` (the core planar-graph face extractor):
  - Wrapped the prior body in `#if 0` for preservation.
  - Angular sorting of every node's adjacency "fan" by polar angle (atan2) immediately after the final adj rebuild — makes the geometric embedding explicit instead of depending on `m_edges` list order (splits, `finalize_edges_` appends, GUI line-tool `pa`/`pb` → Edge `a`/`b`, etc.).
  - Always normalize collected cycles to our CW convention (reverse list + toggle `reversed` flags on `Face_edge`) instead of the early `if (!is_face_clockwise_(face)) break;`. No cycles are discarded.
  - Two-neighbor "immediate successor in the sorted fan" left-most choice (with bias toward sharper |dot| turns over 180° straights at colinear mids) — more robust than full min-angle scan over order-dependent vectors.
  - Final geometric sort of `m_faces` (by leftmost vertex x, then y, using `verts_3d` populated at creation time) so that presentation/inspector/"first face" order is stable and independent of addition order or the direction the last internal edge was drawn.
  - The rewrite made the code significantly cleaner and resolved the "draw mid vertical bottom-to-top vs top-to-bottom swaps which side is the rectangle (or produces square vs rect)" symptom for the common square+mid case, plus improved some bridge rect attachment. However, it was found to **hang** (likely walker loop or bad state with certain bridge/colinear-mid configurations in real sketches) and was reverted by the author. The orientation filter + existing dangling/bridge exclusion logic remains the stable implementation for now.
- Added detailed documentation in `usage-sketch.md` (new section "Sketch faces, internal edges (splitters), holes, and bridges") covering current behavior, the role of the orientation filter, known sensitivities, tips for reliable split faces, and references to the new tests + ongoing work. Updated TOC.
- Updated `CHANGELOG.md` (this entry) and cross-references. See GitHub issue and PR for the full rewrite attempt, the hanging report, and the preserved `#if 0` original body for future iteration.

### Documentation

- Added substantial user-guide coverage for **Boolean Operations** (Cut, Fuse, Common) under Feature Operations in `usage.md`:
  - Clear explanation of the three tools, toolbar invocation, and multi-select workflow in Normal mode.
  - Features table (order sensitivity for Cut, input consumption, undo, material handling, compatibility with extrudes/imports/prior results).
  - Step-by-step "How to Use", practical Tips (including common OCCT boolean failure causes), and a Common Use Cases table.
  - Updated cross-references from the workflow overview, Extrude section, importing notes, and `usage-sketch.md` (circle and underlay sections) to point to the new detailed `#boolean-operations` anchor.
  - Chamfer and Fillet remain lightly documented for now (hotkeys + icons only).

## [0.2.0] - 2026-06-13

### Added

- Consistent **Options pane header** (the current tool/mode name from the toolbar) followed immediately by a small **"?"** contextual documentation button for every mode that renders an Options pane. This includes pure Normal (Inspection) and Sketch inspection modes (previously only active tool modes had the header + button).
- Centralized `get_doc_url_for_mode(Mode)` map (with `EZY_ASSERT_MSG` ensuring its size matches `Mode::_count`) that drives the "?" help buttons to open the exact user-guide section for the active tool (e.g. `#operation-axis-tool`, `#slot-creation-tool`, `#add-node-tool`, `#rectangle-tool-center-point`, etc.).
- Unit test (`tests/doc_link_tests.cpp`) that validates the per-mode documentation URLs (including fragment anchors where present) are reachable on the live docs site.
- Generic issue and PR drafting templates (`agents/issues/issue.md`, `agents/prs/PR.md`) plus updates to `agents/README.md` to list them.
- Version is now displayed in the application window title (`EzyCad 0.2.0 - filename`) and at the top of the Help > About dialog.

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
- Window title and About now surface the product version from `src/version.h`.

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

[Unreleased]: https://github.com/trailcode/EzyCad/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/trailcode/EzyCad/releases/tag/v0.3.0
[0.2.0]: https://github.com/trailcode/EzyCad/releases/tag/v0.2.0
[0.1.0]: https://github.com/trailcode/EzyCad/releases/tag/v0.1.0
