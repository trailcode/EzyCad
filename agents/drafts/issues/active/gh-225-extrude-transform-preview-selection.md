---
github_issue: 225
github_pr: 226
status: active
paired_draft: ../prs/active/gh-226-extrude-transform-preview-selection.md
---

# Extrude/transform preview performance and multi-select retention

**Suggested labels:** `enhancement`, `bug`, `ui`

---

## Title (GitHub)

Extrude/transform preview performance and multi-select retention

## Body (GitHub)

### Summary

Sketch-face extrude and Move/Rotate/Scale live previews were too slow on dense geometry. Transform tools also dropped or collapsed multi-selection when entering the tool, finalizing, cancelling, or undoing.

### Problem

- **Extrude preview:** `MakePrism` / full BREP rebuild on every mouse move is expensive for dense sketch faces (many edges).
- **Move / scale / rotate preview:** calling `Redisplay` after `SetLocalTransformation` recomputed presentations and selection BVHs every frame.
- **Ghost highlight:** with transform-only preview, dynamic `MoveTo` could paint a wireframe ghost at the pre-transform pose.
- **Multi-select enter:** mode-switch faint/selection-mode redisplay cleared AIS selection; restoring AIS alone could leave only one operand after switching to whole-object pick.
- **Multi-select finalize:** baking + leaving the tool (and LMB release `SelectDetected`) collapsed the restored multi-selection to the shape under the cursor.
- **Undo/redo:** restoring `Mode::Move` / `Rotate` / `Scale` re-entered free-drag with a leftover selection.

### Implemented scope

**Performance**

- Extrude: hybrid preview — shaded prism for simple faces; translated face copies for dense faces when **Settings → Sketch → Appearance → Extrude fast preview** is on (edge-count threshold, default 24). Both-sides shows both end faces in lite mode; finalize always `MakePrism`.
- Move/Rotate/Scale: preview via `SetLocalTransformation` + `UpdateCurrentViewer` only (no per-frame `Redisplay`); `SetAllowHighlight(false)` while those tools are active.

**Selection**

- Snapshot enter selection; restore via `Occt_view::set_selected_shps`; seed tools with `begin(enter_selection)`.
- After finalize/cancel, re-select operands; skip AIS Press/Release on the finalize LMB so release cannot replace multi-select.
- Undo/redo maps Move/Rotate/Scale to the parent mode (`Normal`).

**Scripting**

- `ezy.view.set_selected(s1, ...)` (Python, Lua, remote client).

**Documentation**

- `CHANGELOG.md`, `docs/usage.md`, `docs/usage-settings.md`, `docs/scripting.md`, `src/doc/shape.md`, `src/doc/gui.md`, `src/doc/script.md`, `src/doc/undo-redo.md`.

### Acceptance criteria

- [ ] Dense sketch face extrude drag stays responsive with fast preview on; simple faces still show shaded prism; finalize builds a real solid.
- [ ] Move/Rotate/Scale drag of complex solids is smooth; no wireframe ghost at the original pose while idle.
- [ ] Select two solids → G/R/S → both move; after finalize (and Escape cancel) both remain selected.
- [ ] Undo after a multi-shape move returns to Normal without immediately dragging one leftover shape.
- [ ] `ezy.view.set_selected` / `get_selected` round-trip works in Python and Lua.
- [ ] User guide / Settings / CHANGELOG / `src/doc/*` match behavior.

### Files touched

- `src/shp_extrude.cpp` / `.h`
- `src/shp_operation.cpp` / `.h`
- `src/shp_move.cpp` / `.h`, `src/shp_rotate.cpp` / `.h`, `src/shp_scale.cpp` / `.h`
- `src/gui_occt_view.cpp` / `.h`, `src/gui.cpp`, `src/gui.h`, `src/gui_mode.cpp`, `src/gui_settings.cpp`
- `src/scr_python_console.cpp`, `src/scr_lua_console.cpp`, `scripts/ezycad/api.py`
- `docs/usage.md`, `docs/usage-settings.md`, `docs/scripting.md`
- `src/doc/shape.md`, `src/doc/gui.md`, `src/doc/script.md`, `src/doc/undo-redo.md`
- `CHANGELOG.md`
- `agents/drafts/issues/active/gh-225-extrude-transform-preview-selection.md` (this draft)
- `agents/drafts/prs/active/gh-226-extrude-transform-preview-selection.md`

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/225
- PR: https://github.com/trailcode/EzyCad/pull/226
- Follows #223 / #224
- Branch: `Trailcode/extrude_improve`

### Test plan

- [ ] Desktop Debug/Release build; `ctest -C Release`.
- [ ] Extrude dense face with fast preview on/off; both-sides lite faces; finalize solid.
- [ ] Move/Rotate/Scale complex solid: smooth drag, no ghost highlight.
- [ ] Multi-select → G → move → LMB finalize: both still selected; Escape cancel same.
- [ ] Undo after multi move: Normal mode, no auto-drag.
- [ ] Script: `view.set_selected(view.get_selected())`.
- [ ] Spot-check Settings Extrude fast preview help and user docs.
