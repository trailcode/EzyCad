# PR - `Trailcode/ui_improvments2`

## Title

Shape polar duplicate: show permanent sketch nodes in inspection and polar-duplicate modes

## Summary

- Permanent "+" markers for user-placed sketch nodes (added via the Add Node tool) are now visible whenever a sketch is active in inspection mode or during Shape polar duplicate (which relies on sketch node snapping for the polar arm).
- Previously, these nodes were only shown while actively using the Add Node tool (`Sketch_add_node`); they were hidden in `Sketch_inspection_mode` and `Shape_polar_duplicate`.
- Changed the visibility condition in `sync_permanent_node_annos_()` from an explicit mode check to `is_sketch_mode(mode) || mode == Mode::Shape_polar_duplicate`.

## Files Changed

- `src/sketch.cpp`

## Related

- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/ui_improvments2
- Enhancement issue (future sketch-based polar reference): https://github.com/trailcode/EzyCad/issues/102

## Test Plan

- [ ] Build `EzyCad_lib` in Release config.
- [ ] Enter a sketch with existing permanent nodes (added via Add Node tool).
- [ ] Switch to Sketch inspection mode and verify the "+" markers are visible and pickable.
- [ ] Start Shape polar duplicate, select shapes, and verify sketch nodes remain visible for snapping the polar arm origin.
- [ ] Verify nodes hide correctly when leaving all sketch/polar modes (e.g., back to Normal).
- [ ] Confirm no regression in other sketch tools (line, circle, etc.).

## Notes

- Untracked workspace directory exists: `third_party/ImGuiColorTextEdit/`.
