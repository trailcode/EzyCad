# PR - Trailcode/revolve_fix

## Title

Operational axis UX, revolve solid conversion, shape info dialog, and related fixes

## Summary

- **Operational axis:** toolbar renamed to **Operational axis**; axis line visible only in that mode; after definition, permanent **+** markers and sketch snap suppressed for clearer mirror/revolve selection; axes persist in sketch JSON.
- **Revolve:** best-effort conversion of edge revolutions to solids; contextual **?** help on revolve angle field.
- **Shape info:** right-click shape name or **M** button opens topology/property dialog.
- **Fixes:** shape highlight in sketch operation-axis mode; related GUI/settings refactors on the branch.

Closes operational-axis visibility/snap/persistence work tracked in `agents/issues/013-operational-axis-visibility-and-snap.md`.

## Files Changed

- `src/sketch.cpp`, `src/sketch.h`, `src/sketch_nodes.cpp`, `src/sketch_json.cpp`
- `src/occt_view.cpp`, `src/occt_view.h`
- `src/gui.cpp`, `src/gui.h`, `src/gui_mode.cpp`, `src/gui_settings.cpp`
- `src/shp_info.cpp`, `src/shp_info.h`, `src/shp.cpp`, `src/utl_occt.cpp`, `src/utl_occt.h`
- `tests/sketch_tests.cpp`
- `CHANGELOG.md`, `docs/usage-sketch.md`, `docs/usage.md`, `docs/usage-settings.md`, `docs/bugs.md`
- `res/ezycad_settings.json`
- `agents/issues/013-operational-axis-visibility-and-snap.md`, `agents/prs/008-operational-axis-revolve-shape-info.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/142
- PR: https://github.com/trailcode/EzyCad/pull/143
- Draft: `agents/issues/013-operational-axis-visibility-and-snap.md`

## Test Plan

- [ ] Build `EzyCad` and `EzyCad_tests` (Release).
- [ ] `ctest -C Release` or run `OperationAxis_JSON_RoundTrip`, mirror/revolve tests.
- [ ] Manual: Operational axis placement, snap suppression after define, Clear axis, mode switch visibility.
- [ ] Manual: Revolve closed edge profile → Shape info shows Solid when conversion succeeds.
- [ ] `sphinx-build -b html -W docs docs/_build` — review `#operation-axis-tool` and sketch snapping section.

## Notes

- Snap suppression applies only when an axis **exists** in Operational axis mode (not during initial two-point placement).
- Axis visibility is mode-gated; stored axis survives tool switches and file reload.
