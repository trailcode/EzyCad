# Known Bugs and Issues

* All operational axes must be persisted.
* Create a hole in a solid, try and create a face on a curved face. No error is presented to the user. 
* For curved surfaces, snapping needs to be computed at runtime, annotate snapping on actual curve and apply results. [![Screenshot](./images/scr1.png)](./images/scr.png)
* Subdivision for shapes should be based on zoom or dist of object to camera. 
* Shp list should have a field for selected, and the field should reflect what is selected in the viewer.
* Extrude should be in projection mode.
* Create sketch from face needs a temporary face select mode. When mode is changed it should go back to the previous mode. Other tools like chamfer need to do this also.
* Add sketch node should have the ability to allow the user to specify placement distance from another node.
* Ability to show/hide sketch nodes
* Underlay: edge calibration can introduce shear (non-orthogonal bitmap axes). The transform panel currently assumes center + half extents + rotation (orthogonal). Need a full six-parameter affine in the UI so users can adjust position, scale, rotation, and shear without losing calibration.

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