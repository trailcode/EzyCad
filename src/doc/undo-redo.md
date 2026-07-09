# Undo / redo

Developer reference for EzyCad's document history. Public C++ entry points:

- [`delta.h`](../delta.h) — abstract undo step
- [`sketch_op_recorder.h`](../sketch_op_recorder.h) — sketch operation recorder (public API)
- [`gui_occt_view.h`](../gui_occt_view.h) — stack storage and `undo()` / `redo()`

Maintainers: update this file when undo stack layout, delta types, or push/pop contracts change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing shortcuts are documented in [`docs/usage.md`](../../docs/usage.md).

## Purpose

EzyCad keeps a bounded history of user edits so **Ctrl+Z** / **Ctrl+Y** (and Edit menu items) can step backward and forward through the session. History is owned by `Occt_view` and spans both 2D sketch geometry and the wider 3D document (shapes, sketches, underlays, imports).

The implementation uses **two strategies** on one stack:

| Strategy | When used | Stored payload |
| --- | --- | --- |
| **Element delta** | Sketch geometry edits | `std::unique_ptr<Delta>` (`Sketch_op_delta` in `sketch_op_recorder.cpp`) |
| **Full JSON snapshot** | Everything else | `to_json()` string of the whole document |

Sketch deltas are cheaper and precise; JSON snapshots are simple and cover arbitrary document changes (shape booleans, sketch add/remove, file load, etc.).

## Requirements and invariants

### Stack ownership

- `Occt_view` holds `m_undo_stack` and `m_redo_stack` (`std::vector<Undo_entry>`).
- Maximum depth: `k_max_undo` (50). Oldest entries are dropped from the front when exceeded.
- Any new user edit clears `m_redo_stack` (standard linear history).

### `Undo_entry`

Each stack frame stores:

| Field | Role |
| --- | --- |
| `delta` | Non-null for sketch element steps; mutually exclusive with `json` |
| `json` | Full-document snapshot when `delta` is null |
| `mode` | `Mode` active when the operation ran; restored on undo/redo |

### `m_restoring` guard

While `Occt_view::undo()` or `redo()` runs, `m_restoring` is true. During that window:

- `push_undo_snapshot()`, `push_undo_delta()`, and `pop_undo_snapshot()` are no-ops.
- `load()` does **not** clear undo/redo stacks (the navigation itself uses `load(json, false)` for snapshot steps).

This prevents re-entrant history pushes while applying a step.

### Mode restoration

After applying a delta or reloading a snapshot, `Occt_view` calls `GUI::set_mode(state.mode)`. If the stored mode is `Sketch_inspection_mode`, the sketch list pane is shown again.

### View camera

Snapshot undo/redo calls `load(state.json, false)` so **camera pan/zoom/rotate are preserved** across JSON-based steps. Delta steps mutate sketch data in place and do not touch the view matrix.

### Stable sketch identity

