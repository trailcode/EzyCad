---
github_issue: 248
github_pr:
status: active
paired_draft:
---

# Shape List Zoom to, shaft-align mode rename, and third_party IDE filters

**Suggested labels:** `enhancement`, `ui`

---

## Title (GitHub)

Shape List Zoom to, shaft-align mode rename, and third_party IDE filters

## Body (GitHub)

### Summary

Add Shape List **Zoom to**, rename Align shafts mode to `Mode::Shape_shaft_align`, nest vendored compile units under a `third_party` IDE filter in Visual Studio, and keep developer docs aligned with Options / Tab / Enter routing for Align shafts.

### Problem

- No quick way to frame a Shape List solid or group in the 3D view without changing camera orientation manually.
- Mode name `Shape_cyl_align` no longer matches the user-facing **Align shafts** tool.
- TextEditor / tinyfiledialogs / imgui sat in default or top-level VS filters instead of under `third_party`.
- `gui.md` Options / keyboard dispatch needed explicit Align shafts coverage.

### Scope

- **Zoom to** on Shape List name and **M** context menus; `Occt_view::fit_shapes_in_view` (solid or group descendant solids; keeps camera orientation).
- Rename `Mode::Shape_cyl_align` → `Mode::Shape_shaft_align` (hotkey id `mode.cyl_align` / `Shp_cyl_align` unchanged for now).
- CMake `source_group(TREE ...)` so `EzyCad_lib` shows `third_party/{imgui,ImGuiColorTextEdit,tinyfiledialogs}`.
- `src/doc/gui.md` / `shape.md`, `docs/usage.md`, `CHANGELOG.md`.

### Acceptance criteria

- [ ] Shape List solid/group **Zoom to** frames selection with current camera orientation
- [ ] Empty group **Zoom to** disabled; solids with null geometry not zoomable via name menu
- [ ] Mode enum / Options / hotkey dispatch use `Shape_shaft_align`
- [ ] VS Solution Explorer (`EzyCad_lib`) shows vendored sources under `third_party` after CMake reconfigure + reload
- [ ] User docs and CHANGELOG mention Zoom to; developer docs list Align shafts Options/key routing

### Out of scope

- Renaming `Shp_cyl_align` / `mode.cyl_align` / `Gui_action::Mode_cyl_align`
- Changing Zoom to camera orientation (fit only)

### Files touched

- `src/gui.cpp`, `src/gui.h`, `src/gui_mode.cpp`, `src/gui_occt_view.cpp`, `src/gui_occt_view.h`, `src/mode.h`
- `CMakeLists.txt`
- `docs/usage.md`, `CHANGELOG.md`, `src/doc/gui.md`, `src/doc/shape.md`
- `agents/drafts/issues/active/gh-248-zoom-to-shaft-align-ide.md` (this draft)

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/248
- Prior: Align shafts (#247 / #246)
- Branch: `Trailcode/zoom-to`
