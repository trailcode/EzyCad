---
github_issue: 248
github_pr: 249
status: active
paired_draft: ../issues/active/gh-248-zoom-to-shaft-align-ide.md
---

# PR - Zoom to / shaft-align / third_party IDE

## Title

Shape List Zoom to, shaft-align mode rename, and third_party IDE filters

## Summary

- Shape List **Zoom to** (name and **M** menus) via `Occt_view::fit_shapes_in_view` — frames a solid or group descendant solids while keeping camera orientation.
- Rename `Mode::Shape_cyl_align` → `Mode::Shape_shaft_align` (hotkey id `mode.cyl_align` / `Shp_cyl_align` unchanged).
- Nest imgui / TextEditor / tinyfiledialogs under `third_party` IDE filters in `EzyCad_lib` (CMake `source_group`).
- Sync `gui.md` Align shafts Options / Tab / Enter / dist-edit routing; `usage.md` + CHANGELOG for Zoom to.

## Files Changed

- `src/gui.cpp`, `src/gui.h`, `src/gui_mode.cpp`, `src/gui_occt_view.cpp`, `src/gui_occt_view.h`, `src/mode.h`
- `CMakeLists.txt`
- `docs/usage.md`, `CHANGELOG.md`, `src/doc/gui.md`, `src/doc/shape.md`
- `agents/drafts/issues/active/gh-248-zoom-to-shaft-align-ide.md`
- `agents/drafts/prs/active/gh-249-zoom-to-shaft-align-ide.md` (this draft)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/248
- PR: https://github.com/trailcode/EzyCad/pull/249
- Branch: `Trailcode/zoom-to`
- Prior: Align shafts (#247 / #246)

## Test Plan

- [ ] Build Release `EzyCad`
- [ ] Shape List solid: right-click name / **M** → **Zoom to** frames solid, orientation unchanged
- [ ] Group with solids: **Zoom to** frames all descendants; empty group item disabled
- [ ] Align shafts (**J**): Options Flip / Clock rotation still work after mode rename
- [ ] Reconfigure CMake, reload `build/EzyCad.sln`: `EzyCad_lib` → `third_party/{imgui,ImGuiColorTextEdit,tinyfiledialogs}`
- [ ] Spot-check `docs/usage.md` Shape List, `src/doc/gui.md` Options/keyboard, CHANGELOG

## Notes

- Closes #248
- Out of scope: renaming `Shp_cyl_align` / `mode.cyl_align` / `Gui_action::Mode_cyl_align`
