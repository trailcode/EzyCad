# Development / Local Testing Notes

Short notes for local development and testing tasks.

## Local Documentation Build (Read the Docs / Sphinx)

```bash
pip install -r docs/requirements.txt
sphinx-build -b html -W docs docs/_build
```

Then open `docs/_build/index.html`.

See also [docs/readthedocs.md](../docs/readthedocs.md) for RTD maintenance and conventions.

## Desktop Build (Windows + Visual Studio 2022 / MSVC)

**Prerequisites**
- CMake 3.14+
- Visual Studio 2022 (C++ workload)
- nuget CLI (for GLFW/GLEW)
- OCCT 8.0.0 prebuilts + 3rdparty-vc14-64 (strongly recommended — see [docs/building-occt.md](../docs/building-occt.md))

**Typical configure** (run from repo root):

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DOpenCASCADE_DIR=C:\path\to\occt-8.0.0\cmake `
  -DOCCT_3RD_PARTY_DIR=C:\path\to\3rdparty-vc14-64
```

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

**After CMake structural changes** (e.g. adding new folders for IDE visibility under `agents/`, `docs/`, or `github-workflows`):
- Clean the configure cache: delete `build/CMakeCache.txt`, `build/CMakeFiles/`, and any `_deps/*-subbuild/` directories.
- Re-run the configure step above.

See root [README.md](../README.md#building-instructions) and [docs/building-occt.md](../docs/building-occt.md) for more OCCT details and troubleshooting.

## WebAssembly (Emscripten)

Full OCCT + EzyCad wasm instructions live in [docs/building-occt.md](../docs/building-occt.md#webassembly-emscripten) and the root README.

High-level flow (after `emsdk_env`):

1. Build the OCCT 8.0.0 wasm package (if not already done):

   ```powershell
   .\scripts\build-occt-v8-wasm.ps1 -RootDir C:\bin\occt-wasm-build
   ```

2. Configure + build EzyCad (Ninja generator recommended):

   ```powershell
   mkdir build-em
   cd build-em
   emcmake cmake .. -G Ninja -Wno-dev `
     -DOpenCASCADE_DIR=C:\bin\occt-wasm-build\install\lib\cmake\opencascade `
     -DCMAKE_BUILD_TYPE=Release
   ninja
   ```

3. Serve locally:

   ```powershell
   ninja && python -m http.server 8000
   ```

   Load `EzyCad.html` (or the copy under `web/`) in a browser.

See `scripts/build-occt-v8-wasm.ps1` (and .cmd wrapper) for script options.

## Code Quality & Pre-commit Checks

- **Format C++** (run before committing changes under `src/`):

  ```powershell
  .\scripts\format-src.ps1
  ```

  Requires `clang-format` (either in PATH or at the default LLVM location).

- **Check ASCII-only in src/** (must pass before commits; also enforced in CI):

  ```powershell
  .\scripts\check-nonascii-src.ps1
  # .cmd wrapper also available
  ```

  See the ASCII rule in [docs/ezycad_code_style.md](../docs/ezycad_code_style.md) and the summary in [agents/ezycad-ascii-source.md](ezycad-ascii-source.md).

## Other Scripts

- `scripts/sync-github-pages-html.ps1` — Sync `web/` changes (EzyCad.html etc.) to the GitHub Pages wasm demo site.
- `scripts/pbf-to-png.ps1` / `.py` — Icon / asset conversion helpers.
- `scripts/build-occt-v8-wasm.ps1` — As shown above.

## Working with AI Coding Assistants

When using an AI assistant (Grok Build, Cursor, Claude, Copilot, etc.) on this codebase:

- Start here: `agents/README.md` (table of contents) + this file (`dev.md`).
- Follow the style guides in `docs/ezycad_code_style.md` (C++) and `docs/ezycad_doc_style.md`.
- Check `agents/issues/` and `agents/prs/` for context on specific work items.
- Root-level markers `AGENTS.md` and `agents.md` point AIs at the `agents/` directory.
- Deeper-nested instruction files take precedence.

Users who maintain personal/global instruction sets (e.g. in `C:\agents\` or `~/.grok/`) should also consult those alongside this repo's `agents/` notes.

Update this file as new common workflows or commands appear.