---
github_issue: 194
github_pr: 195
status: active
paired_draft: ../issues/active/gh-194-sketch-appearance-settings.md
---

# PR - `Trailcode/sketch-colors`

## Title

Configurable sketch edge/face colors and Settings header persistence

## Summary

- **Sketch appearance:** Settings -> Sketch -> Appearance adds edge color/thickness, edge selection vs hover highlight colors, and face fill / selection fill / highlight fill (RGBA, alpha = opacity). Applied live via `Sketch_annotation_refresh::edge_face_style`.
- **OCCT:** Separate `SetHilightAttributes` (selection) and `SetDynamicHilightAttributes` (hover); faces use shaded hilight so fill colors show.
- **Settings UI:** Collapsing section open state persisted in `gui.settings_headers`.
- **Defaults / docs:** Bundled palette and user/dev docs updated.

## Files Changed

`src/gui.h`, `src/gui_settings.cpp`, `src/sketch.h`, `src/sketch.cpp`, `src/sketch_display.cpp`, `src/sketch_topo.cpp`, `src/doc/gui.md`, `src/doc/sketch.md`, `docs/usage-settings.md`, `docs/usage-sketch.md`, `res/ezycad_settings.json`, `CHANGELOG.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/194
- PR: https://github.com/trailcode/EzyCad/pull/195
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/sketch-colors

## Test Plan

- [ ] Build Release `EzyCad`.
- [ ] Appearance colors update live; selection vs hover distinct for edges and faces.
- [ ] Face selection/highlight tint fill (shaded), not wire-only.
- [ ] Settings headers persist after restart; **Defaults** restores bundled palette.
- [ ] Optional docs build for `usage-settings.md`.
