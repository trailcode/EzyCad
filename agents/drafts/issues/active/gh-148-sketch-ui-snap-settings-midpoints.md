# Sketch UI: snap colors, list hover, per-tool midpoints, settings polish

**Suggested labels:** `enhancement`, `sketch`, `ui`, `docs`

**Branch:** `Trailcode/ui_improvements` (base: `main`)

---

## Title (GitHub)

Sketch UI: snap guide colors, dimension list hover, per-tool midpoint settings, and bundled defaults

## Body (GitHub)

### Summary

Polish sketch-related UI and settings on branch **`Trailcode/ui_improvements`**: two-color snap guides (node vs axis), Sketch List dimension hover in the 3D view, updated bundled defaults and Settings labels, per-tool **Add midpoint nodes** options (square/rectangle on by default), consolidation of sketch annotation refresh APIs, sketch AIS type layout cleanup, and a small extrude finalize access fix.

### Problem

- Snap axis guides still appeared yellow because OCCT selection/highlight styling overrode configured colors; bundled defaults did not match the preferred dark-theme palette.
- **Restore defaults** did not set snap guide mode to **Both** because `snap_guide_mode` was missing from `res/ezycad_settings.json`.
- Sketch List dimensions had no 3D hover highlight (Shape List already did).
- A single global midpoint setting was awkward for shape tools; square and rectangle users expect edge-center snaps without enabling line-edge mids.
- Partial dimension refresh helpers (`refresh_edge_dimension_line_widths`, etc.) were unused dead code.
- `Shp_extrude` called private `Occt_view::finalize_sketch_extrude_()` and failed to compile after access tightening.

### Implemented scope

**Snap guides & defaults**

- Node vs axis snap colors (`snap_guide_color_node`, `snap_guide_color_axis`); deactivate snap AIS so OCCT highlight does not force yellow.
- Bundled defaults: lavender/magenta snap colors, purple element hover, dark view gradient, olive dimension color, **Both** snap mode, **All co-axial nodes** on.
- `get_snap_guide_color_axis()` returns `glm::vec3`.

**Sketch List / Settings UI**

- Dimension row hover uses **Element hover color** and thicker highlight line (`SetDimensionAspect` + `Redisplay`).
- Settings labels: **View presentation**, **Element hover color** (docs updated).

**Midpoint nodes (per tool group)**

- `gui.add_mid_pt_edges` — line / multi-line (default **off**).
- `gui.add_mid_pt_rect_edges` — square + rectangle tools (default **on**).
- `gui.add_mid_pt_slot_edges` — slot straight edges (default **off**).
- Separate Options checkbox per tool group; sync on mode switch.

**Refactors / fixes**

- `Sketch_annotation_refresh` struct replaces multiple refresh entry points; removed unused partial dimension refresh APIs and `apply_length_dimension_line_width` / `apply_length_dimension_arrow_size`.
- `Sketch_AIS_edge` / `Sketch_face_shp` in `utl_types.h`; `Sketch_AIS_node_mark` defined at end of `sketch.cpp`.
- `Shp_extrude`: `push_undo_snapshot()` + `finalize()` instead of private `Occt_view` helper.

### Acceptance criteria

- [ ] Snap node lock uses node color; axis-only snap uses axis color (not yellow).
- [ ] **Defaults** restores **Both** snap mode and bundled color palette.
- [ ] Sketch List dimension hover highlights matching row in 3D view.
- [ ] Square/rectangle Options show midpoint checkbox default **on**; line edge default **off**; settings persist independently.
- [ ] Build succeeds (`EzyCad`, `EzyCad_tests`).

### Files touched

- `src/sketch.cpp`, `src/sketch.h`, `src/sketch_nodes.cpp`, `src/sketch_nodes.h`, `src/sketch_delta.cpp`
- `src/gui.cpp`, `src/gui.h`, `src/gui_mode.cpp`, `src/gui_settings.cpp`
- `src/occt_view.cpp`, `src/occt_view.h`, `src/utl_geom.cpp`, `src/utl_geom.h`, `src/utl_types.h`
- `src/shp_extrude.cpp`, `src/shp_extrude.h`
- `res/ezycad_settings.json`
- `docs/usage-settings.md`, `docs/usage-sketch.md`, `docs/usage.md`, `docs/usage-occt-view.md`, `docs/bugs.md`
- `res/scripts/python/basic.py`, `CMakeLists.txt`

### Test plan

- [ ] Settings **Defaults** — snap mode **Both**, snap colors, dark view background.
- [ ] Sketch snap: full node lock vs X-only / Y-only — distinct colors.
- [ ] Sketch List: hover dimension row — 3D highlight visible.
- [ ] Square with default mids — edge center snap nodes; line edge with default — no mids.
- [ ] Slot / line / rect Options each toggle independent midpoint setting.
- [ ] Face extrude with typed distance — completes without compile/runtime errors.
- [ ] `cmake --build build --config Release --target EzyCad EzyCad_tests`

### Related

- Prior UI branch context: `agents/issues/003-ui-improvements-sketch-shape-lists-and-style.md`
- Midpoint option baseline: `agents/issues/011-edge-midpoint-option.md`
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/ui_improvements

---

**Post-creation notes:**

- GitHub issue: https://github.com/trailcode/EzyCad/issues/148
- GitHub PR: https://github.com/trailcode/EzyCad/pull/149
