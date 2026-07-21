---
status: implemented-v0
topic: cross-section-tool
depends_on: shp-origin-orientation
blocks:
  - sketch-from-shape
---

# Cross-section tool (prototype)

**Load only when** the prompt is about a shape cross-section / section cut / cutting plane preview (not yet “sketch from shape”). Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

## Why before sketch-from-shape

Full “section → editable sketch edges” is high risk (line/arc-only model, node welding, OCCT section quirks). A **cross-section tool** lets us learn:

- Does `BRepAlgoAPI_Section` give usable curves on our solids (desktop + wasm)?
- How should the cutting plane be chosen once shapes have a frame?
- What should the user see (preview wires only vs temporary AIS vs commit)?

Depends on [shp-origin-orientation.md](shp-origin-orientation.md) so the plane can be **shape-local** (e.g. shape XY through origin + offset), not only world axes + bbox center.

## Goal (prototype)

Interactive or one-shot **section of selected solid(s)** by a plane:

1. Plane from shape frame (local XY/XZ/YZ + offset) and/or world axes while experimenting.
2. Compute section curves; **display** as temporary geometry in the viewer.
3. Optional later: export / copy / “promote to sketch” — **not required** for the first prototype.

Fail closed on empty section (status message). No requirement to create a `Sketch` yet.

## Experiment checklist

- [x] Box mid-plane -> four line edges (focused unit test).
- [x] Cylinder mid-plane -> circular edge (focused unit test).
- [x] Offset along normal; miss solid -> clear failure (focused unit test).
- [x] Multi-face / boolean solids: junk edges? duplicates? (fused two-box mid-plane returns **8** line edges, not a clean 4-edge rectangle — sketch import will need weld/dedup).
- [x] Curve types returned (`GeomAbs_*`) -> preview status counts lines, circles, ellipses, B-splines, and other curves.
- [x] WASM vs desktop parity ([occt-wasm-dual-version](../conventions/occt-wasm-dual-version.md)).

## Likely touch points

- New op near `shp_*` or thin helper using `BRepAlgoAPI_Section`
- Mode / Options for plane + offset
- Temp AIS for result wires (no sketch commit in v0)
- Log/inspect curve types for discovery notes (update this plan or a short “findings” section)

## Success

Enough evidence to either:

- green-light [sketch-from-shape-section.md](sketch-from-shape-section.md) with concrete curve→edge rules, or
- narrow scope (e.g. planar coplanar edges only, or preview-only forever).

## Out of scope (v0)

Import into `Sketch_edges`; undoable new sketch; script binding (unless useful for tests).

**Related UX:** [sketch-mode-shape-faint.md](sketch-mode-shape-faint.md) — keep the solid visible but faint under section preview.

## Implemented v0 findings

- `Shp` now has a persisted `gp_Ax3` local frame. It defaults to a world-aligned frame at the shape bounding-box center and follows baked move/rotate/scale transforms.
- `Shp_section` computes one shared cutting plane for the selection with `BRepAlgoAPI_Section` (`Approximation(false)` for exact curve types): orientation from the first selected solid's local axes, origin at the selection bbox center, plus offset. Section edges aggregate into a temporary topmost cyan AIS wire preview, with one translucent yellow plane outline and positive-normal arrow. None of this temporary geometry enters the document or undo history.
- Preview updates fail closed only when the shared plane misses every selected solid, or a hard OCCT failure occurs. Solids the plane does not intersect are skipped and counted in the status message; the yellow plane annotation remains.
- The first curve evidence supports line and circle handling. Ellipse/B-spline/other counts are exposed for further manual experiments before sketch import rules are chosen.
- Boolean solids are not yet sketch-ready: a fused two-box mid-plane yields 8 line edges (likely unmerged/overlapping segments) instead of one 4-edge outer loop.
- Desktop Release build and focused section tests pass with OCCT 8.0.0. The Emscripten Release build also passes with OCCT 7.9.3.
