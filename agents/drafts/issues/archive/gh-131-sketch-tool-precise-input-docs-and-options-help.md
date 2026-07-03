# Sketch tool docs: Tab/Shift+Tab for all creation tools + Options pane headers and ? help buttons

**Suggested labels:** `enhancement`, `docs`, `sketch`, `ui`

**Merged as:** https://github.com/trailcode/EzyCad/pull/132 (closes #131)

---

## Title (GitHub)

Docs: document Tab + Shift+Tab precise input for remaining sketch tools (Operation Axis, Rectangles, Slot) and add consistent Options pane headers + contextual ? help buttons (including for Inspection and Sketch inspection modes)

## Body (GitHub)

### Summary

Improve discoverability and accuracy of the user guide by ensuring every sketch creation tool documents both distance (Tab) and angle (Shift+Tab) inputs during placement. Add a consistent top-level header (tool name from toolbar) + "?" documentation button in the Options pane for all relevant modes, including pure Inspection and Sketch inspection modes. The ? buttons link directly to the relevant usage guide section via a central map.

### Problem

- Several tools (Operation Axis, Rectangle Two Points, Rectangle Center, Slot, and previously Circle/Square) had incomplete "How to use" / Shortcuts / Features sections in usage-sketch.md that only mentioned Tab (distance) and omitted Shift+Tab (angle constraint).
- The in-app Options pane (via options_sketch_len_angle_hotkeys_() and the shared TAB/Shift+TAB routing) already advertised both, creating a docs/code mismatch.
- Pure Inspection (Normal) and Sketch inspection modes had no top-level tool header + contextual "?" help button in the Options pane, unlike the active tool modes.
- No central, maintainable place for per-mode docs URLs; updates were scattered and easy to miss.
- Future work (snapping on circle nodes, angled slots, etc.) relies on users knowing about angle-constrained placement.

### Implemented scope

**Documentation (docs/usage-sketch.md):**

- Circle (Center-Radius), Square, Rectangle (Two Points), Rectangle (Center Point), Slot, and Operation Axis Tool sections now fully document:
  - Tab for length/radius/dimension.
  - Shift+Tab for angle/orientation (with ordering notes and "when using both").
- Added "Angle Constraint:" subsections with details on activation, convention (CCW from +X), snap disabling, and use cases (e.g., "for future snapping operations").
- Updated Features tables, How to use steps, Shortcuts tables, and Tips.
- Slot section specially notes the two-stage nature (first edge = length/orientation of slot, radius point = radius/"second edge dim").
- Operation Axis now correctly documents its two-point axis placement with both inputs.

**Options pane UI + help links (src/):**

- Ensured *every* mode that renders an Options pane (including Normal/Inspection and Sketch_inspection_mode) now starts with:
  - `current_mode_description()` (pulled from m_toolbar_buttons tooltip data, or c_mode_strs as fallback).
  - `options_doc_help_button_()` — a small "?" that calls `open_url_(get_doc_url_for_mode(get_mode()))`.
- Added `get_doc_url_for_mode(Mode)` static map with one entry per Mode (asserted to match Mode::_count at runtime).
- Specific anchors: e.g. Sketch_add_node → usage-sketch.html#add-node-tool, Operation Axis → #operation-axis-tool, Slot (with note on its semantics), etc.
- For planned/empty entries (e.g. Sketch_add_circle_3_pts) we fall back gracefully and left a comment.
- Updated calls in all relevant *_mode_() functions and the common helper.
- Added `options_sketch_inspection_mode_()` + case in the switch, and header+button at the top of `options_normal_mode_()` for completeness.

**Other:**

- `agents/drafts/issues/009-...md` and `agents/drafts/prs/004-...md` (these drafts).
- `agents/README.md` table now lists the new generic issue.md / PR.md templates.
- Minor supporting updates (unit test for link existence, doc style guide mention of the new ? buttons, etc.).
- `CHANGELOG.md` entry under [Unreleased] (docs + UI).

### Acceptance criteria

- [ ] All per-tool sections in usage-sketch.md now document Tab + Shift+Tab (with angle notes and ordering).
- [ ] Options pane for Normal (Inspection) and Sketch_inspection_mode now has the tool header + "?" button.
- [ ] "?" buttons open the correct specific sections (map + test pass).
- [ ] `get_doc_url_for_mode` asserts size == Mode::_count.
- [ ] `EzyCad_tests` (including the new doc link test) and native build succeed.
- [ ] User guide matches in-app Options pane behavior and the new header + help UI.
- [ ] No regression in existing tool docs or Options content.

### Files touched

- `docs/usage-sketch.md`
- `src/gui_mode.cpp`
- `src/gui.h`
- `src/gui.cpp` (minor, for current_mode_description)
- `agents/drafts/issues/archive/gh-131-sketch-tool-precise-input-docs-and-options-help.md` (this draft)
- `agents/drafts/prs/archive/gh-132-sketch-tool-input-docs-options-pane-help.md`
- `agents/README.md`
- `docs/ezycad_doc_style.md` (optional cross-ref)
- `CHANGELOG.md`
- `tests/doc_link_tests.cpp` (if not already)

### Related

- Prior options pane work: `agents/drafts/issues/archive/gh-98-ui-improvements-sketch-shape-lists-and-style.md`, `agents/drafts/prs/001-...md`
- `agents/conventions/user-docs-sync.md` (this is a user-visible docs + in-app UI change)
- Earlier hotkeys / Tab+Shift+Tab docs work on the branch
- GitHub issue (to be created)

### Test plan (for implementation drafts)

- [ ] `sphinx-build -b html -W docs docs/_build` (or RTD preview) and review the updated tool sections.
- [ ] Run `EzyCad_tests --gtest_filter=DocLinkTests.*` (verifies the map URLs are reachable).
- [ ] Manual: switch to each updated tool (incl. pure Inspection and Sketch inspection), verify Options header + "?" appears and opens the right page.
- [ ] Verify in-app "Shortcuts" text under Options for these tools matches the updated user guide.
- [ ] Check that angle-constrained placement works for Operation Axis (first edge length + direction) and Slot (first edge = length/orientation, radius point = radius).
- [ ] Cross-check `agents/conventions/user-docs-sync.md` and add to CHANGELOG.
- [ ] Build (native Debug/Release) and run relevant manual tests.