# Sketch delta-based undo/redo for sketch operations

**Suggested labels:** `enhancement`, `sketch`, `refactor`

---

## Title (GitHub)

Replace sketch JSON undo snapshots with delta-based `Sketch_delta` / `Sketch_op_recorder`

## Body (GitHub)

### Summary

Sketch edits previously pushed full JSON undo snapshots, which was heavy and made precise undo of individual geometry changes harder to reason about. This change introduces a `Delta` hierarchy, records explicit `prev_*` / `curr_*` lists during each sketch operation via an RAII `Sketch_op_recorder`, and applies those lists on undo/redo. Arc, circle, linear edge, length dimension, and operation-axis changes are covered on the paths wired in `sketch.cpp`.

### Problem

- Full JSON snapshots for every sketch undo step are expensive and opaque; diffing element snapshots was fragile.
- Arc and circle undo did not reliably remove or restore both internal edges per arc segment.
- Failed sketch operations (e.g. mirror with no edges selected) could leave an empty recorder scope that should not push undo.
- Implementation details (`Prev_edge_rec`, apply logic, etc.) belonged in `.cpp` files behind PIMPL, not in public headers.

### Implemented scope

**Code changes:**

- `src/delta.h`: abstract `Delta` base (`apply_forward`, `apply_reverse`, `clone`).
- `src/sketch_delta.h` / `src/sketch_delta.cpp`: `Sketch_op_recorder` and `Sketch_delta` with PIMPL; record types (`Prev_edge_rec`, `Curr_linear_edge_record`, `Arc_edge_record`, `Length_dim_record`) live in `Sketch_delta::Impl`.
- `src/occt_view.h` / `src/occt_view.cpp`: undo stack holds `std::unique_ptr<Delta>`; `push_undo_delta()` alongside existing JSON `push_undo_snapshot()` for non-sketch ops.
- `src/sketch.cpp` / `src/sketch.h`: sketch finalize, split, add-edge, mirror, and related paths wrap work in `Sketch_op_recorder`; `m_undo_recorder` set during active recorder scope; arc/circle add records curr edges and nodes; `remove_arc_edge_` matches arc pairs by shared AIS shape.
- `src/sketch_nodes.cpp` / `src/sketch_nodes.h`: `restore_node_at` logic moved into `Sketch_nodes::Impl`.
- `src/gui.cpp` / `src/gui.h`: `sketch_left_click()` and `mirror_selected_sketch_edges()` extracted for headless GUI tests.
- `tests/sketch_tests.cpp`: undo tests for arc, circle, crossing linear finalize; `MirrorSelectedEdges_NoEdgesSelected` asserts no undo push on failed mirror.

**Documentation:**

- `docs/ezycad_code_style.md`: reader-first `.cpp` layout, file-local helpers at bottom, PIMPL guidance (`Sketch_nodes`, `Sketch_delta`).

### Acceptance criteria

- [ ] Linear sketch edge add/finalize undo restores prior geometry and splits correctly.
- [ ] Arc and circle undo remove/restore all internal arc edges; redo restores them.
- [ ] Mirror with no selection shows error and does **not** push an undo step.
- [ ] `Sketch_delta` / `Sketch_op_recorder` use PIMPL; record structs are not in the public header.
- [ ] Non-sketch operations still use JSON undo snapshots where unchanged.
- [ ] `EzyCad_tests` pass (see test plan).

### Files touched

- `src/delta.h`, `src/sketch_delta.h`, `src/sketch_delta.cpp`
- `src/occt_view.h`, `src/occt_view.cpp`
- `src/sketch.h`, `src/sketch.cpp`, `src/sketch_json.cpp`
- `src/sketch_nodes.h`, `src/sketch_nodes.cpp`
- `src/gui.h`, `src/gui.cpp`, `src/gui_mode.cpp`
- `tests/sketch_tests.cpp`
- `docs/ezycad_code_style.md`
- `agents/drafts/issues/archive/gh-144-sketch-delta-undo-redo.md` (this draft)

### Related

- Branch: `Trailcode/better_undo_redo`
- Draft PR: `agents/drafts/prs/archive/gh-145-sketch-delta-undo-redo.md`
- GitHub issue: https://github.com/trailcode/EzyCad/issues/144
- GitHub PR: https://github.com/trailcode/EzyCad/pull/145

### Test plan

- [ ] Build `EzyCad_tests` (Release).
- [ ] Run:
  `EzyCad_tests.exe --gtest_filter=Sketch_test.Undo*:Sketch_test.MirrorSelectedEdges_NoEdgesSelected`
- [ ] Manual: add linear edge, undo/redo; add arc/circle, undo/redo; mirror edges with selection; mirror with no selection (no undo entry).
- [ ] Confirm shape ops and other undo paths still work via JSON snapshots.
