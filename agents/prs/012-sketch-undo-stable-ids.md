# PR - `Trailcode/sketch-undo-stable-ids`

## Title

Sketch undo: stable sketch ids for delta resolve, operation-axis undo, and undo stack byte budget

## Summary

- **Stable sketch ids:** Each sketch gets a monotonic `uint64_t` id (persisted in JSON). `Sketch_delta` resolves the target sketch by id, not display name — fixes Release redo crash from dangling `Sketch*` after reload/recreation.
- **Operation axis undo:** `clear_operation_axis_undoable()` for Options **Clear axis**; axis node recording on finalize; redo-clear via `apply_forward_`. OCCT handle `Nullify()` after `Remove` in `clear_operation_axis()`.
- **Undo stack hygiene:** `Undo_entry_kind`, per-entry `footprint_bytes`, 32 MB cap trimming oldest document snapshots first; debug panel shows stack MB (Debug builds).
- **Tests:** GUI undo/redo regression; reload-by-id redo test; undo tests on `curr_sketch()`.

Closes sketch undo stability work tracked in `agents/issues/017-sketch-undo-stable-ids.md`.

## Files Changed

12 files (+325 / -64 vs `main`): `src/sketch*.cpp/h`, `src/sketch_delta.*`, `src/sketch_json.cpp`, `src/occt_view.*`, `src/delta.h`, `src/gui.cpp`, `src/gui_mode.cpp`, `tests/sketch_tests.cpp`, `docs/bugs.md`.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/150
- PR: https://github.com/trailcode/EzyCad/pull/151
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/sketch-undo-stable-ids
- Prior: #145 sketch delta undo

## Test Plan

- [x] Build Release: `EzyCad_tests`.
- [x] Run: `EzyCad_tests.exe --gtest_filter=Sketch_test.Undo*:Sketch_test.Gui_UndoRedo*`
- [ ] Manual: edge + operation axis; undo 4x, redo 2x (Release — former crash).
- [ ] Manual: Clear axis → undo → redo.
- [ ] Manual: save document, reload, redo sketch delta.

## Notes

- Old `.ezy` files without `"id"` allocate new ids on load (backward compatible).
- `approximate_undo_bytes()` is internal; user sees summed MB only in Debug **dbg** window.
