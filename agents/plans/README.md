# Feature plans

Long-lived design notes for features not yet (or not fully) implemented.

**Token rule:** Do **not** load files in this directory unless the user prompt is clearly about that feature. See [conventions/token-lean.md](../conventions/token-lean.md). Never bulk-load `plans/`.

## Discovery order (sketch-from-shape track)

```text
shp-origin-orientation  -->  cross-section-tool  -->  sketch-from-shape-section
     (frame first)            (prototype/learn)         (deferred until findings)

sketch-mode-shape-faint  (parallel UX: ghost/wire shapes while sketching)
```

| Plan                                                             | Load only when prompt is about                                        |
| ---------------------------------------------------------------- | --------------------------------------------------------------------- |
| [shp-origin-orientation.md](shp-origin-orientation.md)           | shape origin, shape frame, shape axes / orientation annotation        |
| [cross-section-tool.md](cross-section-tool.md)                   | cross-section tool, section cut preview, cutting-plane experiment     |
| [sketch-from-shape-section.md](sketch-from-shape-section.md)     | "sketch from shape", section → editable sketch (end-state; deferred)  |
| [sketch-mode-shape-faint.md](sketch-mode-shape-faint.md)         | faint/ghost/wireframe shapes in sketch mode, alpha blending solids    |
| [shape-list-hierarchy-phase3.md](shape-list-hierarchy-phase3.md) | parent transform inheritance, Parts/planes, Boolean history (#214)    |
| [wasm-multithreading.md](wasm-multithreading.md)                 | WASM/Emscripten pthreads, SharedArrayBuffer, parallel OCCT on web     |
| [wasm-alt-drag-multiselect.md](wasm-alt-drag-multiselect.md)     | WASM Alt+LMB drag rectangle multi-select broken (#220)                |
