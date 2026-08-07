---
github_issue: 220
github_pr: 250
status: active
paired_draft: ../issues/active/gh-220-wasm-alt-drag-multiselect.md
---

# PR - Trailcode/wasm-box-select-fix

## Title

Fix WASM Alt+LMB rectangle multi-select (#220)

## Summary

- WASM Alt+LMB rubber-band multi-select via non-owning `GLFWwindow*` modifier/cursor polling.
- AABB screen-rect supplement after `SelectRectangle` (complex / STEP solids; WASM OCCT↔GLFW mapping).
- Shape List current-group tint distinct from AIS selection.
- Docs + plan updated.

## Related

- Issue: https://github.com/trailcode/EzyCad/issues/220
- PR: https://github.com/trailcode/EzyCad/pull/250
- Branch: `Trailcode/wasm-box-select-fix`
- Plan: `agents/plans/wasm-alt-drag-multiselect.md`

## Test Plan

- [x] Desktop Alt+LMB (incl. complex solids under groups)
- [x] WASM Alt+LMB; Ctrl+click still works
- [x] Shape List current-group vs selection tint
- [ ] CI / `Grouped_solids_stay_displayed_and_selectable`
