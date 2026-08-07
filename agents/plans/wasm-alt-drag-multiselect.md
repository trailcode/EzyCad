---
status: implemented
topic: wasm-alt-drag-multiselect
depends_on: []
blocks: []
github_issue: 220
---

# WASM Alt + LMB drag multi-select

**Load only when** the prompt is about WASM/Emscripten Alt+drag, rectangle / rubber-band selection, or multi-select failing on web. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

**Issue:** https://github.com/trailcode/EzyCad/issues/220

## Context

On desktop, holding **Alt** and **left-dragging** in the 3D view activates OCCT `AIS_ViewController` rectangle selection (`SelectRectangle` / rubber band) and multi-selects shapes under the box.

On the WASM build the same gesture did **not** multi-select (Alt never reached OCCT on mouse-move). Separately, stock `SelectRectangle` could miss complex / STEP solids even when simple root boxes were selected (desktop and web).

## Root causes (confirmed)

1. **WASM Alt polling** — `init_window` skipped `Occt_glfw_win` (correct: `Close()` would destroy the shared canvas). `key_flags_from_glfw_window_()` always returned `NONE`, so OCCT rebound Alt+LMB `SelectRectangle` to orbit on the next move.
2. **Sensitive-entity miss** — OCCT rubber-band pick skipped some imported BREPs; fixed by projecting document-solid AABBs into screen space after `SelectRectangle`.
3. **WASM pixel space** — map projected corners from OCCT/`Wasm_Window` size into GLFW cursor space; keep canvas size synced before project.
4. **Shape List UX** — current-group row used the same strong tint as AIS selection (looked like grouped solids stayed selected).

## Approach (landed)

### Phase 1 — input path

- [x] Non-owning `GLFWwindow* m_glfw_window` for modifier/cursor polling (no owning `Occt_glfw_win` on WASM).
- [x] `handleSelectionPoly` + `select_shps_intersecting_screen_rect_` (overlap AABB; `Handle(...)` for OCCT 7.9.3 / 8).
- [x] Shape List: selection tint from AIS only; weaker tint for current group.

### Phase 2 — docs and parity

- [x] `docs/usage.md`, `docs/usage-occt-view.md` (Alt+drag + web Alt caveat).
- [x] `src/doc/gui.md` (GLFW pointer, AABB supplement, Shape List tint).
- [x] `CHANGELOG.md` `[Unreleased]`.
- [x] Manual: native + WASM Alt+LMB (including complex solids under groups).
- [ ] Close #220 on GitHub when ready.

## Out of scope

- WASM pthreads ([wasm-multithreading.md](wasm-multithreading.md)).
- Broader #93 hotkey parity.
- Changing default OCCT mouse gesture map on desktop.

## Related code

| Area                    | Path                                                    |
| ----------------------- | ------------------------------------------------------- |
| Modifier / cursor       | `src/gui_occt_view.cpp` (`key_flags_*`, `cursor_position_`) |
| Rubber-band supplement  | `handleSelectionPoly`, `select_shps_intersecting_screen_rect_` |
| Shape List tint         | `src/gui.cpp` (`shape_list_`)                           |
| Draft                   | `agents/drafts/issues/active/gh-220-*.md`               |
