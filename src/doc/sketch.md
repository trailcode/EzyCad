# Sketch module

Developer reference for the 2D sketch subsystem. The public C++ entry point is [`sketch.h`](../sketch.h).

Maintainers: update this file when sketch API, module boundaries, or invariants change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing sketch guides live in [`docs/usage-sketch.md`](../../docs/usage-sketch.md).

## Purpose

`Sketch` is a 2D planar profile embedded in EzyCad's 3D OCCT viewer. Each sketch lives on a fixed `gp_Pln`, owns nodes (2D vertices), edges (line segments and circle arcs), derived closed faces, length dimensions, and optional image underlay.

The class is a **coordinator**: it holds shared state (plane, viewer context, visibility, operation axis) and delegates geometry, topology, drawing tools, dimensions, and display to focused sub-modules introduced in the sketch refactor (issue #163).

Typical uses:

- Interactive creation and editing via sketch tools (line, arc, rectangle, slot, add-node, dimension, operation axis).
- Face extraction for extrusion and revolve into 3D solids.
- Mirror selected edges about an operation axis.
- JSON save/load and undo/redo through stable sketch and node identity.

## Requirements and invariants

### Lifetime and ownership

- A `Sketch` is owned by `Occt_view` (`Sketch_list m_sketches`, current sketch `m_cur_sketch`). Use `Sketch_ptr` (`std::shared_ptr<Sketch>`) when sharing.
- The constructor **`Occt_view& view` must outlive the sketch**. The sketch holds references to `m_view` and `m_ctx` (the OCCT interactive context).
- Each sketch receives a stable **`m_id`** from `Occt_view::allocate_sketch_id()`. Display names (`m_name`) may duplicate; undo deltas and file I/O key off `get_id()`.
- Node indices are **never compacted**. Deleted nodes become tombstones (`Node::deleted`) so undo/redo and JSON round-trips stay stable.

### Plane and coordinates

- All sketch geometry is stored as **2D plane coordinates** (`gp_Pnt2d`) in `Sketch_nodes`. Convert to 3D with `Sketch::to_3d_()` or node helpers that take `gp_Pln`.
- Screen input arrives as `ScreenCoords` and is projected onto `m_pln` inside `Sketch_nodes::snap()` and tool handlers.

### Transient vs committed state

- **Committed** geometry: `Sketch_edges` (persistent edge list), `Sketch_topo` (face cache), `Sketch_dims` (length dimensions), `Sketch_node_marks` (permanent "+" markers).
- **Transient** draw state: `Sketch_tools` (`m_tmp_edges`, `m_tmp_node_idxs`, rubber-band previews). Cleared by `cancel_elm()` or committed by `finalize_elm()`.
- Do not mix transient edges into the persistent list; tools finalize through `Sketch_edges` and `Sketch_topo`.

### Operation axis

- Mirror and revolve require an optional **`m_operation_axis`** edge (two nodes). While the axis is active in mirror/revolve mode, sketch snap and permanent node markers are suppressed (`operation_axis_suppresses_sketch_snap_()`).
- Clear with `clear_operation_axis()` before leaving operation-axis workflows.

### Global sketch options (static)

| API | Effect |
| --- | --- |
| `Sketch::set_add_mid_pt_edges(bool)` | When true, linear edge tools create midpoint snap nodes on new segments. Default off in the app; many unit tests enable it in `SetUp`. |
| `Sketch::set_edge_from_center(bool)` | When true, rectangle/circle/slot tools place from center instead of corner. |

Snap distance, guide mode, and guide colors live on `Sketch_nodes` (static setters).

## Architecture

```
Sketch  (coordinator: sketch.cpp, sketch.h)
  |
  +-- Sketch_nodes        vertices, snapping, outside-sketch snap points
  +-- Sketch_node_marks   permanent "+" markers for user-placed nodes
  +-- Sketch_edges        persistent edge list; add, split, remove, pick
  +-- Sketch_topo         planar graph -> closed faces, edge splitting
  +-- Sketch_dims         length dimensions, typed distance/angle input
  +-- Sketch_tools        interactive drawing session (tmp state)
  +-- Sketch_underlay     raster image on the sketch plane

Supporting (not owned sub-objects):
  sketch_edge.*           Sketch_edge type, linear/arc predicates
  sketch_ais.*            Sketch_AIS_edge, Sketch_AIS_node_mark, Sketch_face_shp
  sketch_display.cpp      visibility, edge styling, list hover, set_current
  sketch_operations.cpp   operation axis, mirror, revolve
  sketch_json.*           JSON serialization
  sketch_delta.*          undo/redo (Sketch_op_recorder, Sketch_delta)
```

`Sketch` grants `friend` access to `Sketch_json`, `Sketch_delta`, `Sketch_topo`, `Sketch_edges`, `Sketch_dims`, `Sketch_node_marks`, and `Sketch_tools` for coordinated mutations without exposing internals publicly.

## Public API overview

### Construction

```cpp
Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln);
Sketch(const std::string& name, Occt_view& view, const gp_Pln& pln, const TopoDS_Wire& outer_wire);
```

The wire overload creates a sketch **from a planar face**; `m_originating_face` is displayed and its vertices contribute to snap targets.

### Input routing (from UI / Occt_view)

The viewer forwards pointer and keyboard events to the **current** sketch:

| Method | When |
| --- | --- |
| `add_sketch_pt(screen_coords)` | Click / left-button down for the active sketch tool |
| `sketch_pt_move(screen_coords)` | Mouse move (rubber band, snap guides) |
| `dimension_input(screen_coords)` | Tab: precise length while drawing |
| `angle_input(screen_coords)` | Shift+Tab: angle constraint while drawing |
| `on_enter()` | Enter: confirm typed numeric input |
| `finalize_elm()` | Complete multi-step tool (e.g. right-click finish) |
| `cancel_elm()` | Esc: discard transient state; returns whether an operation was canceled |
| `on_mode()` | Called when `Mode` changes; resets transient state and refreshes axis/mark display |

`get_mode()` reads `Occt_view::get_mode()`; tool behavior in `Sketch_tools` branches on the active `Mode`.

### Visibility and display

| Method | Purpose |
| --- | --- |
| `set_visible` / `is_visible` | Show or hide the whole sketch in the viewer |
| `set_show_faces` / `set_show_edges` / `set_show_dims` | Layer toggles for faces, edges, dimensions |
| `set_edge_style(Full \| Background \| Hidden)` | Current vs background sketch appearance |
| `set_current()` | Make this sketch current in `Occt_view` |
| `refresh_annotations(Sketch_annotation_refresh)` | Rebuild length dimensions and/or permanent node marks after settings changes |
| `append_list_hover_ais(out)` | AIS objects to highlight when the Sketch List row is hovered |

### Geometry queries and inspector

| Method | Purpose |
| --- | --- |
| `has_edges`, `edge_count`, `face_count`, `length_dimension_count` | Counts for UI and mode selection after undo |
| `inspector_*_labels()` | Human-readable labels for Sketch List / inspector panels |
| `get_plane()`, `get_nodes()`, `underlay()` | Access plane, nodes, and image underlay |

### Operations

| Method | Purpose |
| --- | --- |
| `mirror_selected_edges()` | Mirror selected edges about `m_operation_axis` (requires axis + selection) |
| `revolve_selected(angle)` | Revolve selected edges/faces into a 3D solid |
| `clear_operation_axis` / `has_operation_axis` | Operation-axis lifecycle |
| `remove_edge`, `remove_permanent_node_mark` | Delete committed geometry from selection |
| `toggle_edge_dim_anno`, `try_remove_length_dimension` | Dimension tool and deletion |

### Dimensions (delegated to `Sketch_dims`)

Per-dimension visibility, flyout offset, name, and OCCT handle accessors are exposed on `Sketch` for the Sketch List and viewer highlight paths.

## Typical developer usage

### Create a sketch and add geometry (tests / scripts)

```cpp
Occt_view& view = ...;
gp_Pln pln = ...;
auto sketch = std::make_shared<Sketch>("Test", view, pln);
view.set_curr_sketch(sketch);

// Simulate two clicks for a line (screen coords are plane units in tests)
sketch->add_sketch_pt(ScreenCoords(dvec2(0.0, 0.0)));
sketch->add_sketch_pt(ScreenCoords(dvec2(5.0, 0.0)));
```

For closed profiles and face tests, add edges that intersect so `Sketch_topo::update_faces()` splits segments and builds faces. See `tests/sketch_tests.cpp`.

### Record undoable edits

Wrap mutations that should appear on the undo stack in `Sketch_op_recorder`:

```cpp
Sketch_op_recorder rec(view, *sketch);
sketch->add_edge_(pt_a, pt_b, rec);  // private; tools/json use this path
rec.commit();
```

Interactive tools create recorders internally on finalize. JSON load and delta apply/revert bypass live recorders.

### Serialize

```cpp
nlohmann::json j = Sketch_json::to_json(*sketch, assets);
auto loaded = Sketch_json::from_json(view, j);
```

`from_json` restores nodes by index, edges, dimensions, operation axis, and underlay metadata.

### Read edges without exposing the list type

```cpp
sketch->m_edges.for_each_linear([](const Sketch_edge_linear& e) { ... });
sketch->m_edges.for_each_arc([](const Sketch_edge_arc& e) { ... });
```

Prefer these visitors in JSON/delta/topo code over iterating `std::list<Sketch_edge>` directly.

## Module responsibilities (detail)

| File | Responsibility |
| --- | --- |
| `sketch.h` / `sketch.cpp` | Coordinator: input routing, edge shape updates, snap aggregation, inspector labels, static options |
| `sketch_edge.h` | `Sketch_edge` struct; `sketch_edge_is_linear` / `sketch_edge_is_arc` |
| `sketch_edges.h` | Persistent `std::list<Sketch_edge>`; add linear/arc edges; split at intersections; pick and selection |
| `sketch_topo.h` | Planar face extraction from edge graph; automatic splitting at interior nodes and arc crossings |
| `sketch_nodes.h` | Node storage, snap, snap guides, outside-sketch snap points |
| `sketch_node_marks.h` | AIS "+" markers for permanent nodes only |
| `sketch_dims.h` | Length dimensions between node pairs; Tab/Shift+Tab input; dimension-tool pick state |
| `sketch_tools.h` | Mode-specific click/move/finalize/cancel for all sketch creation tools |
| `sketch_underlay.h` | Calibrated raster underlay on the sketch plane |
| `sketch_ais.h` | OCCT AIS wrappers tied back to owning `Sketch` |
| `sketch_display.cpp` | Visibility, edge/face styling, `set_current`, list hover |
| `sketch_operations.cpp` | Operation axis, mirror, revolve |
| `sketch_json.h` | `.ezy` / project JSON for sketches |
| `sketch_delta.h` | Undo/redo deltas keyed by `sketch_id` |

## Edge and face model

- A **linear edge** has `node_idx_a`, `node_idx_b`, optional `node_idx_mid` (midpoint snap node when the option is on).
- An **arc edge** has start, end, and arc midpoint nodes (`node_idx_arc_pt` for the curve point used when snapping).
- **Faces** are derived, not stored as user entities. `Sketch_topo::update_faces()` rebuilds `Sketch_face_shp` objects after edge changes. Faces drive extrusion/revolve and face selection.
- New linear or arc edges that cross existing edges trigger **automatic splitting** so T-junctions and divided regions produce valid topology (see user doc on automatic splitting).

## Testing

- GTest suite: `tests/sketch_tests.cpp`, filter `Sketch_test.*`.
- Build target: `EzyCad_tests` (see [`agents/workflows/local-dev.md`](../../agents/workflows/local-dev.md)).
- Tests construct `Occt_view`, create `Sketch` on a plane, and drive geometry through `add_sketch_pt` or private helpers via test fixtures.

## Related code outside `src/sketch*`

- `occt_view.h` / `occt_view.cpp` -- sketch list, current sketch, input forwarding, sketch creation from planar faces.
- `mode.h` -- `Mode` enum selects the active sketch tool.
- `gui.cpp` -- toolbar and Sketch List UI wired to `Sketch` methods.
- `shp.h` -- `Shp_rslt` from `revolve_selected`.
