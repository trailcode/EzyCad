# Releasing a new version (x.y.z semantic versioning)

See also the comment in `src/version.h`, the project declaration in `CHANGELOG.md`, and `.github/workflows/windows-msvc.yml`.

1. Ensure all user-visible changes (UI, shortcuts, Options/Settings, docs, persisted data) have corresponding updates (follow [agents/conventions/user-docs-sync.md](../conventions/user-docs-sync.md)).
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
    - Activate Emscripten: run the emsdk_env (see [docs/building-occt.md](../../docs/building-occt.md)).
    - (Re)build OCCT wasm if needed: `.\scripts\build-occt-793-wasm.ps1`
    - Configure + build (from repo root):
      ```powershell
      emcmake cmake -S . -B build-em-7-9-3 -G Ninja -Wno-dev `
        -DOpenCASCADE_DIR=$env:USERPROFILE\occt-wasm-build\V7_9_3\install\lib\cmake\opencascade `
        -DCMAKE_BUILD_TYPE=Release
      ninja -C build-em-7-9-3
      ```
    - This produces (in build-em/): `EzyCad.html`, `EzyCad.js`, `EzyCad.wasm`, `EzyCad.data`.
    - **Bump cache:** edit `web/EzyCad.html` (var EZYCAD_WEB_CACHE) — the build-em/ copy will be updated on next full build.
    - Copy the four files above into your local clone of `trailcode/trailcode.github.io/EzyCad/`.
    - Sync the HTMLs: `.\scripts\sync-github-pages-html.ps1 -PagesRepo C:\src\trailcode.github.io` (updates index.html + EzyCad.html with the new cache value).
    - In the pages repo: `git add EzyCad/ ; git commit -m "Update wasm demo to vX.Y.Z" ; git push`
    - The public demo (https://trailcode.github.io/EzyCad/EzyCad.html) now serves the new build (cache bust ensures clients get the fresh .wasm/.data).
    - Test locally first with `python -m http.server` from the build-em dir.

On `0.x` versions (current series), minor bumps (`0.1.0` → `0.2.0`) are still allowed to contain breaking changes because the public API is not yet declared stable.

**Why the "build msvc download link" appears later:** The MSVC (and portable) artifacts are produced by CI after the (slow) build completes on the GitHub runner. Refresh the Releases page and the Actions run after the workflow finishes. The tag push is what starts everything.
