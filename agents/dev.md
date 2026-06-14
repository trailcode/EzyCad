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

- **Check ASCII-only in src/ and tests/** (EzyCad_tests sources; must pass before commits; also enforced in CI):

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
- When you add or change user-visible functionality (UI, settings, workflows, shortcuts, persisted keys), sync user docs per [user-docs-sync.md](user-docs-sync.md) (same branch/PR as the code).
- Check `agents/issues/` and `agents/prs/` for context on specific work items.
- Root-level markers `AGENTS.md` and `agents.md` point AIs at the `agents/` directory.
- Deeper-nested instruction files take precedence.

Users who maintain personal/global instruction sets (e.g. in `C:\agents\` or `~/.grok/`) should also consult those alongside this repo's `agents/` notes.

Update this file as new common workflows or commands appear.

## Releasing a New Version (x.y.z Semantic Versioning)

See also the comment in `src/version.h`, the project declaration in `CHANGELOG.md`, and `.github/workflows/windows-msvc.yml`.

1. Ensure all user-visible changes (UI, shortcuts, Options/Settings, docs, persisted data) have corresponding updates (follow `agents/user-docs-sync.md`).
2. In `CHANGELOG.md`:
   - Move the content from the top `## [Unreleased]` section into a new dated release section: `## [X.Y.Z] - YYYY-MM-DD`.
   - Leave a fresh empty `## [Unreleased]` at the very top.
   - Update the bottom reference links (`[Unreleased]` compare URL and the new `[X.Y.Z]` tag link).
3. Bump the version in `src/version.h`:
   ```c
   #define EZYCAD_VERSION_MAJOR x
   #define EZYCAD_VERSION_MINOR y
   #define EZYCAD_VERSION_PATCH z
   #define EZYCAD_VERSION_STRING "x.y.z"
   ```
   (Keep MAJOR/MINOR/PATCH decisions per [SemVer](https://semver.org/): MAJOR for breaking, MINOR for new backwards-compatible features, PATCH for fixes.)
4. Verify the version is visible to users:
   - Window title should read `EzyCad X.Y.Z - <filename or untitled>`.
   - Help > About should start with a bold **EzyCad X.Y.Z** header (pulled from `EZYCAD_VERSION_STRING`).
5. Run pre-commit checks:
   - `.\scripts\format-src.ps1`
   - `.\scripts\check-nonascii-src.ps1`
6. Commit the release prep changes (version bump + changelog + any doc tweaks).
7. **Create and push only the annotated tag** (this is the trigger):
   ```powershell
   git tag -a vX.Y.Z -m "EzyCad X.Y.Z"
   git push origin main --tags
   ```
   **Do not** pre-create the GitHub Release with `gh release create`. The CI will create/update the release and attach assets.
8. Monitor the build: go to the **Actions** tab and watch the "Windows MSVC Build and Test" workflow run for your tag.
   - The job runs on GitHub's windows-2022 runner (full VS2022 + OCCT prebuilt + vcpkg).
   - It builds Release, runs tests, and packages artifacts.
9. When the tag build succeeds:
   - The GitHub Release page will automatically get:
     - The portable user download: `EzyCad-X.Y.Z-windows-x64.zip` (self-contained, top-level `EzyCad/` folder, tests exe excluded). This is attached directly to the release assets.
     - Release notes (from the action + you can edit to paste the detailed dated section from CHANGELOG).
   - Additionally, under the specific workflow run → "Artifacts", you will find:
     - `EzyCad-X.Y.Z-windows-x64` (same portable zip).
     - `EzyCad-Windows-MSVC-Release` (raw `build/Release/` tree containing EzyCad.exe + all DLLs + res/ + everything from the MSVC build; useful for "build" inspection or advanced users). This is **not** automatically a direct release asset; download it from the Actions artifacts list.
10. (Optional) If you want the raw MSVC build tree as a direct `.zip` asset on the Releases page too, the workflow can be extended (see the packaging step). For now the portable x64 zip is the primary end-user download.
11. **Rebuild and publish the public WebAssembly demo** (critical after any C++ change or release):
    - Activate Emscripten: run the emsdk_env (see docs/building-occt.md).
    - (Re)build OCCT wasm if needed: `.\scripts\build-occt-v8-wasm.ps1 -RootDir C:\bin\occt-wasm-build`
    - Configure + build (from repo root):
      ```powershell
      # Use a dir like build-em (or clean it)
      emcmake cmake -S . -B build-em -G Ninja -Wno-dev `
        -DOpenCASCADE_DIR=C:\bin\occt-wasm-build\install\lib\cmake\opencascade `
        -DCMAKE_BUILD_TYPE=Release
      ninja -C build-em
      ```
    - This produces (in build-em/): `EzyCad.html`, `EzyCad.js`, `EzyCad.wasm`, `EzyCad.data`.
    - **Bump cache** (already done in this release prep for 0.2.0): edit `web/EzyCad.html` (var EZYCAD_WEB_CACHE) — the build-em/ copy will be updated on next full build.
    - Copy the four files above into your local clone of `trailcode/trailcode.github.io/EzyCad/`.
    - Sync the HTMLs: `.\scripts\sync-github-pages-html.ps1 -PagesRepo C:\src\trailcode.github.io` (updates index.html + EzyCad.html with the new cache value).
    - In the pages repo: `git add EzyCad/ ; git commit -m "Update wasm demo to vX.Y.Z" ; git push`
    - The public demo (https://trailcode.github.io/EzyCad/EzyCad.html) now serves the new build (cache bust ensures clients get the fresh .wasm/.data).
    - Test locally first with `python -m http.server` from the build-em dir.

On `0.x` versions (current series), minor bumps (`0.1.0` → `0.2.0`) are still allowed to contain breaking changes because the public API is not yet declared stable.

**Why the "build msvc download link" appears later:** The MSVC (and portable) artifacts are produced by CI after the (slow) build completes on the GitHub runner. Refresh the Releases page and the Actions run after the workflow finishes. The tag push is what starts everything.