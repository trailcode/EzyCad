---
github_issue: 225
github_pr: 226
status: active
paired_draft: ../issues/active/gh-225-extrude-transform-preview-selection.md
---

# PR - Trailcode/extrude_improve

## Title

Extrude/transform preview performance and multi-select retention

## Summary

- **Extrude:** hybrid live preview (shaded prism for simple faces; translated face copies for dense faces when Settings Extrude fast preview is on); both-sides lite faces; finalize always builds the solid.
- **Move / Rotate / Scale:** preview without per-frame `Redisplay`; disable dynamic highlight while active to avoid ghost wireframes; keep and restore multi-selection on enter, finalize, and Escape; skip AIS select on the finalize LMB release; undo/redo returns to Normal instead of re-entering the free-drag tool.
- **Scripting:** `ezy.view.set_selected(...)` for Python, Lua, and the remote client.

## Files Changed

- `src/shp_extrude.cpp` / `.h`
- `src/shp_operation.cpp` / `.h`
- `src/shp_move.cpp` / `.h`, `src/shp_rotate.cpp` / `.h`, `src/shp_scale.cpp` / `.h`
- `src/gui_occt_view.cpp` / `.h`, `src/gui.cpp`, `src/gui.h`, `src/gui_mode.cpp`, `src/gui_settings.cpp`
- `src/scr_python_console.cpp`, `src/scr_lua_console.cpp`, `scripts/ezycad/api.py`
- `docs/usage.md`, `docs/usage-settings.md`, `docs/scripting.md`
- `src/doc/shape.md`, `src/doc/gui.md`, `src/doc/script.md`, `src/doc/undo-redo.md`
- `CHANGELOG.md`
- `agents/drafts/issues/active/gh-225-extrude-transform-preview-selection.md`
- `agents/drafts/prs/active/gh-226-extrude-transform-preview-selection.md` (this draft)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/225
- PR: https://github.com/trailcode/EzyCad/pull/226
- Follows #223 / #224
- Branch: `Trailcode/extrude_improve`

## Test Plan

- [ ] Desktop Debug/Release build; `ctest -C Release`
- [ ] Extrude dense face with Extrude fast preview on/off; both-sides; finalize solid
- [ ] Move/Rotate/Scale complex solid: smooth drag, no ghost at original pose
- [ ] Select two solids → G → move → LMB finalize: both stay selected; Escape cancel same
- [ ] Undo after multi move: Normal mode, no auto-drag of one shape
- [ ] Script: `view.set_selected(view.get_selected())`
- [ ] Spot-check Settings Extrude fast preview, `docs/usage.md` / `usage-settings.md` / `scripting.md`, CHANGELOG

## Notes

- Builds on #224 selection-restore / extrude-finalize patterns; adds hybrid extrude preview, transform preview perf, post-finalize multi-select, and `set_selected` bindings.
