# PR - sketch-tool-input-docs-options-pane-help

## Title

Docs + UI: document Tab + Shift+Tab for Operation Axis / Rectangles / Slot / Circle + add consistent Options pane headers + contextual "?" help buttons (incl. for Inspection modes)

## Summary

- Bring the user guide (usage-sketch.md) up to date for every remaining sketch creation tool so it documents both precise distance (Tab) and angle (Shift+Tab) during placement, with full "Angle Constraint" subsections, updated Features/How-to/Shortcuts/Tips, and notes on use cases (future snapping, etc.).
- Special handling for Slot (first edge = length/orientation of the slot, radius point = the "second edge dim" / arc radius) and for tools that auto-align (e.g. Rectangle Two Points).
- Add (or ensure) a top-level tool/mode header using `current_mode_description()` followed by a small "?" button in the Options pane for *all* modes that render options content, including pure Normal (Inspection) and Sketch_inspection_mode.
- The "?" buttons use a central `get_doc_url_for_mode(Mode)` map (with runtime size assert vs Mode::_count) so future tools automatically get correct contextual links (e.g. #operation-axis-tool, #slot-creation-tool, #rectangle-tool-center-point, etc.).
- Added the generic `agents/issues/issue.md` and `agents/prs/PR.md` templates and referenced them from agents/README.md (for future drafts).
- Minor supporting changes: doc style guide note about the new ? buttons, unit test for link existence, etc.

This makes the in-app Options experience (headers + help) and the written user guide consistent, and gives users (and future agents) an easy way to jump from the UI to the exact docs for the active tool.

## Files Changed

- `docs/usage-sketch.md` (main user-guide updates for the listed tools)
- `src/gui_mode.cpp` (header+button calls for Normal + new Sketch_inspection_mode handler, map + assert, options_doc_help_button_ helper)
- `src/gui.h` (declarations)
- `src/gui.cpp` (current_mode_description lookup from toolbar data)
- `agents/issues/009-sketch-tool-precise-input-docs-and-options-help.md` (this issue draft)
- `agents/prs/004-sketch-tool-input-docs-options-pane-help.md` (this PR draft)
- `agents/README.md` (added references to the new generic templates)
- `docs/ezycad_doc_style.md` (note about Options-pane ? buttons + links)
- `CHANGELOG.md` (under [Unreleased])
- `tests/doc_link_tests.cpp` (if not already on branch)

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/ (to be created from the draft)
- Branch compare (once pushed)
- Prior related drafts: agents/issues/003-..., agents/prs/001-..., 002-..., agents/issues/006-..., and the earlier "Hot keys" / "Options pane help links" commits on this branch.
- `agents/user-docs-sync.md` (user-visible docs + in-app UI change)

## Test Plan

- [ ] `sphinx-build -b html -W docs docs/_build` (or RTD preview) + review the updated tool sections and new Angle Constraint subsections.
- [ ] `EzyCad_tests --gtest_filter=DocLinkTests.*` (the map URLs should be reachable; the test exercises get_doc_url_for_mode for every Mode that has a specific entry).
- [ ] Manual verification:
  - Switch to each updated tool (Operation Axis, both Rectangles, Slot, Circle, Square, etc.) and pure Inspection / Sketch inspection modes.
  - Confirm the Options pane now shows the tool name header + "?" button right after it.
  - Click the "?" and verify it opens the expected specific section (e.g. #operation-axis-tool, #slot-creation-tool with the length-vs-radius note).
- [ ] For each tool, verify that Tab and Shift+Tab work during placement as described (and that the in-app "Shortcuts" text under the tool's Options section matches the user guide).
- [ ] Check the assert in get_doc_url_for_mode (temporarily remove one map entry and confirm it fires on first call).
- [ ] Build (native Debug + Release) succeeds; no regressions in other Options content or hotkey routing.
- [ ] Cross-check `agents/user-docs-sync.md` and that CHANGELOG has an entry.
- [ ] Review the new agents/ templates and the updates to agents/README.md.

## Notes

- For the Rectangle (Two Points) tool the final geometry is always axis-aligned, so angle only affects the temporary rubber-band; the docs note this explicitly.
- Slot is documented with the two-stage semantics the user described (first edge = length/orientation, radius point = radius).
- The map already had most of the anchors; a couple of "general" or "planned" entries have comments and fall back gracefully.
- This is primarily a documentation + small UI polish change (the underlying Tab/Shift+Tab routing and options_sketch_len_angle_hotkeys_ helper were already present for these modes).
- Unrelated: the usual build artifacts / Python DLL copy noise in the log are unrelated to this PR.

## Post-merge (checklist for the author)

- [ ] Update / close the linked GitHub issue.
- [ ] If any follow-up work is needed (e.g. adding more specific anchors, updating the general hotkeys summary table, or adding angle support notes elsewhere), open a new issue draft from the template.
- [ ] Consider whether the top-level "Hotkeys" summary table in usage-sketch.md should be broadened now that more tools document angle input.