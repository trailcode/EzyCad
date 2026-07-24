---
status: partial
topic: sketch-from-shape
depends_on:
  - shp-origin-orientation
  - cross-section-tool
---

# Sketch from shape (section curves)

**Load only when** the prompt is about creating a sketch from a shape / section → sketch. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

## Status

**Partial.** Cross-section Options **Cross section sketch** imports cached section **line** and **circle** edges into a new sketch (`Occt_view::create_sketch_from_cross_section`). Remaining work: weld/dedup for boolean sections, ellipse/B-spline densify, script binding.

## Goal (end state)

New sketch whose plane cuts selected `Shp` solid(s). Intersection curves become **committed editable** sketch edges (not snap-only overlay).

**Related (different):** planar-face sketch (`create_sketch_from_planar_face_`) only stores `m_originating_face` for snap — see [src/doc/sketch.md](../../src/doc/sketch.md). User docs must say **editable edges**.

## Working assumptions (revise after prototype)

| Topic               | Assumption                                                                 |
| ------------------- | -------------------------------------------------------------------------- |
| Curves              | `BRepAlgoAPI_Section` (confirm via cross-section tool)                     |
| Plane               | Prefer **shape-local** axes from shape frame + offset; world axes fallback |
| Result              | New sketch; import edges; `rebuild_faces()`; fail closed if empty          |
| Live link to source | None in v1                                                                 |

Earlier draft used world XY/XZ/YZ through bbox center — superseded once shape origin exists.

## Algorithm sketch (post-discovery)

1. Plane face on chosen `gp_Pln` covering solid AABB.
2. Section → explore edges → classify line / circle / polyline fallback.
3. Weld nodes → `rebuild_faces()` → one undo commit.

**Reuse:** `to_2d`/`to_3d`, `add_sketch`, `Sketch_edges`, `Sketch_op_recorder`.  
**Shipped MVP:** Options button on the cross-section tool; caches section plane with compound.

## Phases (after prerequisites)

- **MVP:** One solid; plane from shape frame; line+circle; undo; tests. *(Delivered via cross-section tool button; multi-solid uses shared plane.)*
- **Later:** densify; script; face-pick plane; optional `originating_shape_id`; weld overlapping boolean edges.

## Acceptance (when un-deferred)

- [x] Editable sketch edges from section; undo works; fail closed on miss.
- [x] Docs + CHANGELOG; curve rules match cross-section findings (line + circle first).
- [ ] Weld/dedup for fused solids; densify unsupported curves.

## Out of scope until unblocked

Changing planar-face behavior.
