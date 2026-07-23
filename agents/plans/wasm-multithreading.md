---
status: planned
topic: wasm-multithreading
depends_on: []
blocks: []
---

# WASM multithreading (Emscripten pthreads)

**Load only when** the prompt is about WASM/Emscripten threads, pthreads, SharedArrayBuffer, parallel OCCT on the web, or enabling the cross-section (or similar) worker pool under `__EMSCRIPTEN__`. Skip otherwise ([token-lean](../conventions/token-lean.md)). Index: [plans/README.md](README.md).

## Context (what already exists)

Desktop cross-section preview already parallelizes per-solid `BRepAlgoAPI_Section` via a small worker pool in `shp_cross_section.cpp` (`for_each_index_` / `section_shapes_on_plane_`). WASM is intentionally serial:

```cpp
#ifdef __EMSCRIPTEN__
  // single-threaded loop
#else
  // std::thread pool sized to hardware_concurrency
#endif
```

AIS / viewer updates stay on the main thread after results are collected. BBox cull skips obvious plane misses on both targets.

Related: [cross-section-tool.md](cross-section-tool.md), [occt-wasm-dual-version](../conventions/occt-wasm-dual-version.md) (desktop OCCT 8 vs wasm 7.9.3).

## Why WASM needs its own plan

Emscripten does not get free `std::thread` parallelism. Real threads require:

1. Compile/link with **`-pthread`** (app + preferably OCCT wasm kit).
2. Browser **SharedArrayBuffer**, which needs **cross-origin isolation** (`Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp` or equivalent) on the hosting page and assets.
3. Larger binary, pthread pool sizing (`PTHREAD_POOL_SIZE` / `PROXY_TO_PTHREAD` choices), and careful main-thread vs worker ownership of OpenGL/AIS.

Until that stack exists, keep the `#ifdef __EMSCRIPTEN__` serial path. Do not pretend `SetRunParallel(true)` alone enables multi-core on current single-threaded wasm builds.

## Goal

Enable the **same parallel map pattern** used by cross-section preview on WASM when isolation + pthread builds are available, without regressing:

- Non-isolated hosts (fallback to serial).
- Desktop behavior.
- OCCT 7.9.3 wasm kit compatibility ([occt-wasm-dual-version](../conventions/occt-wasm-dual-version.md)).

Cross-section is the **first consumer / proof**; later ops (multi-solid Clip half-space, other embarrassingly parallel BREP maps) should reuse one helper.

## Approach (phased)

### Phase 0 — document + gate (no behavior change)

- [ ] Record current desktop pool API shape (`for_each_index_` or a shared `utl_parallel.h` helper).
- [ ] Keep WASM serial by default.
- [ ] Note hosting constraints in [docs/building-occt.md](../../docs/building-occt.md) / wasm run notes when work starts.

### Phase 1 — Emscripten pthread build option

- [ ] CMake/Emscripten option (e.g. `EZYCAD_WASM_PTHREADS=ON`) adding `-pthread` to compile and link.
- [ ] Set sensible defaults: `PTHREAD_POOL_SIZE` (start small, e.g. 4), memory growth still allowed.
- [ ] Rebuild or document whether the **OCCT 7.9.3 wasm kit** must also be built with pthreads (likely yes for allocator / internal parallel safety).
- [ ] CI or local smoke: Release wasm with option off (current) and on (new).

### Phase 2 — Hosting / COOP+COEP

- [ ] Serve demo/pages with COOP/COEP (or document required headers for GitHub Pages / custom host).
- [ ] Runtime detect: if `crossOriginIsolated` is false, **force serial** even in a pthread build (avoid SAB failures).
- [ ] Shell HTML / deploy notes updated so agents and maintainers know why threads are off.

### Phase 3 — Wire shared parallel helper on WASM

- [ ] Lift `for_each_index_` (or equivalent) to a shared utility used by `shp_cross_section` and future callers.
- [ ] Under `__EMSCRIPTEN__` + pthread + `crossOriginIsolated`: use the same worker-pool logic as desktop.
- [ ] Otherwise: serial loop (today’s behavior).
- [ ] Still: **no AIS/OCCT viewer calls from workers**; only BREP compute, then main-thread Display.

### Phase 4 — Hardening

- [ ] Stress test: many selected solids in cross-section preview (Offset drag) on Chrome/Firefox/Safari as available.
- [ ] Watch for OCCT allocator / intermittent section crashes under concurrent `BRepAlgoAPI_Section` (known historical risk); fall back to serial if unstable on 7.9.3.
- [ ] Optional: cancel/stale-token so an old preview does not finish after a newer one (helps UI more than threads alone).
- [ ] Binary size / startup cost note in CHANGELOG when enabling by default.

## Non-goals (for this plan)

- Web Worker with a second full OCCT instance (heavier architecture; revisit only if pthreads prove insufficient).
- Making all OCCT booleans internally `SetRunParallel` on wasm without measuring.
- Requiring pthreads for every wasm deploy (isolated hosting may stay optional).

## Success

- With pthread build + cross-origin isolation, multi-solid cross-section preview uses multiple cores on wasm similarly to desktop.
- Without isolation or with pthreads off, behavior matches today’s single-threaded wasm (no crash, no SAB error).
- One reusable parallel-for helper; cross-section remains the reference caller.

## Likely touch points

| Area                         | Notes                                                                 |
| ---------------------------- | --------------------------------------------------------------------- |
| `src/shp_cross_section.cpp`  | First consumer; drop hard serial-only `#ifdef` once helper exists     |
| New `utl_parallel.*` (maybe) | Shared `for_each_index` / pool; desktop + wasm gated                  |
| `CMakeLists.txt`             | `EZYCAD_WASM_PTHREADS`, link flags, pool size                         |
| `scripts/build-occt-*-wasm*` | Rebuild OCCT kit with `-pthread` if required                          |
| `web/EzyCad.html` / host     | COOP/COEP or meta documentation                                       |
| Docs                         | `building-occt.md`, maybe short wasm run section                      |

## Open questions

1. Must the OCCT wasm kit itself be pthread-linked, or is app-only `-pthread` enough for independent `BRepAlgoAPI_Section` instances?
2. Default ON for demo builds, or opt-in forever for smaller non-isolated deploys?
3. Safari / iOS SharedArrayBuffer support matrix for our supported browsers?