`Sketch_op_delta` keys sketches by stable `sketch_id` (not display name or list index). Node indices are never compacted; undo tombstones nodes created during an operation. See [sketch.md](sketch.md#requirements-and-invariants).

## Architecture

```
GUI (menus, hotkeys, some UI-driven snapshots)
  |
  +-- Occt_view::undo() / redo()
  |     m_undo_stack / m_redo_stack (Undo_entry)
  |
  +-- push_undo_snapshot()  -->  entry.json = to_json()
  +-- push_undo_delta()     -->  entry.delta = Sketch_op_delta
  +-- pop_undo_snapshot()   -->  drop last entry (failed/aborted edit)

Sketch edits
  Sketch_op_recorder (RAII)
    note_prev_* / note_curr_* during mutation
    commit() --> push_undo_delta(Sketch_op_delta)
    cancel() --> discard (destructor auto-cancels if not committed)

Delta (abstract)
  apply_forward(view)   redo
  apply_reverse(view)   undo
  clone()               copy for opposite stack
```

`Sketch_op_delta` is currently the only `Delta` subclass (defined in [`sketch_op_recorder.cpp`](../sketch_op_recorder.cpp), not the public header).

## Core types

### `Delta` ([`delta.h`](../delta.h))

```cpp
class Delta {
  virtual void apply_forward(Occt_view& view) = 0;
  virtual void apply_reverse(Occt_view& view) = 0;
  virtual std::unique_ptr<Delta> clone() const = 0;
};
```

Subclasses record the **forward** change. Undo applies `apply_reverse`; redo reapplies `apply_forward`.

### `Sketch_op_recorder` ([`sketch_op_recorder.h`](../sketch_op_recorder.h))

RAII scope around one sketch edit:

1. **Construction** — captures live node positions and linear edges at operation start; initializes `Sketch_op_data` for the sketch's `get_id()`.
2. **Mutation** — sketch code calls `note_*` helpers as it changes geometry.
3. **`commit()`** — if any notes were recorded, pushes the delta via `Occt_view::push_undo_delta()`.
4. **`cancel()`** or scope exit without commit — nothing is pushed (used for no-op mirror, failed tools, etc.).

Recorder notes (deduplicated by geometry):

| Method | Meaning |
| --- | --- |
| `note_prev_linear_edge` | Edge removed or split away (must have existed at op start) |
| `note_curr_linear_edge` | New linear segment added |
| `note_prev_arc_edge` / `note_curr_arc_edge` | Arc circle segment removed / added |
| `note_curr_node` | New node not live at op start (stores `permanent` for redo) |
| `note_prev_length_dim` / `note_curr_length_dim` | Dimension removed / added |
| `note_prev_operation_axis` / `note_curr_operation_axis` | Operation axis before / after |

`note_prev_linear_edge` only records edges that were present when the recorder opened (`linear_edge_at_op_start_`), so incidental splits of pre-existing geometry are not mistaken for undoable removals of the user's new work.

### `Sketch_op_delta` apply logic ([`sketch_op_recorder.cpp`](../sketch_op_recorder.cpp))

**Forward (redo):** add `curr_*` linear edges and arcs, restore `curr_*` dimensions and operation axis, refresh faces.

**Reverse (undo):** clear curr operation axis; remove curr dimensions, arcs, and linear segments; restore `prev_*` linear edges and arcs; restore prev operation axis and dimensions; tombstone `curr_node_pts`; refresh faces.

Geometry is matched by **2D coordinates** (`Precision::SquareConfusion()`), not raw node indices, so deltas survive sketch list reordering as long as the sketch id resolves.

`resolve_sketch_()` finds the target sketch in `Occt_view::get_sketches()` by id, with a fallback raw pointer for headless tests.

## Undo / redo navigation

`Occt_view::undo()` (simplified):

1. Set `m_restoring = true`.
2. Pop the undo entry; build a redo entry from the **current** state (clone delta or `to_json()`).
3. If the popped entry has a delta, `apply_reverse`; else `load(json, false)`.
4. Push redo entry; restore `mode`.
5. Clear `m_restoring`.

`redo()` is symmetric: pop redo, capture current state for undo, `apply_forward` or `load`, push undo.

## What pushes history

### Sketch deltas (`Sketch_op_recorder` + `commit()`)

| Area | Trigger |
| --- | --- |
| `Sketch_tools::finalize` | Line, multi-edge, rectangle, square, circle, slot, operation axis |
| `Sketch_dims` | Add/remove length dimension; dimension tool finalize |
| `Sketch_tools` / `Sketch_dims` | Add-node tool (split at interior node) |
| `sketch_operations.cpp` | Mirror selected edges |
| `Sketch_edges` / `Sketch_topo` | Splits during edge add (via recorder passed into add/split paths) |

Interactive tools create the recorder internally. Direct `add_edge_(..., rec)` calls are for tests and delta replay.

### JSON snapshots (`push_undo_snapshot()`)

| Area | Operation |
| --- | --- |
| `Occt_view` | New file, add/remove sketch, sketch from face, sketch extrude finalize, revolve, delete shapes/sketches, primitives (box, sphere, ...), STEP/PLY import |
| `shp_*` | Fuse, cut, common, fillet, chamfer, move, rotate, scale, polar duplicate, extrude finalize |
| `GUI` | Open project, underlay import/remove/calibration/transform sliders, underlay orthogonalize |
| `GUI` | Underlay panel edits push on `IsItemActivated` so one snapshot covers a drag |

### `pop_undo_snapshot()` (abort without restoring)

Used when a snapshot was pushed optimistically but the operation failed or made no change:

- Invalid project open (`GUI::on_file`)
- Underlay calibration rescale failure
- Other guarded UI paths in `gui.cpp`

## User interface

| Input | Action |
| --- | --- |
| Ctrl+Z | Undo |
| Ctrl+Shift+Z or Ctrl+Y | Redo |
| Edit menu | Undo / Redo (disabled when stack empty) |
| Settings pane | Shows stack sizes: `Undo: N | Redo: M [max 50]` |

Hotkeys are handled in `GUI::on_key` (`gui_mode.cpp`) when dist/angle edit popups are not active.

## Document load and new file

- **Open `.ezy`:** `GUI::on_file` pushes a snapshot of the current document, then `load(manifest)`. A failed parse calls `pop_undo_snapshot()`.
- **`Occt_view::load`:** clears shapes and sketches from JSON; clears undo/redo stacks unless `m_restoring`.
- **`Occt_view::new_file`:** pushes snapshot, then clears document and creates the default sketch.

## Adding undo for new behavior

### Sketch geometry change

1. Open a `Sketch_op_recorder` around the mutation.
2. Call `note_prev_*` before removing or splitting existing geometry; `note_curr_*` after adding.
3. `commit()` at success; `cancel()` on no-op or error.
4. If redo/undo needs logic not covered by existing note types, extend `Sketch_op_data` and both `apply_forward_` / `apply_reverse_` in `sketch_op_recorder.cpp`.

### Non-sketch document change

Call `Occt_view::push_undo_snapshot()` **before** mutating sketches, shapes, or assets. Use `pop_undo_snapshot()` if the mutation does not commit.

Prefer pushing only when work will actually happen (see `create_sketch_from_planar_face_` guard).

### New delta subclass

1. Subclass `Delta` in a dedicated header/source pair.
2. Implement `apply_forward`, `apply_reverse`, and `clone`.
3. Push via `push_undo_delta(std::make_unique<Your_delta>(...))`.
4. Document the new type here and in the owning module doc.

Today only sketch edits justify a delta; shape booleans and transforms remain JSON snapshots.

## Related docs

- [sketch.md](sketch.md) — sketch coordinator, stable ids, recorder usage in tools
- [shape.md](shape.md) — shape operations that push JSON snapshots
- [gui.md](gui.md) — input routing, mode switching restored on undo
