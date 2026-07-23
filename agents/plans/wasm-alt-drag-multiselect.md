---
status: planned
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

On the WASM build the same gesture does **not** multi-select.

EzyCad already maps modifiers into OCCT flags:

- `Occt_view::key_flags_from_glfw_` — `GLFW_MOD_ALT` → `Aspect_VKeyFlags_ALT`
- Mouse press/release: `PressMouseButton` / `ReleaseMouseButton` with `theMods`
- Mouse move: `UpdateMousePosition(..., key_flags_from_glfw_window_(), ...)` which polls `glfwGetKey(... LEFT/RIGHT_ALT ...)`

Related input/hotkey tracking: [#93](https://github.com/trailcode/EzyCad/issues/93), draft `agents/drafts/issues/active/gh-93-emscripten-web-hotkeys-followup.md`. OCCT desktop vs wasm kit: [occt-wasm-dual-version](../conventions/occt-wasm-dual-version.md).

## Goal

Make **Alt + LMB drag** multi-select on WASM match desktop, without changing desktop gesture mapping or selection schemes.

## Likely causes (investigate in order)

1. **Alt not present on pointer events under Emscripten/GLFW** — browser menu focus, `preventDefault`, or incomplete `mods` on `glfwSetMouseButtonCallback`.
2. **`glfwGetKey(ALT)` false during drag** — rubber-band gesture needs Alt on move updates (`key_flags_from_glfw_window_`), not only on button press.
3. **ImGui / focus** — canvas loses keyboard focus when Alt is pressed; Alt never reaches GLFW.
4. **OCCT 7.9.3 wasm kit** — confirm default `MouseGestureMap` still binds Alt+LMB to select-rectangle (unlikely difference, but verify if input looks correct).

## Approach

### Phase 0 — reproduce and instrument

- [ ] Confirm desktop Alt+LMB rubber band still works.
- [ ] On WASM, log (temporary) `mods` on LMB press/release and Alt from `key_flags_from_glfw_window_` during drag.
- [ ] Note browser (Chrome/Firefox/Edge) and whether the OS/browser steals Alt.

### Phase 1 — fix input path

- [ ] Ensure Alt is forwarded for press, move, and release while the gesture is active (Emscripten/GLFW and/or synthetic modifier from `event.altKey` if GLFW is incomplete).
- [ ] Avoid focusing browser chrome on Alt when the canvas has focus (as far as the platform allows).
- [ ] Keep `GUI::on_mouse_button` / sketch `mods == 0` click paths unchanged for unmodified LMB.

### Phase 2 — docs and parity

- [ ] Document Alt+drag multi-select in `docs/usage.md` and/or `docs/usage-occt-view.md`; call out any remaining Web limitation.
- [ ] Manual check: Ctrl+click multi-select still works on WASM if it already does; Alt+drag matches desktop.
- [ ] Close #220 when done; update this plan status.

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
