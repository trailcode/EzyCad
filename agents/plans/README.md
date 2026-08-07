# Feature plans

Long-lived design notes for features. Keep files after shipping as reference (`status: done`).

**Token rule:** Do **not** load files in this directory unless the user prompt is clearly about that feature. See [conventions/token-lean.md](../conventions/token-lean.md). Never bulk-load `plans/`.

## Status values

YAML `status` at the top of each plan:

| Status     | Meaning                                           |
| ---------- | ------------------------------------------------- |
| `planning` | Design only / not started                         |
| `partial`  | Useful slice shipped; acceptance criteria open    |
| `done`     | Plan scope met; file kept as reference            |
| `deferred` | Explicitly postponed (post-v1, blocked, or later) |

Update `status` when a matching PR merges or the work is shelved. Prefer `done` over synonyms (`implemented`, etc.).

## Discovery order (sketch-from-shape track)

```text
shp-origin-orientation  -->  cross-section-tool  -->  sketch-from-shape-section
     (partial)                 (done)                    (partial)

sketch-mode-shape-faint  (done; parallel UX)
```

| Plan                                                             | Status     | Load only when prompt is about                                         |
| ---------------------------------------------------------------- | ---------- | ---------------------------------------------------------------------- |
| [shp-origin-orientation.md](shp-origin-orientation.md)           | partial    | shape origin, shape frame, shape axes / orientation annotation         |
| [cross-section-tool.md](cross-section-tool.md)                   | done       | cross-section tool, section cut preview, cutting-plane experiment      |
| [sketch-from-shape-section.md](sketch-from-shape-section.md)     | partial    | "sketch from shape", section → editable sketch                         |
| [sketch-mode-shape-faint.md](sketch-mode-shape-faint.md)         | done       | faint/ghost/wireframe shapes in sketch mode, alpha blending solids     |
| [shape-list-hierarchy-phase3.md](shape-list-hierarchy-phase3.md) | deferred   | parent transform inheritance, Parts/planes, Boolean history (#214)     |
| [assembly-inspection-mode.md](assembly-inspection-mode.md)       | deferred   | assembly idle/inspection mode, Part vs arrange context, Move semantics |
| [wasm-multithreading.md](wasm-multithreading.md)                 | planning   | WASM/Emscripten pthreads, SharedArrayBuffer, parallel OCCT on web      |
| [wasm-alt-drag-multiselect.md](wasm-alt-drag-multiselect.md)     | implemented | WASM Alt+LMB drag rectangle multi-select (#220); verify on WASM then close |
| [configurable-hotkeys.md](configurable-hotkeys.md)               | done       | remappable shortcuts, keybindings, `gui.hotkeys`, free Dimension off D |
