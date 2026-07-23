# Shape module

Developer reference for 3D solid modeling in EzyCad. The public C++ entry point is [`shp.h`](../shp.h).

Maintainers: update this file when shape API, operation patterns, or invariants change (see [agents/conventions/token-lean.md](../../agents/conventions/token-lean.md#developer-docs-in-srcdoc)). User-facing 3D workflows live in [`docs/usage.md`](../../docs/usage.md) (Shape List, extrude, booleans, transforms).

## Purpose

`Shp` wraps a `TopoDS_Shape` as an OCCT `AIS_Shape` in the viewer. The `shp_*` translation units implement **3D modeling operations**: primitive creation, sketch-face extrusion, booleans, fillet/chamfer, move/rotate/scale, polar duplicate, and shape inspection metadata.

Unlike the sketch subsystem (2D coordinator + sub-modules), shape code is organized as:

- One core type (`Shp`) in `shp.h` / `shp.cpp`
- Shared operation helpers in `Shp_operation_base`
- One class or namespace per operation file (`Shp_fuse`, `shp_create`, `shp_info`, ...)

`Occt_view` owns the shape list (`m_shps`), constructs all operation objects, routes UI input, and handles JSON save/load for shapes.

Typical uses:

- Display and select 3D solids in the interactive context.
- Create primitives (box, sphere, cylinder, ...) and add them to the document.
- Extrude a sketch face into a solid; revolve sketch geometry (revolve lives in `skt_operations.cpp`, returns `Shp_rslt`).
- Boolean fuse/cut/common on selected shapes.
- Interactive move, rotate, and scale with preview transforms.
- Preview cross-sections on a shape-local XY, XZ, or YZ plane (optional hide of the back half).
- Fillet/chamfer by shape, face, wire, or edge pick mode.
- Polar duplicate selected shapes about an arm on the current sketch plane.

## Requirements and invariants

### Lifetime and ownership

- Shapes are stored in `Occt_view::m_shps` (`std::list<Shp_ptr>`). Access via `get_shapes()` or internal `add_shp_()`.
- Each solid stores a `gp_Ax3` local frame. New geometry defaults to a world-aligned frame at its bounding-box center. Baked move/rotate/scale transforms update the frame; project JSON and shape undo records preserve it.
- `Shp_ptr` is `opencascade::handle<Shp>`. New shapes are allocated with `new Shp(ctx(), topo_shape)` then registered through `Occt_view::add_shp_()`. Groups use `Shp::create_group` (empty compound, never displayed).
- Hierarchy: `parent_id` (0 = root) + `sibling_order`. Organizational groups only (no transform inheritance). Helpers: `shape_children`, `shape_descendant_solids`, `group_shapes`, `ungroup_shape`, `reparent_shape`, `would_reparent_create_cycle`.
- **Current group** (`Occt_view::current_group_id`, 0 = root): Shape List click sets it; empty groups are valid. Primitives / extrude / revolve / PLY / unioned STEP import call `add_shp_(..., use_current_group=true)` so new solids land under that group. Booleans keep `assign_result_parent_`.
- `Shp_operation_base` is a **`friend` of `Occt_view`** so operations can call `add_shp_()`, read selection, and use pick helpers without exposing those on the public view API.
- **`Occt_view&` must outlive** all `Shp_*` operation objects (they are member subobjects of the view).
- Boolean/polar results call `assign_result_parent_` so the new solid shares the inputs' parent when all match; otherwise root (`parent_id` 0).

### Adding a shape to the document

Always go through `Occt_view::add_shp_(Shp_ptr&)` (or a wrapper that calls it):

| Step | Action                                                              |
| ---- | ------------------------------------------------------------------- |
| 1    | Apply the view default material and shading refresh                 |
| 2    | Set the document-wide shape selection mode (`m_shp_selection_mode`) |
| 3    | Redisplay and append to `m_shps`                                    |

`add_shp_()` does **not** push undo; callers that mutate the document should push a typed shape delta (or snapshot fallback) after a successful commit. New shapes receive a stable `Shape_id` from `allocate_shape_id()` when id is still 0.

### Operation selection cache (`m_shps` on `Shp_operation_base`)

Each operation object inherits `Shp_operation_base` and uses **`m_shps`** as a lazy cache of shapes involved in the current operation:

| Helper                           | Requirement                                     |
| -------------------------------- | ----------------------------------------------- |
| `ensure_operation_shps_()`       | One or more selected `Shp` objects              |
| `ensure_operation_multi_shps_()` | Two or more selected shapes (fuse, cut, common) |

On first call, `m_shps` is filled from `get_selected_shps_()` (viewer selection filtered to `Shp`). Cleared on `reset()` / cancel paths in interactive tools.

Do not confuse this vector with `Occt_view::m_shps` (the document list).

### Transform preview vs bake

| Phase        | Move / rotate / scale                                                     | Booleans / fillet / chamfer                         |
| ------------ | ------------------------------------------------------------------------- | --------------------------------------------------- |
| **Preview**  | `SetLocalTransformation()`; `redisplay_operation_shps_after_transform_()` | N/A (immediate replace)                             |
| **Finalize** | `operation_shps_finalize_()` -> `bake_transform_into_geometry()`          | New `Shp`; `delete_operation_shps_()`; `add_shp_()` |
| **Cancel**   | `operation_shps_cancel_()` -> `ResetTransformation()`                     | N/A                                                 |

### Sketch-linked geometry

| Operation       | Module                     | Sketch tie-in                                        |
| --------------- | -------------------------- | ---------------------------------------------------- |
| Face extrude    | `Shp_extrude`              | Picks `Sketch_face_shp`; uses owning sketch plane    |
| Polar duplicate | `Shp_polar_dup`            | Rotation axis and arm on `curr_sketch().get_plane()` |
| Revolve         | `Sketch::revolve_selected` | Returns `Shp_rslt`; view calls `add_shp_()`          |

### Result type

`Shp_rslt` is `Result<Shp_ptr>` (`shp.h`). Used when an operation may fail before producing a shape (e.g. revolve).

## Architecture

```
Occt_view
  |
  +-- std::list<Shp_ptr> m_shps          document shapes
  +-- Shp_move / Shp_rotate / Shp_scale  interactive transforms
  +-- Shp_cross_section                       temporary local-plane section preview
  +-- Shp_extrude                        sketch face extrude session
  +-- Shp_fuse / Shp_cut / Shp_common    booleans
  +-- Shp_fillet / Shp_chamfer           edge modifiers (replace in place)
  +-- Shp_polar_dup                      polar array duplicate

shp.h / shp.cpp                          Shp AIS wrapper (name, visibility, display mode)
shp_operation.h / shp_operation.cpp      Shp_operation_base shared helpers
shp_create.*                             stateless primitive TopoDS builders (namespace shp_create)
shp_info.*                               shape info dialog lines (namespace shp_info)
```

There is no single `Shape` coordinator class; **`Occt_view` is the hub** and exposes accessors such as `shp_move()`, `shp_fuse()`, `add_box()`, etc.

## Core type: `Shp`

```cpp
class Shp : public AIS_Shape {
  Shp(AIS_InteractiveContext& ctx, const TopoDS_Shape& shp);
  static Shp_ptr create_group(...); // organizational node; not displayed
  // name, display mode, visibility preference, parent_id, sibling_order, is_group
  void apply_context_shown(bool); // Erase/Display without changing get_visible()
};
```

`set_visible` stores the user preference. `Occt_view::sync_sketch_shape_faint_style` applies effective visibility (own flag, ancestor groups, Hide all overlay, sketch faint/hide) via `apply_context_shown` so Hide all does not stomp per-shape flags. `update_display_()` re-binds selection after mode changes.

`.ezy` `shapes[]` entries include `id`, `name`, `parentId`, `order`, `visible`, and either `isGroup: true` or `material` + `geom` + `frame`. Undo uses `Shape_rec` (including the local frame) plus `Shape_tree_delta` for reparent/group/ungroup.

## `Shp_operation_base`

Protected helpers used by all operation classes:

| Method                                                        | Purpose                                               |
| ------------------------------------------------------------- | ----------------------------------------------------- |
| `get_selected_shps_()`                                        | Current viewer selection as `Shp_ptr` vector          |
| `ensure_operation_shps_()` / `ensure_operation_multi_shps_()` | Lazy-fill `m_shps` from selection                     |
| `delete_operation_shps_()`                                    | Remove operation shapes from viewer and document list |
| `operation_shps_finalize_()`                                  | Bake local transforms into geometry                   |
| `operation_shps_cancel_()`                                    | Reset local transforms                                |
| `get_shape_` / `get_face_` / `get_wire_` / `get_edge_`        | Pick sub-shapes at screen coords                      |
| `add_shp_(Shp_ptr&)`                                          | Register new shape in the document                    |
| `copy_shape_material_from_(dest, src)`                        | Preserve material after replace-style ops             |

## Operation modules

| File              | Type                   | Behavior                                                                                                                                                                     |
| ----------------- | ---------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `shp_create.h`    | `namespace shp_create` | Pure functions: `create_box`, `create_pyramid`, `create_sphere`, `create_cylinder`, `create_cone`, `create_torus` -> `TopoDS_Shape`. Called from `Occt_view::add_*` helpers. |
| `shp_extrude.h`   | `Shp_extrude`          | Two-click extrude of `Sketch_face_shp`; live preview solid + tmp length dimension; `finalize` / `cancel`; optional both-sides extrude.                                       |
| `shp_fuse.h`      | `Shp_fuse`             | `selected_fuse()` -- sequential `BRepAlgoAPI_Fuse` on all selected shapes -> one new `Shp`.                                                                                  |
| `shp_cut.h`       | `Shp_cut`              | `selected_cut()` -- first selected = blank, rest = tools (`BRepAlgoAPI_Cut`).                                                                                                |
| `shp_common.h`    | `Shp_common`           | `selected_common()` -- sequential `BRepAlgoAPI_Common` (intersection).                                                                                                       |
| `shp_move.h`      | `Shp_move`             | Drag on view plane; axis constraints (`Move_options`); Tab distance entry; finalize bakes translation.                                                                       |
| `shp_rotate.h`    | `Shp_rotate`           | Rotate about view axis, global X/Y/Z, or view-to-object; angle Tab entry; optional axis/center AIS guides.                                                                   |
| `shp_scale.h`     | `Shp_scale`            | Uniform scale from bbox center vs mouse distance; clamped factor 0.01..100.                                                                                                  |
| `shp_fillet.h`    | `Shp_fillet`           | `add_fillet(..., Fillet_mode)` -- `BRepFilletAPI_MakeFillet`; modes: Shape, Face, Wire, Edge (`mode.h`).                                                                     |
| `shp_chamfer.h`   | `Shp_chamfer`          | `add_chamfer(..., Chamfer_mode)` -- diagonal distance converted to setback (`dist/sqrt(2)`).                                                                                 |
| `shp_polar_dup.h` | `Shp_polar_dup`        | Arm on sketch plane; `dup()` copies selection at polar steps; options: rotate copies, combine into one solid.                                                                |
| `shp_cross_section.h`   | `Shp_cross_section`          | Shared `BRepAlgoAPI_Section` preview for the selection (first-shape local XY/XZ/YZ orientation, selection-bbox center + offset); temporary cyan section wires plus one translucent yellow plane/normal annotation; optional per-shape `Graphic3d_ClipPlane` to hide the negative-normal half. |
| `shp_info.h`      | `namespace shp_info`   | `collect(TopoDS_Shape, Display_meta*)` -> labeled lines for Shape info dialog.                                                                                               |

## Input routing (from UI / `Occt_view`)

`GUI` and `Occt_view` dispatch by `Mode` and toolbar actions:

| Mode / action                 | Mouse move (`GUI::on_mouse_pos`) | Left click                                                                   | Tab / Enter                                    | Esc (`Occt_view::cancel`)              |
| ----------------------------- | -------------------------------- | ---------------------------------------------------------------------------- | ---------------------------------------------- | -------------------------------------- |
| `Mode::Move`                  | `shp_move().move_selected`       | `shp_move().finalize`                                                        | `shp_move().show_dist_edit` (`gui_mode`)       | `shp_move().cancel` -> `Normal`        |
| `Mode::Rotate`                | `shp_rotate().rotate_selected`   | `shp_rotate().finalize`                                                      | `shp_rotate().show_angle_edit`                 | `shp_rotate().cancel` -> `Normal`      |
| `Mode::Scale`                 | `shp_scale().scale_selected`     | `shp_scale().finalize`                                                       | --                                             | `shp_scale().cancel` -> `Normal`       |
| `Mode::Sketch_face_extrude`   | `sketch_face_extrude(..., true)` | 1st click: pick face; 2nd click (view): `finalize_sketch_extrude_` if active | `dimension_input` / `on_enter` refresh preview | `m_shp_extrude.cancel`                 |
| `Mode::Shape_fillet`          | --                               | `shp_fillet().add_fillet(..., Fillet_mode)`                                  | --                                             | --                                     |
| `Mode::Shape_chamfer`         | --                               | `shp_chamfer().add_chamfer(..., Chamfer_mode)`                               | --                                             | --                                     |
| `Mode::Shape_polar_duplicate` | `shp_polar_dup().move_point`     | `shp_polar_dup().add_point`                                                  | --                                             | `shp_polar_dup().reset` on mode change |
| `Mode::Shape_cross_section`   | --                               | --                                                                           | Auto-preview on enter / selection / Options plane, offset, hide-back | Preview and clips cleared              |
| Fuse / cut / common (toolbar) | --                               | `selected_fuse` / `selected_cut` / `selected_common` (one-shot)              | --                                             | --                                     |
| Primitives (menu / script)    | --                               | `Occt_view::add_box`, `add_sphere`, ...                                      | --                                             | --                                     |
| Revolve (sketch Options)      | --                               | `Occt_view::revolve_selected` -> `add_shp_`                                  | --                                             | --                                     |
| Polar duplicate commit        | --                               | Options **Dup** button -> `shp_polar_dup().dup()`                            | --                                             | --                                     |

On mode change, the view cancels in-progress move, rotate, scale, and sketch extrude sessions.

Full GLFW -> `GUI` routing before these delegates: [`src/doc/gui.md`](gui.md).

## Undo

Shape ops use typed deltas from [`shp_delta.h`](../shp_delta.h) (see [undo-redo.md](undo-redo.md)). No full-document JSON for these paths.

| Mechanism                         | When                                                          |
| --------------------------------- | ------------------------------------------------------------- |
| `Shape_add_delta`                 | Primitives, extrude, revolve, STEP/PLY import                 |
| `Shape_remove_delta`              | Delete selection when only `Shp` objects are selected         |
| `Shape_geom_delta`                | Move / rotate / scale **finalize** (not preview)              |
| `Shape_replace_delta`             | Fuse / cut / common / fillet / chamfer / polar duplicate      |
| `push_undo_snapshot()` (fallback) | Delete when selection also includes sketch edges / dimensions |

Shapes carry a stable `Shape_id` persisted as `shapes[].id` in project JSON.

## Persistence

Shape serialization is handled in **`Occt_view`** (JSON `shapes` array in project save/load), not in a dedicated `shp_json` module. Fields include stable `id`, name, material, and BREP payload.

Import/export (STEP, IGES, STL, PLY) also flows through `Occt_view` reader/writer helpers.

## Typical developer usage

### Create and register a primitive

```cpp
Occt_view& view = ...;
// Prefer view.add_box(...) which registers the shape and pushes Shape_add_delta.
view.add_box(0, 0, 0, 10, 10, 10);
```

### Implement a replace-style operation

```cpp
Status My_op::run() {
  CHK_RET(ensure_operation_multi_shps_());
  std::vector<Shape_rec> removed;
  for (const Shp_ptr& s : m_shps)
    removed.push_back(capture_shape_rec(*s));
  // ... BRepAlgoAPI_* ...
  Shp_ptr result = new Shp(ctx(), new_topo);
  result->set_name("MyOp");
  delete_operation_shps_();
  add_shp_(result);
  view().push_undo_delta(std::make_unique<Shape_replace_delta>(
      std::move(removed), std::vector<Shape_rec>{capture_shape_rec(*result)}));
  return Status::ok();
}
```

### Shape info lines

```cpp
shp_info::Display_meta meta{shp->get_name(), "...", "...", shp->get_visible()};
auto lines = shp_info::collect(shp->Shape(), &meta);
```

## Testing

| Item         | Notes                                                                                                                        |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------- |
| GTest suite  | `tests/shp_tests.cpp` — filters `Shp_create.*`, `Shp_info.*`, `Shp_test.*`                                                   |
| Coverage     | `shp_create` volumes/bboxes; `shp_info::collect`; `Occt_view::add_*` / unique names; fuse/cut/common; shape undo/redo deltas |
| Fixture      | `Shp_test` inherits `Sketch_test` (headless `Occt_view`)                                                                     |
| Related      | Sketch-face extrude / revolve still live under `Sketch_test.*`                                                               |
| Build target | `EzyCad_tests` ([`agents/workflows/local-dev.md`](../../agents/workflows/local-dev.md))                                      |

## Related code outside `src/shp_*`

| Location                            | Role                                                                             |
| ----------------------------------- | -------------------------------------------------------------------------------- |
| `occt_view.h` / `gui_occt_view.cpp` | Shape list, `add_shp_`, primitives, I/O, operation member objects                |
| `skt_ais.h`                         | `Sketch_face_shp` extrusion source                                               |
| `skt_operations.cpp`                | `Sketch::revolve_selected` -> `Shp_rslt`                                         |
| `mode.h`                            | `Fillet_mode`, `Chamfer_mode`, tool modes                                        |
| `gui.h` / `gui.cpp`                 | Toolbar, Shape List, fillet/chamfer mode, material UI                            |
| `utl_geom.h`                        | Plane projection, bbox center, rotation helpers used by transforms and polar dup |
