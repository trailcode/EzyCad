---
github_issue: 242
github_pr: 243
status: active
paired_draft: ../issues/active/gh-242-shape-copy-paste.md
---

# PR - copy-paste

## Title

In-app shape copy and paste (Ctrl+C / Ctrl+V)

## Summary

- Adds remappable **Ctrl+C** / **Ctrl+V** (`edit.copy` / `edit.paste`) for an in-app shape clipboard.
- Deep-copies selected solids, or a Shape List group subtree when the selection matches that group's descendant solids.
- Paste inserts under the current group (new ids, uniquified names, same pose) with one `Shape_add_delta`; clipboard survives **New**.
- Docs, settings defaults, and unit tests included.

## Files Changed

- `src/gui_hotkeys.h`, `src/gui_hotkeys.cpp`, `src/gui_mode.cpp`
- `src/gui_occt_view.h`, `src/gui_occt_view.cpp`
- `res/ezycad_settings.json`
- `tests/shp_tests.cpp`
- `docs/usage.md`, `docs/usage-settings.md`, `CHANGELOG.md`
- `src/doc/gui.md`, `src/doc/shape.md`
- `agents/drafts/issues/active/gh-242-shape-copy-paste.md`
- `agents/drafts/prs/active/gh-243-shape-copy-paste.md` (this draft)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/242
- PR: https://github.com/trailcode/EzyCad/pull/243
- Branch: `copy-paste`

## Test Plan

- [x] `EzyCad_tests --gtest_filter=Shp_test.Copy*:Shp_test.New_file*`
- [ ] Manual: select solids → Ctrl+C / Ctrl+V (in place, undo)
- [ ] Manual: click Shape List group → Ctrl+C → paste preserves nesting
- [ ] Manual: copy → New → paste into blank project
- [ ] Settings → Keyboard shortcuts shows Copy / Paste (Ctrl+C / Ctrl+V)
- [ ] Script console text Ctrl+C/V still works while typing
- [ ] Spot-check `docs/usage.md` / `usage-settings.md` / CHANGELOG

## Notes

- Closes #242
- Out of scope: OS clipboard, sketch copy/paste, cut, hold-key duplicate-translate
