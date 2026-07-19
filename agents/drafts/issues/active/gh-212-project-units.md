# Project units: Inch/Millimeter display for sketches and New defaults

**Suggested labels:** `enhancement`, `ui`

---

## Title (GitHub)

Project units: Inch/Millimeter display for sketches and New defaults

## Body (GitHub)

### Summary

Add selectable project length units (Inches / Millimeters) for display and input, without rewriting stored geometry. Include Settings defaults for File -> New (unit + 2D view framing).

### Problem

- Sketch and display lengths were hard-coded as inches.
- Users designing metric parts (or switching between house-scale and small parts) had no way to work in millimeters.
- Default 2D view framing lived under 3D view navigation and was easy to mis-set on a wide slider.

### Implemented scope (for drafts after work begins)

**Code changes:**

- `src/utl_types.h`: `Project_unit`, `k_mm_per_inch`
- `src/gui_occt_view.*`: `to_model` / `to_display` / `get_display_to_model_scale`, persist `projectUnit`, New applies default unit
- Length I/O call sites use display scale; import/export keep inch-based `get_dimension_scale()`
- `src/gui_settings.cpp`: **New project defaults** section; drag fields for 2D framing
- `tests/skt_tests.cpp`: `ProjectUnit_displayConversionAndJsonRoundTrip`

**Documentation:**

- `docs/usage.md`, `usage-settings.md`, `usage-sketch.md`, `scripting.md`, `CHANGELOG.md`
- `src/doc/gui.md`, `src/doc/utility.md`

### Acceptance criteria

- [x] File -> Project units switches Inch/Millimeter without rewriting geometry
- [x] `.ezy` persists `projectUnit`; older files default to Inches
- [x] Settings -> New project defaults for unit + framing
- [x] File -> New applies those defaults
- [x] Grid Settings show project unit suffixes
- [x] Export unit picker independent
- [x] Unit test passes
- [x] User docs / CHANGELOG updated

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/212
- PR: (see `agents/drafts/prs/active/`)
