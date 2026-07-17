# Undo / redo

Developer reference for EzyCad's document history. Public C++ entry points:

- [`delta.h`](../delta.h) -- abstract undo step
- [`skt_op_recorder.h`](../skt_op_recorder.h) -- sketch operation recorder (public API)
- [`shp_delta.h`](../shp_delta.h) -- shape add / remove / geom / replace deltas
- [`doc_delta.h`](../doc_delta.h) -- sketch structural and underlay deltas
- [`gui_occt_view.h`](../gui_occt_view.h) -- stack storage and `undo()` / `redo()`

Maintainers: update this file when undo stack layout, delta types, or push/pop contracts change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing shortcuts are documented in [`docs/usage.md`](../../docs/usage.md).

## Purpose

EzyCad keeps a bounded history of user edits so **Ctrl+Z** / **Ctrl+Y** (and Edit menu items) can step backward and forward through the session. History is owned by `Occt_view` and spans both 2D sketch geometry and the wider 3D document (shapes, sketches, underlays, imports).

Interactive edits use **typed element deltas** that store only affected objects. Full-document JSON snapshots remain only for a few fallback cases (mixed selection delete, optimistic file-open checkpoint).

| Strategy               | When used                                       | Stored payload                           |
| ---------------------- | ----------------------------------------------- | ---------------------------------------- |
| **Element delta**      | Sketch, shape, underlay, sketch add/remove      | `std::unique_ptr<Delta>`                 |
| **Full JSON snapshot** | Mixed delete (sketch edges + shapes); file open | `to_json()` string of the whole document |

## Requirements and invariants

### Stack ownership

- `Occt_view` holds `m_undo_stack` and `m_redo_stack` (`std::vector<Undo_entry>`).
- Maximum depth: `k_max_undo` (50). Oldest entries are dropped from the front when exceeded.
- Any new user edit clears `m_redo_stack` (standard linear history).

### `Undo_entry`

Each stack frame stores:

| Field   | Role                                                        |
| ------- | ----------------------------------------------------------- |
| `delta` | Non-null for typed steps; mutually exclusive with `json`    |
| `json`  | Full-document snapshot when `delta` is null                 |
| `mode`  | `Mode` active when the operation ran; restored on undo/redo |

### `m_restoring` guard

While `Occt_view::undo()` or `redo()` runs, `m_restoring` is true. During that window:

- `push_undo_snapshot()`, `push_undo_delta()`, and `pop_undo_snapshot()` are no-ops.
- `load()` does **not** clear undo/redo stacks (the navigation itself uses `load(json, false)` for snapshot steps).

This prevents re-entrant history pushes while applying a step.

### Mode restoration

After applying a delta or reloading a snapshot, `Occt_view` calls `GUI::set_mode(state.mode)`. If the stored mode is `Sketch_inspection_mode`, the sketch list pane is shown again.

### View camera

Snapshot undo/redo calls `load(state.json, false)` so **camera pan/zoom/rotate are preserved** across JSON-based steps. Delta steps mutate data in place and do not touch the view matrix.

### Stable identities

