# PR - `Trailcode/sketch_list_improvements`

## Title

Sketch list dimension controls and dimension arrow-size settings

## Summary

- Add expandable sketch inspector sections in Sketch List for dimensions, edges, and faces.
- Add per-dimension controls in Sketch List: visibility checkbox and numeric offset input (flyout distance).
- Persist per-dimension visibility/offset in sketch JSON with backward-compatible loading for legacy arrays.
- Add a new Settings -> Sketch control for dimension arrow size, persist it in GUI settings JSON, and apply it live.
- Apply global arrow-size setting to sketch dimensions and extrude preview dimensions.

## Files Changed

- `src/geom.cpp`
- `src/geom.h`
- `src/gui.cpp`
- `src/gui.h`
- `src/gui_settings.cpp`
- `src/occt_view.cpp`
- `src/occt_view.h`
- `src/shp_extrude.cpp`
- `src/sketch.cpp`
- `src/sketch.h`
- `src/sketch_json.cpp`

## Related

- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/sketch_list_improvements
- Issue draft: `issue.md`

## Test Plan

- [ ] Build `EzyCad_lib` in Release config.
- [ ] Open Sketch List and verify per-sketch expand/collapse works.
- [ ] Expand `Dimensions` and verify each row has visibility and offset controls.
- [ ] Set offset to non-zero and verify label flyout distance updates.
- [ ] Set offset to `0` and verify automatic flyout behavior is restored.
- [ ] Save and reopen project; verify per-dimension visibility/offset persist.
- [ ] Open Settings -> Sketch; adjust `Dimension arrow size` and verify existing dimensions update live.
- [ ] Verify extrude preview dimension arrow size follows the same setting.

## Notes

- Untracked workspace directory exists: `third_party/ImGuiColorTextEdit/`.
