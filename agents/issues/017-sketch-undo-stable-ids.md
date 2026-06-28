# Sketch undo: stable ids, operation-axis undo, stack budget

**Suggested labels:** `bug`, `sketch`, `enhancement`

**Branch:** `Trailcode/sketch-undo-stable-ids` (base: `main`)

---

## Title (GitHub)

Sketch undo: stable sketch ids for delta resolve, operation-axis undo, and undo stack byte budget

## Body (GitHub)

### Summary

Harden sketch delta undo/redo on branch **`Trailcode/sketch-undo-stable-ids`**: assign each sketch a stable persisted **id** for `Sketch_delta` lookup (display names may duplicate), fix operation-axis undo/clear and OCCT handle teardown, add undo-stack hygiene (entry kind + byte budget), and regression tests including a GUI-driven undo/redo path.

### Problem

- **Release crash on redo:** `Sketch_delta` cached a raw `Sketch*` and returned it without validation; after JSON reload or sketch recreation the pointer was dangling (`add_edge_impl_` read access violation).
- **Name-based resolve was fragile:** duplicate sketch names (rename UI, `"Sketch from face"`, load) could target the wrong sketch silently.
- **Operation axis undo incomplete:** Clear axis was not undoable; redo-after-clear could leave a stale axis; OCCT `Standard_ProgramError` when clearing axis if AIS handle was not nulled after `Remove`.
- **Undo stack memory:** 50 full JSON snapshots could dominate memory; no byte budget or debug footprint summary.

### Implemented scope

**Stable sketch ids**

- `Sketch::get_id()` / `uint64_t m_id`; allocated via `Occt_view::allocate_sketch_id()`.
- Persisted in sketch JSON as `"id"`; backward compatible when field missing.
- `Sketch_delta` resolves target sketch by **id**, not name.

**Operation axis undo**

- `clear_operation_axis_undoable()` for Options **Clear axis**; records prev axis in delta.
- `finalize_operation_axis_` records axis endpoint nodes for undo tombstone.
- `apply_forward_` clears axis when redoing a clear step.
- `clear_operation_axis()`: `Nullify()` AIS handle after `Remove`.

**Undo stack hygiene**

- `Undo_entry_kind`: `Sketch_delta` vs `Document_snapshot`.
- `k_max_undo_bytes` (32 MB): trim oldest document snapshots first when over budget.
- Debug panel shows undo/redo step count and total MB (Debug builds only).

**Tests**

- `Gui_UndoRedo_edge_then_operation_axis_four_undo_two_redo` (GUI path).
- `Undo_delta_resolves_sketch_by_id_after_reload`.
- Existing undo tests use `curr_sketch()` (document-owned sketches).

### Acceptance criteria

- [ ] Add edge + operation axis, undo/redo repeatedly — no crash (Release).
- [ ] Clear axis is undoable; redo clear works.
- [ ] JSON save/load preserves sketch `id`; redo after reload applies to correct sketch.
- [ ] Duplicate sketch display names do not break delta undo.
- [ ] `EzyCad_tests`: `Sketch_test.Undo*`, `Sketch_test.Gui_UndoRedo*`.

### Files touched

- `src/delta.h`, `src/sketch.h`, `src/sketch.cpp`, `src/sketch_delta.h`, `src/sketch_delta.cpp`, `src/sketch_json.cpp`
- `src/occt_view.h`, `src/occt_view.cpp`, `src/gui.cpp`, `src/gui_mode.cpp`
- `tests/sketch_tests.cpp`, `docs/bugs.md`
- `agents/issues/017-sketch-undo-stable-ids.md`, `agents/prs/012-sketch-undo-stable-ids.md`

### Related

- Builds on sketch delta undo (#144 / #145).
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/sketch-undo-stable-ids
- GitHub issue: https://github.com/trailcode/EzyCad/issues/150
- GitHub PR: https://github.com/trailcode/EzyCad/pull/151

### Test plan

- [ ] Build Release: `EzyCad`, `EzyCad_tests`.
- [ ] Run: `EzyCad_tests.exe --gtest_filter=Sketch_test.Undo*:Sketch_test.Gui_UndoRedo*`
- [ ] Manual: edge + operation axis; undo 4x, redo 2x (former Release crash).
- [ ] Manual: Clear axis → undo → redo.
- [ ] Manual: save `.ezy`, reload, redo sketch delta step.

---

**Post-creation notes:**

- GitHub issue: https://github.com/trailcode/EzyCad/issues/150
- GitHub PR: https://github.com/trailcode/EzyCad/pull/151
