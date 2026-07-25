---
github_issue: 223
github_pr: 224
status: active
paired_draft: ../issues/active/gh-223-gen-fixes-rotate-selection-extrude.md
---

# PR - Trailcode/gen-fixes

## Title

Fix rotate Tab degrees, selection restore, and extrude finalize

## Summary

- Fix shape **Rotate** Tab angle entry to use the degrees popup (`deg`) instead of length units; Enter/click-away commit works again when a unit suffix is shown.
- Preserve AIS selection when entering **Move** / **Rotate** / **Scale** (same pattern as cross-section).
- Extrude LMB finalize no longer re-picks a face on the same click; thin extrude finalize/cancel wrappers removed.

## Files Changed

- `src/shp_rotate.cpp`
- `src/gui.cpp`
- `src/gui_mode.cpp`
- `src/gui_occt_view.cpp`
- `src/gui_occt_view.h`
- `src/shp_extrude.cpp`
- `src/doc/gui.md`
- `src/doc/shape.md`
- `CHANGELOG.md`
- `agents/drafts/issues/active/gh-223-gen-fixes-rotate-selection-extrude.md`
- `agents/drafts/prs/active/gh-224-gen-fixes-rotate-selection-extrude.md` (this draft)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/223
- PR: https://github.com/trailcode/EzyCad/pull/224
- Branch: `Trailcode/gen-fixes`

## Test Plan

- [ ] Desktop Debug/Release build
- [ ] Rotate: select shape(s) → R → Tab → type degrees → Enter finalizes; popup shows `deg`
- [ ] Move Tab distance: Enter / click-away commits with `in`/`mm` suffix
- [ ] Pre-select solids → G / R / S: selection kept for first drag
- [ ] Active extrude preview → LMB confirms once (no immediate re-pick)
- [ ] Spot-check `docs/usage.md` Rotate section; `CHANGELOG.md` Fixed entries

## Notes

- User guide already documented Rotate Tab as degrees; no `docs/usage.md` change.