- Sketches: stable `sketch_id` (see [sketch.md](sketch.md#requirements-and-invariants)).
- Shapes: stable `Shape_id` on `Shp`, allocated by `Occt_view::allocate_shape_id()`, persisted as `shapes[].id` in project JSON. Deltas resolve shapes by id, not list index.

Booleans and fillets are lossy: undo must retain the **pre-op BREP of affected shapes** (not the whole document).

## Architecture

```
GUI (menus, hotkeys, underlay UI)
  |
  +-- Occt_view::undo() / redo()
  |     m_undo_stack / m_redo_stack (Undo_entry)
  |
  +-- push_undo_delta()     -->  typed Delta subclass
  +-- push_undo_snapshot()  -->  entry.json = to_json() (fallback only)
  +-- pop_undo_snapshot()   -->  drop last entry (failed/aborted edit)

Sketch edits
  Sketch_op_recorder --> Sketch_op_delta

Shape edits
  Shape_add_delta / Shape_remove_delta / Shape_geom_delta / Shape_replace_delta

Document structure
  Sketch_struct_delta / Underlay_delta / Composite_delta

Delta (abstract)
  apply_forward(view)   redo
  apply_reverse(view)   undo
  clone()               copy for opposite stack
```

## Core types

### `Delta` ([`delta.h`](../delta.h))

```cpp
class Delta {
  virtual void apply_forward(Occt_view& view) = 0;
  virtual void apply_reverse(Occt_view& view) = 0;
  virtual std::unique_ptr<Delta> clone() const = 0;
};
```

### Sketch deltas

See [`Sketch_op_recorder`](../skt_op_recorder.h) / `Sketch_op_delta` (unchanged element prev/curr lists).

### Shape deltas ([`shp_delta.h`](../shp_delta.h))

| Type                  | Stores                                       | Used for                                       |
| --------------------- | -------------------------------------------- | ---------------------------------------------- |
| `Shape_add_delta`     | Added `Shape_rec` list (id, name, mat, geom) | Primitives, extrude, revolve, STEP/PLY import  |
| `Shape_remove_delta`  | Removed `Shape_rec` list                     | Delete selection (shapes only)                 |
| `Shape_geom_delta`    | Per-id before/after `TopoDS_Shape`           | Move / rotate / scale finalize                 |
| `Shape_replace_delta` | Removed + added `Shape_rec` lists            | Fuse / cut / common / fillet / chamfer / polar |

`Shape_rec` holds a shared `TopoDS_Shape` plus attrs (not BREP text). File I/O still serializes BREP in project JSON. Helpers: `capture_shape_rec`, `Occt_view::insert_shape_rec` / `remove_shape_by_id` / `set_shape_geom_by_id`.

### Document deltas ([`doc_delta.h`](../doc_delta.h))

| Type                  | Stores                                                 | Used for                            |
| --------------------- | ------------------------------------------------------ | ----------------------------------- |
| `Sketch_struct_delta` | One sketch JSON (add or remove); optional auto-default | Add/remove sketch, sketch-from-face |
| `Underlay_delta`      | Sketch id + before/after underlay JSON                 | Underlay import/remove/calib/UI     |
| `Composite_delta`     | Ordered child deltas                                   | Multi-part edits if needed          |

Underlay pixels stay in `Ezy_asset_store`; undo restores placement / asset id references only.

## Undo / redo navigation

`Occt_view::undo()` (simplified):

1. Set `m_restoring = true`.
2. Pop the undo entry; build a redo entry from the **current** state (clone delta or `to_json()`).
3. If the popped entry has a delta, `apply_reverse`; else `load(json, false)`.
4. Push redo entry; restore `mode`.
5. Clear `m_restoring`.

`redo()` is symmetric.

## What pushes history

### Sketch deltas (`Sketch_op_recorder` + `commit()`)

Same as before: tool finalize, dims, splits, mirror (see prior sections / [sketch.md](sketch.md)).

### Shape deltas

| Area        | Operation                                                                          |
| ----------- | ---------------------------------------------------------------------------------- |
| `Occt_view` | Primitives, revolve, STEP/PLY import, shape-only delete                            |
| `shp_*`     | Fuse, cut, common, fillet, chamfer, move/rotate/scale finalize, polar dup, extrude |

Push **after** a successful mutation (capture result ids/BREPs). Failed booleans/fillets do not push.

### Sketch / underlay deltas

| Area        | Operation                                                                            |
| ----------- | ------------------------------------------------------------------------------------ |
| `Occt_view` | `add_sketch`, `remove_sketch`, sketch-from-face                                      |
| `GUI`       | Underlay import/remove/calib/orthogonalize; transform sliders on activate/deactivate |

### JSON snapshots (`push_undo_snapshot()`)

| Area        | Operation                                                             |
| ----------- | --------------------------------------------------------------------- |
| `Occt_view` | Delete when selection includes non-shape AIS (sketch edges / dims)    |
| `GUI`       | Open project (optimistic push; cleared by `load` or `pop` on failure) |

### `pop_undo_snapshot()`

Used when a snapshot was pushed optimistically but the operation failed (invalid project open).

## User interface

| Input                  | Action                                  |
| ---------------------- | --------------------------------------- |
| Ctrl+Z                 | Undo                                    |
| Ctrl+Shift+Z or Ctrl+Y | Redo                                    |
| Edit menu              | Undo / Redo (disabled when stack empty) |
| Settings pane          | Shows stack sizes: Undo N / Redo M      |

Hotkeys are handled in `GUI::on_key` (`gui_mode.cpp`) when dist/angle edit popups are not active.

## Document load and new file

- **Open `.ezy`:** `GUI::on_file` pushes a snapshot of the current document, then `load(manifest)`. A failed parse calls `pop_undo_snapshot()`. Successful `load` clears both stacks.
- **`Occt_view::new_file`:** clears undo/redo stacks, resets sketch and shape id allocators, creates the default sketch, and calls `reset_default_view()` (top view framed to Settings default 2D view size).

## Adding undo for new behavior

### Sketch geometry change

1. Open a `Sketch_op_recorder` around the mutation.
2. Call `note_prev_*` / `note_curr_*` as appropriate.
3. `commit()` at success; `cancel()` on no-op or error.

### Shape change

1. Capture affected `Shape_rec` / `TopoDS_Shape` **before** mutating when needed for reverse.
2. Mutate; capture results.
3. `push_undo_delta(std::make_unique<Shape_*_delta>(...))` on success only.

### Sketch add/remove or underlay

Use `Sketch_struct_delta` or `Underlay_delta` (see GUI underlay helpers `begin_underlay_undo_` / `commit_underlay_undo_` / `push_underlay_change_`).

### New delta subclass

1. Subclass `Delta` in a dedicated header/source pair.
2. Implement `apply_forward`, `apply_reverse`, and `clone`.
3. Push via `push_undo_delta`.
4. Document the new type here and in the owning module doc.

Prefer typed deltas over `push_undo_snapshot()` for interactive edits.

## Related docs

- [sketch.md](sketch.md) -- sketch coordinator, stable ids, recorder usage
- [shape.md](shape.md) -- shape operations and shape-id undo
- [gui.md](gui.md) -- input routing, mode switching restored on undo
