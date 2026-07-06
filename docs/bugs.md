# Known Bugs and Issues

* ~~All operational axes must be persisted.~~
* Create a hole in a solid, try and create a face on a curved face. No error is presented to the user. 
* For curved surfaces, snapping needs to be computed at runtime, annotate snapping on actual curve and apply results. [![Screenshot](./images/scr1.png)](./images/scr.png)
* Subdivision for shapes should be based on zoom or dist of object to camera. 
* Shp list should have a field for selected, and the field should reflect what is selected in the viewer.
* Extrude should be in projection mode.
* Create sketch from face needs a temporary face select mode. When mode is changed it should go back to the previous mode. Other tools like chamfer need to do this also.
* Add sketch node should have the ability to allow the user to specify placement distance from another node.
* Ability to show/hide sketch nodes
* Underlay calibration ("Set X from edge..." / "Set Y from edge...") can (and frequently does) introduce shear because the picks are measured against the basis present at click time. Full affine support exists (6-DOF editor + raw distortion toggle + per-axis flips for corner mapping), and both axes now auto-apply "Make orthogonal (keep lengths + U)" on completion. The "Define underlay datum..." tool that forced a clean perpendicular V has been removed. Rough edges that remain:
  - The raw 6-DOF editor exposes low-level numbers (Base + U/V) rather than higher-level controls (e.g. a direct shear-angle slider).
  - In raw mode you sometimes need the "Reverse image U/V" flips to get source (0,0) at the exact parallelogram corner you expect (due to OCCT face parametrization on sheared quads created from a wire).
  - The two render paths (default "preserve" resampling vs. raw/direct) have deliberately different visual results for the same affine; this can be surprising until you understand the toggle.
  - Calibration always affects the *entire* source image; there is no built-in crop/region-of-interest inside the underlay itself.
  - The cyan frame always shows the precise current full-image parallelogram (never a simple rectangle once shear exists).

* For selection mode, not all of the current modes are useful.
* Scale mode broken.
* Add concept of dim or dimension, use with node, a measurement, but does not participate in generating faces.
* Vendor and commit `third_party/ImGuiColorTextEdit/` into this repository; it has not been maintained upstream for a long time and we should pin a known-good copy.
* ~~After an edge is split by its midpoint, the split edges do not have midpoints to snap to~~ - FIXED 
* Operation axis, after adding undo does not work correctly.
* Add rectangle two points, with angle constraint, preview does not honor it.
* Add rectangle with center point behaves like the two point version.
* Add circle from three points is broken.
* For slot tool, there is no need for the second edge to have a angle constraint.
* If the exact same arc circle or edge is added twice, unneeded edges are added. The mirror tool does this.
* Need a option to specify circle by diameter. 
* ~~Need a mirror option for edge. First specify center.~~
* ~~Need a option to not add edge midpoint nodes.~~
* Need a option to split a edge.
* ~~If the operational axis is defined. Do not render snap points and perminate nodes.~~
* Need a hotkey for face inspection mode.
* Dist and angle input should accept basic algebraic functions, e.g. /, +, etc
* Arc function needs to accept tab for radius. 
* Dist tab dialog should state if it is a radius or diameter for arc.*
* Dist tab dialog should state if it is a 1/2 dist or full, for edge tool.
* **Sketch: slot + circle — edges split / colored incorrectly at intersections.** Draw a slot (two horizontals + end-cap arcs), then add a circle that crosses both horizontals. Observed: one horizontal may split while the other does not; circle arcs may not split at line crossings; some edge segments show the wrong highlight color (e.g. white vs green). Caps can spuriously split the horizontals near the slot center before the circle is added. Likely causes investigated (fix attempted and reverted; still open):
  - `segment_arc_intersections_2d` (`utl_geom.cpp`) uses `GeomAPI_ExtremaCurveCurve` and returns `p_on_seg` while validating arc parameter from `p_on_arc`. Extrema can pair a segment foot with a nearby point on the *full* circle outside the trimmed semicircle, yielding false interior hits (e.g. on the slot centerline when adding the right cap).
  - `add_arc_circle_edges` (`sketch_edges.cpp`) may use inconsistent node indices for intersection/knot logic vs `GC_MakeArcOfCircle` (vector is `[start, pick, end]` from `Sketch::add_arc_circle_`; `pt_end` / knot end node must be `node_idxs[2]`, not `[1]`). A long-standing knot bug masked wrong argument order in circle finalize (`sketch_tools.cpp`: `add_arc_circle_(points[0], points[2], points[1])` passes bottom/top as `pt_b` where the API expects arc *end*, with pick on the opposite side).
  - `update_faces()` face walker can loop on parallel shared edges (e.g. slot horizontals); a `Walk_key` step cap was added in `sketch_topo.cpp` as a guard — may hide topology errors rather than fix turn logic.
  - Relevant tests (if re-added): `SlotHorizontalLinesUnsplitBeforeCircleArcs`, `ArcSplit_CircleCrossesSlotLines` in `tests/sketch_tests.cpp`. Use `slot_c` perpendicular to the slot centerline (e.g. `(15, 5)` for `slot_b = (15, 0)`), not `(0, 5)`, to match the slot tool.
* Wasm / OCCT 8.x GLES shading regression: With OCCT **8.0.0.p1** (`V8_0_0_p1`) and `TKOpenGles`, new solids and extrude previews often render wireframe-only or with partial face shading in the browser. Native desktop (OCCT 8 + `TKOpenGl`) is unaffected. Rebuilding against **7.9.3** (`scripts/build-occt-793-wasm.ps1`) restores correct shading. Workarounds in EzyCad (`SetMaterial` + `Redisplay`, `refresh_shape_shading_`) do not fix the root cause on OCCT 8. Use 7.9.3 for wasm builds and demo publishing until upstream fixes GLES presentation.
