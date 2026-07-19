# PR - Project units and New project defaults

## Title

Project units and New project defaults

## Summary

- Add **File -> Project units** (Inches / Millimeters) for on-the-fly display/input; model space stays inch-scaled.
- Persist `projectUnit` in `.ezy`; **Settings -> New project defaults** for New unit + 2D framing.
- Route length I/O through `get_display_to_model_scale()`; keep import/export on inch-based scale.

## Files Changed

- `src/utl_types.h`, `src/gui_occt_view.*`, `src/gui*.cpp`, `src/skt_*.cpp`, `src/shp_*.cpp`, `src/scr_*.cpp`
- `tests/skt_tests.cpp`
- `docs/usage*.md`, `docs/scripting.md`, `CHANGELOG.md`, `src/doc/gui.md`, `src/doc/utility.md`
- `res/ezycad_settings.json`
- `agents/drafts/issues/active/gh-212-project-units.md`
- `agents/drafts/prs/active/gh-213-project-units.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/212
- PR: https://github.com/trailcode/EzyCad/pull/213

## Test Plan

- [ ] Build `EzyCad` / `EzyCad_tests` (Debug)
- [ ] `EzyCad_tests.exe --gtest_filter=Sketch_test.ProjectUnit_displayConversionAndJsonRoundTrip`
- [ ] Switch File -> Project units; confirm dims remap, geometry unchanged
- [ ] Save/reload and old-file default to Inches
- [ ] New project defaults + File -> New
- [ ] Grid Settings unit suffixes; Export picker independent
- [ ] Docs / CHANGELOG spot-check
