# Local development and testing

Short notes for day-to-day build, test, and quality checks.

## Desktop build (Windows + Visual Studio 2026 / MSVC)

**Prerequisites**
- CMake that supports generator **Visual Studio 18 2026** (VS 2026 bundled CMake 4.3+ is fine; older PATH CMake may lack this generator)
- Visual Studio 2026 (C++ workload) — primary local toolchain
- nuget CLI (for GLFW/GLEW)
- OCCT 8.0.0 prebuilts + 3rdparty-vc14-64 (strongly recommended — see [docs/building-occt.md](../../docs/building-occt.md))

**Typical configure** (run from repo root):

```powershell
# Prefer VS-bundled cmake if PATH cmake is too old for the VS 18 generator:
#   "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 `
  -DOpenCASCADE_DIR=C:\path\to\occt-8.0.0\cmake `
  -DOCCT_3RD_PARTY_DIR=C:\path\to\3rdparty-vc14-64
```

Do **not** use `-G "Visual Studio 17 2022"` on a machine that only has VS 2026 installed — configure fails with a generator/instance mismatch. CI still uses VS 2022 (`windows-msvc.yml`).

**Build**:

```powershell
cmake --build build --config Release
# or open build/EzyCad.sln in Visual Studio and build the EzyCad project
```

The resulting `EzyCad.exe` (and `EzyCad_tests.exe`) plus required DLLs end up under `build/Debug` or `build/Release`.

**Running tests**:

```powershell
cd build
ctest -C Release --output-on-failure
# Or run the EzyCad_tests project / executable directly from Visual Studio
```

**After CMake structural changes** (e.g. adding new folders for IDE visibility under `agents/`, `docs/`, or `github-workflows`), or when switching generators (e.g. VS 17 -> VS 18):
- Clean the configure cache: delete `build/CMakeCache.txt`, `build/CMakeFiles/`, and any `_deps/*-subbuild/` directories.
- Re-run the configure step above.

See root [README.md](../../README.md#building-instructions) and [docs/building-occt.md](../../docs/building-occt.md) for more OCCT details and troubleshooting.

## WebAssembly (Emscripten)

**Agent note:** desktop uses OCCT **8**, recommended wasm uses **7.9.3** until OCCT 8 wasm works. Shared `src/` must compile on both — see [conventions/occt-wasm-dual-version.md](../conventions/occt-wasm-dual-version.md).

Full OCCT + EzyCad wasm instructions live in [docs/building-occt.md](../../docs/building-occt.md#webassembly-emscripten) and the root README.

High-level flow (after `emsdk_env`):

1. Build the OCCT wasm package (if not already done):

   ```powershell
   # Recommended (7.9.3)
   .\scripts\build-occt-793-wasm.ps1

   # OCCT 8.0.0.p1 (upstream regression compare)
   .\scripts\build-occt-v8-wasm.ps1
   ```

   Default install trees: `%USERPROFILE%\occt-wasm-build\V7_9_3\install` and `...\V8_0_0_p1\install`.

2. Configure + build EzyCad (Ninja generator recommended):

   ```powershell
   emcmake cmake -S . -B build-em-7-9-3 -G Ninja -Wno-dev `
     -DOpenCASCADE_DIR=$env:USERPROFILE\occt-wasm-build\V7_9_3\install\lib\cmake\opencascade `
     -DCMAKE_BUILD_TYPE=Release
   ninja -C build-em-7-9-3
   ```

3. Serve locally:

   ```powershell
   ninja && python -m http.server 8000
   ```

   Load `EzyCad.html` (or the copy under `web/`) in a browser.

See `scripts/build-occt-793-wasm.ps1`, `scripts/build-occt-v8-wasm.ps1`, and shared `scripts/build-occt-wasm.ps1` (with `.cmd` wrappers) for script options.

## Code quality and pre-commit checks

- **Format C++** (run before committing changes under `src/`):

  ```powershell
  .\scripts\format-src.ps1
  ```

  Requires `clang-format` (either in PATH or at the default LLVM location).

- **Check ASCII-only in src/ and tests/** (EzyCad_tests sources; must pass before commits; also enforced in CI):

  ```powershell
  .\scripts\check-nonascii-src.ps1
  # .cmd wrapper also available
  ```

  See the ASCII rule in [docs/ezycad_code_style.md](../../docs/ezycad_code_style.md) and the summary in [agents/conventions/ascii-source.md](../conventions/ascii-source.md).

## Other scripts

- `scripts/ezycad/` — Importable remote client (`pip install -e .` then `import ezycad`). Typed `ezy` / `view` / `sketch` API for IPython completion; see [docs/scripting.md](../../docs/scripting.md#remote-python---listen).
- `scripts/ezycad_remote.py` — CLI wrapper (`python -m ezycad` preferred after install). Smoke: `EzyCad --listen 127.0.0.1:8765`, then `python -c "import ezycad; print(ezycad.connect().view.sketch_count())"`.
- `scripts/align_md_tables.py` — Align GFM pipe tables in `.md` files for source + preview readability (see [conventions/markdown-tables.md](../conventions/markdown-tables.md)).
- `scripts/sync-github-pages-html.ps1` — Sync `web/` changes (EzyCad.html etc.) to the GitHub Pages wasm demo site.
- `scripts/pbf-to-png.ps1` / `.py` — Icon / asset conversion helpers.
- `scripts/pimpl_gen.py` — PIMPL stub generator.
- `scripts/ezycad_graphical_debugging.xml` — Geometry Watch type defs (Graphical Debugging extension). After editing, re-select the path under Tools -> Options -> Graphical Debugging -> General (restart alone does not reload).
- `scripts/build-occt-793-wasm.ps1` — OCCT 7.9.3 wasm (recommended).
- `scripts/build-occt-v8-wasm.ps1` — OCCT 8.0.0.p1 wasm.
- `scripts/build-occt-wasm.ps1` — Shared implementation (advanced `-OcctTag` use).

Agent pointers: [agents/README.md](../README.md). Token budget: [conventions/token-lean.md](../conventions/token-lean.md).
