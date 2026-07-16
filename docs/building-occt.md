# Building Open CASCADE (OCCT) for EzyCad

EzyCad does **not** vendor OCCT; builds live **outside** the tree. Point CMake at an install via `OpenCASCADE_DIR` (and on Windows desktop, `OCCT_3RD_PARTY_DIR`).

**Official references**

- [OCCT releases](https://github.com/Open-Cascade-SAS/OCCT/releases)
- [Build OCCT (overview)](https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html)
- Upstream WebGL/wasm samples exist but are outdated; use the V8 script documented below for current EzyCad WASM support.

---

## Version matrix (EzyCad)

| Target              | Documented / tested                                                               | CMake variables                         |
| ------------------- | --------------------------------------------------------------------------------- | --------------------------------------- |
| **Windows desktop** | OCCT **8.0.0** prebuilt                                                           | `OpenCASCADE_DIR`, `OCCT_3RD_PARTY_DIR` |
| **WebAssembly**     | OCCT **7.9.3** (`V7_9_3`) recommended; **8.0.0.p1** (`V8_0_0_p1`) has regressions | `OpenCASCADE_DIR` only (static install) |

---

## Windows desktop (easiest): prebuilt binaries

No OCCT compile. Use **Release** builds (EzyCad is tested with Release OCCT).

### Download (8.0.0 — current README default)

```text
# Combined (OCCT + 3rd-party, simplest layout for desktop):
https://github.com/Open-Cascade-SAS/OCCT/releases/download/V8_0_0/occt-combined-release-no-pch.zip
```

Note: The V8 combined release asset is a small wrapper zip containing `opencascade-8.0.0-vc14-64-combined.zip`. Extracting it yields:

- `opencascade-8.0.0-vc14-64/` (contains `cmake/OpenCASCADEConfig.cmake` → use for `-DOpenCASCADE_DIR=.../opencascade-8.0.0-vc14-64/cmake`)
- `3rdparty-vc14-64/`

```text
# Or separate (if you prefer):
https://github.com/Open-Cascade-SAS/OCCT/releases/download/V8_0_0/3rdparty-vc14-64.zip
# + the opencascade-*-vc14-64 combined from the V8_0_0 release page
```

### Configure EzyCad

Example layout after extracting the combined zip so both folders sit under `C:\bin`:

```text
cmake -S C:\src\EzyCad -B build -G "Visual Studio 18 2026" -A x64 ^
  -DOpenCASCADE_DIR=C:\bin\opencascade-8.0.0-vc14-64\cmake ^
  -DOCCT_3RD_PARTY_DIR=C:\bin\3rdparty-vc14-64
```

- `OpenCASCADE_DIR` — directory containing `OpenCASCADEConfig.cmake` (combined package: `...\opencascade-8.0.0-vc14-64\cmake`; install trees may use `...\lib\cmake\opencascade` — **verify on disk**).
- `OCCT_3RD_PARTY_DIR` — root of `3rdparty-vc14-64` (FreeType, TBB, FFmpeg DLLs copied next to `EzyCad.exe` per `CMakeLists.txt`).

Use **Visual Studio 18 2026** (or another installed VS generator), **x64**, **Release** for the app. After deleting `build/`, re-run configure before `cmake --build`.

---

## Windows desktop: build from source

Only when prebuilts are insufficient (custom flags, Debug OCCT, patches).

1. Install **VS 2026** (C++ workload; VS 2022 also works for CI/source builds), **CMake 3.16+** (4.3+ for the VS 18 generator), **Git**.
2. Download [3rdparty-vc14-64.zip](https://github.com/Open-Cascade-SAS/OCCT/releases) matching the OCCT tag.
3. Clone OCCT, configure with CMake GUI or CLI, `BUILD_LIBRARY_TYPE=Shared` or `Static` as needed, enable `USE_OPENGL`, `USE_FREETYPE`, etc. per [build guide](https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html).
4. Build **INSTALL** target; set `OpenCASCADE_DIR` to the install prefix’s cmake package path.

Prefer **prebuilt** packages unless you need a custom source build.

---

## WebAssembly (Emscripten)

EzyCad’s wasm target links **`TKOpenGles`** (not `TKOpenGl`), static OCCT, and **`-fexceptions`** (see `CMakeLists.txt` Emscripten block).

### Automated wasm scripts

EzyCad links **`TKOpenGles`** (not `TKOpenGl`), static OCCT, and **`-fexceptions`** (see `CMakeLists.txt` Emscripten block).

| Script                                     | OCCT tag                 | Default tree root                         |
| ------------------------------------------ | ------------------------ | ----------------------------------------- |
| `scripts/build-occt-793-wasm.ps1` (`.cmd`) | **V7_9_3** (recommended) | `%USERPROFILE%\occt-wasm-build\V7_9_3`    |
| `scripts/build-occt-v8-wasm.ps1` (`.cmd`)  | V8_0_0_p1                | `%USERPROFILE%\occt-wasm-build\V8_0_0_p1` |

Both wrap the shared implementation `scripts/build-occt-wasm.ps1`. Each version gets its own `src/`, `build/`, and `install/` under the versioned root (flat layout).

**Prerequisites:** Git, CMake 3.16+, [Emscripten emsdk](https://emscripten.org/docs/getting_started/downloads.html) 3.0+ with `emsdk_env` active (`emcc`, `emcmake`, `emmake` on `PATH`).

**PowerShell (repo root, after `emsdk_env`):**

```powershell
# Recommended for wasm (7.9.3)
.\scripts\build-occt-793-wasm.ps1

# OCCT 8.0.0.p1 (for upstream regression testing)
.\scripts\build-occt-v8-wasm.ps1
```

Or use the matching `.cmd` wrappers if PowerShell script execution is restricted.

**Script flags** (both wrappers): `-SkipDownload`, `-SkipFreeType`, `-SkipOcct`, `-ReconfigureOnly`, `-Jobs 8`, `-BuildType Release`, `-RootDir` (override versioned default).

**Advanced:** call `build-occt-wasm.ps1` directly with `-OcctTag` and optional `-VersionDir` (defaults to tag name under `-RootDir` for side-by-side trees under one parent).

**Expect:** 1-3+ hours compile, large disk use. Success prints `OpenCASCADE_DIR=...`.

**Configure EzyCad wasm** (match OCCT version in path and build dir):

```text
emcmake cmake C:\src\EzyCad -Wno-dev -G Ninja -B build-em-7-9-3 ^
  -DOpenCASCADE_DIR=%USERPROFILE%\occt-wasm-build\V7_9_3\install\lib\cmake\opencascade ^
  -DCMAKE_BUILD_TYPE=Release
ninja -C build-em-7-9-3
```

Serve: `python -m http.server 8000` from the build output directory (the `.html` + `.wasm` + `.data` files).

### OCCT wasm CMake flags (reference)

Used by `build-occt-wasm.ps1` — keep in sync if editing the script:

| Variable                                                         | Value          | Why                                    |
| ---------------------------------------------------------------- | -------------- | -------------------------------------- |
| `BUILD_LIBRARY_TYPE`                                             | `Static`       | `.a` archives linked into EzyCad.wasm  |
| `BUILD_MODULE_Draw`                                              | `OFF`          | No Tcl/Tk harness                      |
| `USE_OPENGL`                                                     | `OFF`          | Desktop GL driver                      |
| `USE_GLES2`                                                      | `ON`           | `TKOpenGles` for WebGL2                |
| `USE_VTK`                                                        | `OFF`          | Default in 8.0; avoids GLES VTK issues |
| `USE_FREETYPE`                                                   | `ON`           | Dimension / text rendering             |
| `USE_TBB`, `USE_FFMPEG`, `USE_FREEIMAGE`, `USE_OPENVR`, `USE_TK` | `OFF`          | Reduce deps and build time             |
| `CMAKE_*_FLAGS`                                                  | `-fexceptions` | Match EzyCad emscripten link flags     |

FreeType wasm configure also disables optional zlib/png/harfbuzz finds to simplify the emscripten build.

### Wasm troubleshooting

| Symptom                                                               | Fix                                                                                                                                                                         |
| --------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `running scripts is disabled`                                         | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` **or** use `build-occt-793-wasm.cmd` / `build-occt-v8-wasm.cmd` **or** `powershell -ExecutionPolicy Bypass -File ...` |
| `Can't initialize filter; xz`                                         | Script uses `.tar.gz` for FreeType; delete stale `*.tar.xz` under `src/` and re-run                                                                                         |
| `source directory .../build/freetype does not contain CMakeLists.txt` | Fixed: do not name a PowerShell function parameter `$Args` (shadows automatic `$Args`)                                                                                      |
| `emcc` not found                                                      | Run `emsdk_env.bat` / `emsdk_env.ps1` in the same shell                                                                                                                     |
| EzyCad configure hangs on `find_package(OpenCASCADE)`                 | `emcmake cmake ... --debug-output`; verify `OpenCASCADE_DIR` path                                                                                                           |
| OCCT 8 + ghosted dimension labels                                     | Retest `gui.edge_dim_text_render_mode` (Z-layer Topmost); grid compositing changed in 8.0                                                                                   |
| Shaded faces missing / wireframe-only solids (wasm, OCCT 8.x)         | Use **7.9.3** (`build-occt-793-wasm.ps1`); see [bugs.md](bugs.md)                                                                                                           |

---

**Note:** Desktop builds remain on OCCT **8.0.0**. Wasm currently targets **7.9.3** until OCCT 8.x GLES shading is fixed upstream.

## Wrapper scripts and automation

When adding helper scripts under `scripts/`, follow existing repo conventions.

### Windows `.cmd` (ExecutionPolicy bypass)

`scripts/build-occt-793-wasm.cmd` and `scripts/build-occt-v8-wasm.cmd` wrap the PowerShell scripts (same pattern as `check-nonascii-src.cmd`):

```bat
@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-occt-793-wasm.ps1" %*
if errorlevel 1 exit /b 1
```

**Batch + emsdk:**

```bat
call C:\src\emsdk\emsdk_env.bat
cd /d C:\src\EzyCad
scripts\build-occt-793-wasm.cmd
```

### Unix `.sh`

No first-party wasm shell script yet; a `scripts/build-occt-wasm.sh` may mirror `build-occt-wasm.ps1`:

- `set -euo pipefail`
- Require `emcc`, `git`, `cmake`
- `ROOT="${ROOT:-$HOME/occt-wasm-build/V7_9_3}"`
- Download `freetype-$VER.tar.gz` (gzip portable; avoid `.tar.xz` if `xz` missing)
- `emcmake cmake -S ... -B ...` with the same `-D` flags as the `.ps1`
- `emmake cmake --build ... --target install -j"$(nproc)"`
- Print `OpenCASCADE_DIR=$ROOT/install/lib/cmake/opencascade`

Use forward slashes in cmake paths on Unix.

### PowerShell one-liner (no policy change)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\src\EzyCad\scripts\build-occt-793-wasm.ps1
```

### curl downloads (CI)

**Windows desktop 8.0 combined:**

```bash
curl -L -o occt-combined.zip https://github.com/Open-Cascade-SAS/OCCT/releases/download/V8_0_0/occt-combined-release-no-pch.zip
```

### vcpkg (alternative desktop path)

OCCT 8 supports vcpkg (`USE_VTK=ON` only if needed). EzyCad’s `CMakeLists.txt` is written for `find_package(OpenCASCADE)` + manual `OCCT_3RD_PARTY_DIR` DLL copies — vcpkg integration is **not** wired in-tree.

---

## Maintaining OCCT version pins

| File                                 | What to update                                                                           |
| ------------------------------------ | ---------------------------------------------------------------------------------------- |
| `README.md`                          | Pin version, download URLs, example `OpenCASCADE_DIR`                                    |
| `CMakeLists.txt`                     | `OCCT_3RD_PARTY_DIR` paths (freetype/tbb/ffmpeg versions in `DLLS_COMMON`)               |
| `.github/workflows/windows-msvc.yml` | Prebuilt download URL, cache key, `OpenCASCADE_DIR`/`OCCT_3RD_PARTY_DIR` in CI configure |
| `scripts/build-occt-wasm.ps1`        | Shared wasm build; `$OcctTag`, `$FreeTypeVersion`, cmake flags                           |
| `scripts/build-occt-793-wasm.ps1`    | Wrapper defaulting to `V7_9_3`                                                           |
| `scripts/build-occt-v8-wasm.ps1`     | Wrapper defaulting to `V8_0_0_p1`                                                        |
| `docs/building-occt.md`              | This document                                                                            |
| `src/ply_io.cpp`                     | Comments if triangulation API changes                                                    |

---

## Quick reference: EzyCad CMake variables

| Variable             | Platform        | Purpose                                     |
| -------------------- | --------------- | ------------------------------------------- |
| `OpenCASCADE_DIR`    | All             | Path to `OpenCASCADEConfig.cmake` directory |
| `OCCT_3RD_PARTY_DIR` | Windows desktop | Root of 3rdparty bundle for runtime DLLs    |
| `CMAKE_BUILD_TYPE`   | Native / wasm   | `Release` recommended                       |

EzyCad wasm also sets `TKOpenGles` in `OpenCASCADE_LIBS` when `CMAKE_CXX_COMPILER_ID` is `Emscripten`.
