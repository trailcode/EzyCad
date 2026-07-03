# PR - `Trailcode/ui_improvements`

## Title

UI improvements for sketch/shape lists and coding style guidance

## Summary

- Make Sketch List selection switch GUI into `Sketch_inspection_mode`.
- Improve Sketch List and Shape List behavior in narrow panes (horizontal-scroll content and aligned name-field width handling).
- Refactor shared list name-width logic into `GUI::list_name_field_width_`.
- Update style docs and Cursor rule guidance for reader-first `.cpp` organization, helper placement near use, and pragmatic DRY guidance.
- Add issue drafts for branch context and follow-up tracking.

## Files Changed

- `src/gui.cpp`
- `src/gui.h`
- `src/gui_settings.cpp`
- `ezycad_code_style.md`
- local AI assistant rule file (not committed to the repo)
- `agents/issues/003-ui-improvements-sketch-shape-lists-and-style.md`

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/98
- Compare: https://github.com/trailcode/EzyCad/compare/main...Trailcode/ui_improvements

## Test Plan

- [x] Build `EzyCad_lib` in Release config.
- [ ] In Sketch List, click a sketch row selector and verify mode becomes sketch inspection.
- [ ] Resize Sketch List very narrow and verify controls remain reachable via horizontal scroll.
- [ ] Resize Shape List very narrow and verify controls remain reachable via horizontal scroll.
- [ ] Rename sketches and shapes with short and long names; verify input widths are usable and consistent.
- [ ] Verify no ImGui ID-stack warnings appear while interacting with both lists.

## Notes

- There is an unrelated untracked directory in this branch workspace: `third_party/ImGuiColorTextEdit/`.
