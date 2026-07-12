---
github_issue: 200
github_pr: 201
status: active
paired_draft: ../issues/active/gh-200-undo-redo-typed-deltas.md
---

# PR - Typed undo/redo deltas for shapes and document ops

## Title

Typed undo/redo deltas for shapes and document ops

## Summary

- Interactive undo no longer copies the whole document for shape/sketch/underlay edits.
- Stable shape ids + typed deltas (`shp_delta`, `doc_delta`); `Shape_rec` holds `TopoDS_Shape`.
- JSON snapshots remain only for mixed delete and open-file checkpoint.
- Closes #200 (extends the sketch-delta approach from #144 / #145).

## Files Changed

- `src/shp_delta.h`, `src/shp_delta.cpp`
- `src/doc_delta.h`, `src/doc_delta.cpp`
- `src/shp.h`, `src/shp.cpp`
- `src/gui_occt_view.h`, `src/gui_occt_view.cpp`
- `src/gui.h`, `src/gui.cpp`
- `src/shp_*.cpp` (fuse/cut/common/fillet/chamfer/move/rotate/scale/polar_dup/extrude)
- `tests/shp_tests.cpp`
- `src/doc/undo-redo.md`, `src/doc/shape.md`, `src/doc/utility.md`
- `agents/drafts/issues/active/gh-200-undo-redo-typed-deltas.md`
- `agents/drafts/prs/active/gh-201-undo-redo-typed-deltas.md` (this draft)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/200
- PR: https://github.com/trailcode/EzyCad/pull/201
- Prior: https://github.com/trailcode/EzyCad/issues/144 , https://github.com/trailcode/EzyCad/pull/145

## Test Plan

- [x] Build `EzyCad_tests` (Release).
- [x] `EzyCad_tests.exe --gtest_filter=Shp_test.Undo*:Shp_test.Shape_ids*:Sketch_test.Undo*`
- [ ] Manual: box add, fuse, add sketch, underlay slider undo/redo.
- [ ] Mixed sketch-edge delete still uses snapshot path.

## Notes

- Branch commits include an early `WIP` squash candidate; optional follow-up to tidy history before merge.
- Underlay pixels remain in `Ezy_asset_store` (same as before); deltas store placement/asset id JSON only.

## Post-merge (checklist for the author)

- [ ] Close #200 if fully addressed (Closes link should do this).
- [ ] Move drafts to `archive/`.
- [ ] Consider `CHANGELOG.md` `[Unreleased]` entry.
