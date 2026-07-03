# PR - `Trailcode/ui_improvements`

## Title

Sketch UI: snap colors, dimension list hover, per-tool midpoint settings, and bundled defaults

## Summary

- **Snap guides:** Separate node (vertex lock) and axis (X/Y only) colors; prevent OCCT default yellow highlight on snap AIS shapes. Update bundled defaults (lavender/magenta snaps, dark view, purple element hover, **Both** snap mode, co-axial overlay on).
- **Sketch List:** Hover a dimension row to highlight its length dimension in the 3D view (same pattern as Shape List; uses **Element hover color**).
- **Settings / docs:** Rename **3D view background** to **View presentation** and **Shape list hover color** to **Element hover color**; document new snap color keys and defaults.
- **Midpoint nodes:** Split into three persisted settings — line/multi-line (off), square/rectangle (on), slot (off) — each with its own Options checkbox.
- **Refactor:** `Sketch_annotation_refresh` groups dimension/node refresh; remove unused partial dimension refresh APIs. Move `Sketch_AIS_edge` / `Sketch_face_shp` to `utl_types.h`; `Sketch_AIS_node_mark` lives in `sketch.cpp`.
- **Fix:** `Shp_extrude` finalizes via public `push_undo_snapshot()` + `finalize()` (no private `Occt_view` access).

## Files Changed

27 files (+863 / -519 vs `main`): `src/sketch*.cpp/h`, `src/sketch_nodes.*`, `src/gui*.cpp/h`, `src/gui_mode.cpp`, `src/gui_settings.cpp`, `src/occt_view.*`, `src/utl_geom.*`, `src/utl_types.h`, `src/shp_extrude.*`, `res/ezycad_settings.json`, docs, `res/scripts/python/basic.py`, `CMakeLists.txt`.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/148
- PR: https://github.com/trailcode/EzyCad/pull/149
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/ui_improvements
- Builds on earlier `Trailcode/ui_improvements` list-pane work (`agents/drafts/issues/archive/gh-98-ui-improvements-sketch-shape-lists-and-style.md`)

## Test Plan

- [ ] Build Release: `EzyCad`, `EzyCad_tests`.
- [ ] Settings **Defaults** — snap mode **Both**, new color palette, dark view.
- [ ] Sketch snap: node lock vs axis-only — distinct colors (not yellow).
- [ ] Sketch List dimension hover — 3D highlight tracks row.
- [ ] Square (default mids on) vs line edge (default off) — correct midpoint behavior.
- [ ] Toggle midpoint Options independently for line, square/rect, slot; restart app — settings persist.
- [ ] Sketch face extrude with Tab distance entry — completes and creates shape.
- [ ] Optional: `sphinx-build -b html -W docs docs/_build` for doc label changes.

## Notes

- Six commits on branch (`WIP` x2, `Refactor`, `WIP`, `Simplify`, `Improve`); squash message can follow PR title.
- Legacy JSON key `gui.add_mid_pt_edges` still controls line/multi-line mids only.
- Doc follow-up: expand `usage-settings.md` JSON table for `add_mid_pt_rect_edges` / `add_mid_pt_slot_edges` and square/rectangle/slot Options sections in `usage-sketch.md`.
