---
status: planning
topic: shp-origin-orientation
depends_on: null
blocks:
  - cross-section-tool
  - sketch-from-shape
---

# Shape origin and orientation

**Load only when** the prompt is about shape origin, shape frame, shape axes, or annotating a shape’s position/orientation. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

## Why first

Sketches already have an editable origin ([`Sketch::origin_pt`](../../src/sketch.h)). Shapes (`Shp`) do **not**: after create/bake they are a `TopoDS_Shape` + name/id. Move/rotate/scale use **bbox center** as a temporary pivot (`get_shape_bbox_center`), not a persistent frame.

A stored **origin + orientation** (local `gp_Ax2` / `gp_Ax3`) is the prerequisite for:

1. Cross-section planes relative to the shape (not only world XY/XZ/YZ).
2. Later “sketch from shape” cutting planes.
3. Clearer transforms, polar dup, and inspect UI.

## Goal (prototype-friendly)

Let the user **see and set** a shape’s local frame:

- **Origin** (3D point, default: bbox center or create-time corner/center).
- **Orientation** (X/Y/Z axes; at least a right-handed triad).
- **Annotate** in the viewer (axis triad / marker; show when shape selected or via Shape List / properties).
- **Persist** in `.ezy` JSON with the shape; survive undo.

## Open design (experiment)

| Question          | Candidates                                                                                                                                  |
| ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------- |
| Representation    | `gp_Ax2` on `Shp`; or origin + quaternion; or `TopLoc_Location` separate from BRep location                                                 |
| Default on create | Primitive dialog origin; extrude → sketch-plane origin projected; boolean → first operand / bbox center                                     |
| Edit UX           | Properties fields; drag origin; align to face/edge; “reset to bbox center”                                                                  |
| Display           | Always-on triad vs selection-only; reuse sketch origin visual language where sensible                                                       |
| Bake vs frame     | Does move/rotate update the frame, the BRep, or both? Prefer: bake geometry as today; frame is **annotation** unless user “redefine origin” |

Default lean: **frame is document metadata on `Shp`**, not a second transform stack. Geometry bake stays as today; tools that need a pivot/plane **read the frame**.

## Likely touch points

- [`shp.h`](../../src/shp.h) / [`shp.cpp`](../../src/shp.cpp) — store frame
- Shape JSON / [`shp_delta`](../../src/shp_delta.h) — serialize + undo
- [`shp_info`](../../src/shp_info.cpp) — show origin/axes
- GUI: Shape List / properties / optional AIS triad
- [`src/doc/shape.md`](../../src/doc/shape.md) when API lands

## Acceptance (first slice)

- [ ] Selected shape shows origin + axis annotation.
- [ ] User can set origin (and at least rotate frame about one axis, or full triad).
- [ ] Round-trip `.ezy` + undo.
- [ ] Default frame on new primitives is defined and documented.

## Out of scope here

Cross-section tool; sketch-from-shape import; changing planar-face sketch behavior.

**Related UX:** [sketch-mode-shape-faint.md](sketch-mode-shape-faint.md) — dim solids while sketching so a frame triad stays readable.
