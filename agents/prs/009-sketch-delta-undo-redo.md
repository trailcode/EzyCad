# PR - Trailcode/better_undo_redo

## Title

Sketch delta-based undo/redo with `Sketch_op_recorder` and PIMPL `Sketch_delta`

## Summary

- **Delta undo for sketches:** New `Delta` base and `Sketch_delta` record explicit `prev_*` / `curr_*` state (linear edges, arcs, length dims, operation axis, node tombstones) instead of full JSON snapshots for sketch operations.
- **`Sketch_op_recorder`:** RAII scope records changes during one edit; `commit()` pushes a delta, `cancel()` drops it (e.g. mirror with no selection). Destructor cancels if still uncommitted.
- **Arc/circle undo:** Fixed internal-edge removal/restoration; circle undo covers both semicircles (four internal edges).
- **PIMPL:** `Sketch_delta` and `Sketch_op_recorder` hide implementation in `.cpp`; record types live in `Sketch_delta::Impl`. `Sketch_nodes::restore_node_at` moved to `Sketch_nodes::Impl`.
- **Tests / GUI hooks:** Headless tests for undo (arc, circle, crossing edges) and mirror-with-no-selection cancel path; `GUI::sketch_left_click()` and `GUI::mirror_selected_sketch_edges()` for testability.
- **Code style doc:** Reader-first `.cpp` order, helpers at file bottom, PIMPL examples.

Closes sketch delta undo work tracked in `agents/issues/014-sketch-delta-undo-redo.md`.

## Files Changed

- `src/delta.h`
- `src/sketch_delta.h`, `src/sketch_delta.cpp`
- `src/occt_view.h`, `src/occt_view.cpp`
- `src/sketch.h`, `src/sketch.cpp`, `src/sketch_json.cpp`
- `src/sketch_nodes.h`, `src/sketch_nodes.cpp`
- `src/gui.h`, `src/gui.cpp`, `src/gui_mode.cpp`
- `tests/sketch_tests.cpp`
- `docs/ezycad_code_style.md`
- `agents/issues/014-sketch-delta-undo-redo.md`, `agents/prs/009-sketch-delta-undo-redo.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/144
- PR: https://github.com/trailcode/EzyCad/pull/145
- Compare / branch: `Trailcode/better_undo_redo`
- Draft: `agents/issues/014-sketch-delta-undo-redo.md`

## Test Plan

- [x] Build `EzyCad_lib` and `EzyCad_tests` (Release).
- [x] Run:
  `EzyCad_tests.exe --gtest_filter=Sketch_test.Undo*:Sketch_test.MirrorSelectedEdges_NoEdgesSelected`
  (4 tests: arc undo, circle undo, crossing linear finalize undo, mirror no-selection cancel)
- [ ] Manual: sketch linear edge add + undo/redo; arc/circle + undo/redo; mirror with axis and selected edges; mirror with no selection (error, undo stack unchanged).
- [ ] Smoke: shape fuse/cut/chamfer undo still uses JSON snapshots.
- [ ] No user-facing docs required for this refactor (internal undo architecture).

## Notes

- `push_undo_snapshot()` remains for non-sketch operations (shapes, GUI material changes, etc.).
- `Sketch_op_recorder::cancel()` is called explicitly when mirror finds no selected edges; otherwise uncommitted recorders cancel in the destructor.
- `note_prev_arc_edge` is available on the recorder but linear-style prev filtering at op start is the primary wired path; arc prev recording may expand in follow-up ops.
- Record types intentionally live in `Sketch_delta::Impl`, not the public header.

## Post-merge (checklist for the author)

- [x] Open GitHub issue from `agents/issues/014-sketch-delta-undo-redo.md` and link PR.
- [x] Fill issue/PR URLs in both agent drafts.
- [ ] Consider `[Unreleased]` CHANGELOG entry if sketch undo behavior is user-visible enough to mention.
