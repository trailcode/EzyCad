# Operational axis: visibility, snap suppression, persistence, and toolbar label

**Suggested labels:** `enhancement`, `sketch`, `ui`, `docs`

---

## Title (GitHub)

Operational axis mode: show axis only in tool, suppress snap/markers when defined, persist in JSON

## Body (GitHub)

### Summary

Improve the **Operational axis** sketch tool so the axis line is visible only while that mode is active, permanent **+** node markers and sketch snap are suppressed after the axis is defined (mirror/revolve selection phase), the toolbar tooltip reads **Operational axis**, and operational axes round-trip in sketch project JSON.

### Problem

- The operational axis line stayed visible when switching to other sketch modes, cluttering the view.
- After defining an axis for Mirror/Revolve, permanent node markers and snap guides interfered with selecting edges and faces (`docs/bugs.md` tracked both persistence and snap/node suppression).
- Toolbar tooltip said "Define operation axis" while the tool also supports mirror/revolve workflows after definition.
- Operational axes were not persisted in saved projects.

### Implemented scope

**Code changes:**

- `src/sketch.cpp` / `src/sketch.h`: `sync_operation_axis_display_()`, `show_operation_axis_()`, `operation_axis_suppresses_sketch_snap_()`; permanent node sync hides markers when axis is defined in Operational axis mode; finalize/clear axis refresh display and annotations.
- `src/sketch_nodes.cpp`: early return in snap paths when `Occt_view::sketch_snap_suppressed()`.
- `src/occt_view.cpp` / `src/occt_view.h`: `sketch_snap_suppressed()`.
- `src/sketch_json.cpp`: serialize/load `operation_axis` (two plane points).
- `src/gui.cpp`: toolbar tooltip **Operational axis**.
- `tests/sketch_tests.cpp`: JSON round-trip for operation axis.

**Documentation:**

- `CHANGELOG.md` — Operation axis / Revolve section expanded.
- `docs/usage-sketch.md` — Operational axis tool, sketch snapping table, visibility and snap-suppression behavior.
- `docs/usage.md` — toolbar icon list.
- `docs/bugs.md` — struck through persistence and snap/node items.

### Acceptance criteria

- [ ] Operational axis line visible only in Operational axis mode.
- [ ] After axis is defined: no permanent **+** markers, no sketch snap until Clear axis or mode change.
- [ ] Phase 1 axis placement still snaps normally.
- [ ] Toolbar and Options mode description say **Operational axis**.
- [ ] `operation_axis` persists in sketch JSON save/load.
- [ ] User guide and changelog match behavior.
- [ ] `EzyCad_tests` pass (including `OperationAxis_JSON_RoundTrip`).

### Files touched

- `src/sketch.cpp`, `src/sketch.h`, `src/sketch_nodes.cpp`, `src/sketch_json.cpp`
- `src/occt_view.cpp`, `src/occt_view.h`, `src/gui.cpp`
- `tests/sketch_tests.cpp`
- `CHANGELOG.md`, `docs/usage-sketch.md`, `docs/usage.md`, `docs/bugs.md`
- `agents/issues/013-operational-axis-visibility-and-snap.md` (this draft)

### Related

- `docs/bugs.md` items (now struck through).
- Branch also includes revolve solid conversion, shape info dialog — see PR for full branch scope.
- GitHub issue: https://github.com/trailcode/EzyCad/issues/142
- GitHub PR: https://github.com/trailcode/EzyCad/pull/143

### Test plan

- [ ] Define operational axis; confirm snap during placement.
- [ ] After second click: axis visible, no **+** markers, no snap guides when moving cursor.
- [ ] Mirror/Revolve selection works; Clear axis restores markers and snap.
- [ ] Switch to another sketch tool: axis hidden; return to Operational axis: axis visible again.
- [ ] Save/reload project: axis restored; hidden when not in Operational axis mode.
- [ ] Run `OperationAxis_JSON_RoundTrip` and full `EzyCad_tests`.
