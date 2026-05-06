# [Draft] Emscripten / Web: 3D view hotkeys and input (follow-up to PR #97)

**Opened on GitHub from this draft.** Suggested labels: `bug`, `platform: web` (or `emscripten` if you use that label)

**Parent / context:** [PR #97 — 3D view hotkeys, settings, About, tooling, docs](https://github.com/trailcode/EzyCad/pull/97)

---

## Title (GitHub)

Emscripten / Web: 3D view hotkeys and keyboard input bugs (follow-up to PR #97)

## Body (GitHub)

### Summary

The **Emscripten (Web / WASM)** build is called out on [PR #97](https://github.com/trailcode/EzyCad/pull/97) as having **bugs** around the new **3D view hotkeys**; navigation is partly **exposed via the toolbar** instead of full keyboard parity with the desktop build.

This issue tracks **making Web behavior correct and documented**, not blocking the desktop PR.

### Scope

- **Keyboard 3D navigation** on Web: numpad orbit, roll, zoom, axis snap — whatever GLFW/browser key routing supports under Emscripten.
- **Consistency** with `usage.md` / `usage-occt-view.md` (clearly mark Web limitations or fix behavior).
- **Regression checks** for existing `#ifdef __EMSCRIPTEN__` paths in `src/gui.cpp`, `src/gui_settings.cpp`, `src/gui_mode.cpp`, `CMakeLists.txt`, etc.

### Known gaps (fill in as you reproduce)

- [ ] Document concrete failures (e.g. keys swallowed, wrong modifiers, no repeat, focus).
- [ ] Toolbar vs keyboard: intended UX for Web until keys are fixed.

### Acceptance criteria

- [ ] Either **Web matches documented desktop shortcuts** where technically feasible, or **docs + UI** state what works on Web and why.
- [ ] PR #97 (or a small follow-up PR) **updates the testing checklist** so Emscripten is not an vague “if applicable” line.
- [ ] No **silent** broken shortcuts: if a key does nothing on Web, users see a hint (docs or in-app).

### Links

- PR: https://github.com/trailcode/EzyCad/pull/97
- Related umbrella: https://github.com/trailcode/EzyCad/issues/93 (hot keys)

---

## Metadata (do not paste into GitHub)

- Draft path: `agents/issues/002-emscripten-pr97-web-hotkeys.md`
- Repo: `trailcode/EzyCad`
