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

On the WASM build the same gesture did **not** multi-select.

EzyCad maps modifiers into OCCT flags:

- `Occt_view::key_flags_from_glfw_` — `GLFW_MOD_ALT` → `Aspect_VKeyFlags_ALT`
- Mouse press/release: `PressMouseButton` / `ReleaseMouseButton` with `theMods`
- Mouse move: `UpdateMousePosition(..., key_flags_from_glfw_window_(), ...)` which polls Alt via `glfwGetKey`

Related input/hotkey tracking: [#93](https://github.com/trailcode/EzyCad/issues/93), draft `agents/drafts/issues/active/gh-93-emscripten-web-hotkeys-followup.md`. OCCT desktop vs wasm kit: [occt-wasm-dual-version](../conventions/occt-wasm-dual-version.md).

## Goal

Make **Alt + LMB drag** multi-select on WASM match desktop, without changing desktop gesture mapping or selection schemes.

## Root cause (confirmed in code)

WASM `init_window` skipped creating `Occt_glfw_win` (correct: `Close()` would destroy the shared canvas). `key_flags_from_glfw_window_()` then always returned `NONE` because it only read modifiers from `m_occt_window`. OCCT rebinds when modifiers drop (`myMouseModifiers != theModifiers`), so Alt+LMB `SelectRectangle` became plain LMB orbit on the next move.

## Approach (landed)

### Phase 0 — reproduce and instrument

- [x] Root cause identified from code path (null `m_occt_window` on WASM → no Alt on move).
- [ ] Manual WASM retest after fix (Chrome/Firefox): Alt+LMB box select; Ctrl+click still works; desktop unchanged.

### Phase 1 — fix input path

- [x] Store non-owning `GLFWwindow* m_glfw_window` in `Occt_view::init_window`.
- [x] Poll modifiers and cursor from `m_glfw_window` (`key_flags_from_glfw_window_`, `cursor_position_`).
- [x] Do not wrap WASM window in owning `Occt_glfw_win`.

### Phase 2 — docs and parity

- [x] Document Alt+drag in `docs/usage.md` and `docs/usage-occt-view.md`; note web Alt/menu caveat.
- [x] `src/doc/gui.md` notes non-owning GLFW pointer for WASM modifier polling.
- [x] `CHANGELOG.md` `[Unreleased]` Fixed entry.
- [ ] Close #220 when verified on WASM.

## Out of scope

- WASM pthreads / parallel OCCT ([wasm-multithreading.md](wasm-multithreading.md)).
- Broader #93 hotkey parity beyond what this gesture needs.
- Changing default OCCT mouse gesture map on desktop.

## Related code

| Area                         | Path                                      |
| ---------------------------- | ----------------------------------------- |
| Modifier → OCCT flags        | `src/gui_occt_view.cpp` (`key_flags_*`)   |
| Mouse → view controller      | `src/gui_occt_view.cpp` (`on_mouse_*`)    |
| GLFW → GUI                   | `src/main.cpp`, `src/gui.cpp`             |
| Draft                        | `agents/drafts/issues/active/gh-220-*.md` |
