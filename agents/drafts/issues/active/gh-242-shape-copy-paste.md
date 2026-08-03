---
github_issue: 242
github_pr: 243
status: active
paired_draft: ../prs/active/gh-243-shape-copy-paste.md
---

# In-app shape copy and paste (Ctrl+C / Ctrl+V)

**Suggested labels:** `enhancement`, `ui`

---

## Title (GitHub)

In-app shape copy and paste (Ctrl+C / Ctrl+V)

## Body (GitHub)

### Summary

Add remappable **Ctrl+C** / **Ctrl+V** for an in-app clipboard that deep-copies selected solids and Shape List group subtrees, with undo via `Shape_add_delta`. Sketch entities and the OS clipboard are out of scope.

### Problem

- No CAD-level copy/paste existed; closest tools were polar duplicate, sketch mirror, and ImGui text clipboard.
- Users need to duplicate solids/groups and seed a new project from a selection (copy -> New -> paste).

### Implemented scope

**Code:**

- `Gui_action::Edit_copy` / `Edit_paste` (`edit.copy` / `edit.paste`, defaults Ctrl+C / Ctrl+V)
- `Occt_view::copy_selected_shapes` / `paste_clipboard_shapes` — session clipboard; group subtree when selection matches current group's descendant solids; paste under `current_group_id` with new ids and uniquified names
- Clipboard survives **New** (not cleared on `new_file`)

**Docs / tests:**

- `docs/usage.md`, `docs/usage-settings.md`, `CHANGELOG.md`, `src/doc/gui.md`, `src/doc/shape.md`
- Unit tests in `tests/shp_tests.cpp` (`Copy_*`, `New_file_keeps_shape_clipboard`)

### Acceptance criteria

- [x] Ctrl+C / Ctrl+V remappable; work when ImGui does not want text input
- [x] Copy solids; copy group subtree when Shape List group selection matches all descendants
- [x] Paste deep-copies under current group; one undo step; same pose
- [x] Clipboard survives New project
- [x] Unit tests pass; user docs and CHANGELOG updated

### Out of scope

- OS / cross-app clipboard
- Sketch edge copy/paste
- Ctrl+X cut
- Hold-key duplicate-translate (`edit.duplicate_translate`)

### Files touched

- `src/gui_hotkeys.h`, `src/gui_hotkeys.cpp`, `src/gui_mode.cpp`
- `src/gui_occt_view.h`, `src/gui_occt_view.cpp`
- `res/ezycad_settings.json`
- `tests/shp_tests.cpp`
- `docs/usage.md`, `docs/usage-settings.md`, `CHANGELOG.md`
- `src/doc/gui.md`, `src/doc/shape.md`
- `agents/drafts/issues/active/gh-242-shape-copy-paste.md` (this draft)

### Related

- Issue: https://github.com/trailcode/EzyCad/issues/242
- PR: https://github.com/trailcode/EzyCad/pull/243
- Branch: `copy-paste`
- Follow-up: hold-key duplicate translate (configurable-hotkeys plan)
- Shape List duplicate group (hierarchy phase 2, #215)

### Test plan

- [x] `EzyCad_tests --gtest_filter=Shp_test.Copy*:Shp_test.New_file*`
- [ ] Manual: select solids Ctrl+C / Ctrl+V; Shape List group copy; copy -> New -> paste
- [ ] Settings Keyboard shortcuts shows Copy / Paste
- [ ] Script console text Ctrl+C/V still works (WantTextInput)
