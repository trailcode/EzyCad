# OCCT desktop vs WASM (until v8 wasm works)

**Temporary:** keep this file until EzyCad WASM builds and runs correctly against OCCT **8.x** (GLES shading and any remaining regressions fixed; see [docs/bugs.md](../../docs/bugs.md)). Then delete this convention and drop the pointers from `AGENTS.md` / `agents/README.md` / `token-lean.md`.

## Pins

| Target            | OCCT version                        | Notes                                      |
| ----------------- | ----------------------------------- | ------------------------------------------ |
| Desktop (Windows) | **8.0.0**                           | Prebuilts + `TKOpenGl`                     |
| WASM (Emscripten) | **7.9.3** (`V7_9_3`) recommended    | `scripts/build-occt-793-wasm.ps1`          |
| WASM (compare)    | 8.0.0.p1 (`V8_0_0_p1`) experimental | Shading regressions; not for demos/release |

Details: [docs/building-occt.md](../../docs/building-occt.md), [workflows/local-dev.md](../workflows/local-dev.md).

## Shared `src/` must compile on both

Most app code is one tree linked against **desktop 8** and **wasm 7.9.3**. Prefer the **API intersection**. Do not assume an OCCT 8-only symbol or overload is available on the wasm kit.

### `Standard_Failure` messages

Use `standard_failure_message(e)` from [`utl_occt.h`](../../src/utl_occt.h) — not raw `what()` or `GetMessageString()`:

```cpp
catch (const Standard_Failure& e)
{
  const char* msg = standard_failure_message(e);
  // ...
}
```

OCCT **8** (desktop) deprecates `GetMessageString()` in favor of `what()`; OCCT **7.9.3** (wasm) has no `what()` on `Standard_Failure`. The helper picks the right API via `OCC_VERSION_HEX` so shared code builds on both without warnings or missing-member errors.

### Other dual-version habits

- Prefer existing call patterns already used for both targets over newer OCCT 8 helpers.
- If you must use an 8-only API, gate it or verify a wasm 7.9.3 configure/build still succeeds.
- Default wasm work to **7.9.3**; use the v8 wasm script only when comparing upstream regressions.

## When retiring this note

After wasm on OCCT 8 is the supported path: switch demo/release scripts to `build-occt-v8-wasm.ps1`, update [docs/building-occt.md](../../docs/building-occt.md) / [docs/bugs.md](../../docs/bugs.md), then remove this file and its index links.
