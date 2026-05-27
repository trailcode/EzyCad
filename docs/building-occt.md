# Building Open CASCADE (OCCT) for EzyCad

EzyCad does **not** vendor OCCT; builds live **outside** the tree. Point CMake at an install via `OpenCASCADE_DIR` (and on Windows desktop, `OCCT_3RD_PARTY_DIR`).

**Official references**

- [OCCT releases](https://github.com/Open-Cascade-SAS/OCCT/releases)
- [Build OCCT (overview)](https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html)
- [WebGL / wasm sample (upstream)](https://dev.opencascade.org/doc/occt-7.9.0/overview/html/occt_samples_webgl.html)

---

## Version matrix (EzyCad)

| Target | Documented / tested | Experimental | CMake variables |
| --- | --- | --- | --- |
| **Windows desktop** | OCCT **7.9.1** prebuilt | OCCT **8.0.0** prebuilt | `OpenCASCADE_DIR`, `OCCT_3RD_PARTY_DIR` |
| **WebAssembly** | OCCT **7.9.x** manual / [wasm-occ-demo](https://github.com/mathysyon/wasm-occ-demo) | OCCT **8.0.0** via `scripts/build-occt-v8-wasm.ps1` | `OpenCASCADE_DIR` only (static install) |

After upgrading OCCT, retest **sketch dimensions**, **grid**, **fillet/chamfer/boolean**, and **STEP/STL** export. OCCT 8.0 changes grid rendering and many modeling paths.

---

## Windows desktop (easiest): prebuilt binaries

No OCCT compile. Use **Release** builds (EzyCad is tested with Release OCCT).

### Download (7.9.1 — current README default)

```text
# Combined (OCCT + 3rd-party, simplest layout):
https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_1/opencascade-7.9.1-vc14-64-combined.zip

# Or separate:
https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_1/3rdparty-vc14-64.zip
# + a 7.9.1 opencascade-*-vc14-64 package from the V7_9_1 release page
```

### Download (8.0.0 — upgrade path)

```text
https://github.com/Open-Cascade-SAS/OCCT/releases/download/V8_0_0/occt-combined-release-no-pch.zip
https://github.com/Open-Cascade-SAS/OCCT/releases/download/V8_0_0/3rdparty-vc14-64.zip
```

Unpack so `3rdparty-vc14-64` and the OCCT folder share a **parent directory** (required for DRAW; EzyCad only needs CMake paths).

### Configure EzyCad

```text
cmake C:\src\EzyCad -DOpenCASCADE_DIR=C:\bin\OCCT-install\cmake -DOCCT_3RD_PARTY_DIR=C:\bin\3rdparty-vc14-64
```

- `OpenCASCADE_DIR` — directory containing `OpenCASCADEConfig.cmake` (often `...\cmake` or `...\lib\cmake\opencascade` depending on package layout; **verify on disk**).
- `OCCT_3RD_PARTY_DIR` — root of `3rdparty-vc14-64` (FreeType, TBB, FFmpeg DLLs copied next to `EzyCad.exe` per `CMakeLists.txt`).

Use **Visual Studio** generator, **x64**, **Release** for the app.

---

## Windows desktop: build from source

Only when prebuilts are insufficient (custom flags, Debug OCCT, patches).

1. Install **VS 2022** (C++), **CMake 3.16+**, **Git**.
2. Download [3rdparty-vc14-64.zip](https://github.com/Open-Cascade-SAS/OCCT/releases) matching the OCCT tag.
3. Clone OCCT, configure with CMake GUI or CLI, `BUILD_LIBRARY_TYPE=Shared` or `Static` as needed, enable `USE_OPENGL`, `USE_FREETYPE`, etc. per [build guide](https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html).
4. Build **INSTALL** target; set `OpenCASCADE_DIR` to the install prefix’s cmake package path.

Prefer **prebuilt** packages unless you need a custom source build.

---

## WebAssembly (Emscripten)

EzyCad’s wasm target links **`TKOpenGles`** (not `TKOpenGl`), static OCCT, and **`-fexceptions`** (see `CMakeLists.txt` Emscripten block).

### Automated: `scripts/build-occt-v8-wasm.ps1`

Builds **FreeType 2.13.3** + **OCCT `V8_0_0`** static with GLES2 into a single install prefix.

**Prerequisites:** Git, CMake 3.16+, [Emscripten emsdk](https://emscripten.org/docs/getting_started/downloads.html) 3.0+ with `emsdk_env` active (`emcc`, `emcmake`, `emmake` on `PATH`).

**Layout created** (default `RootDir` = `%USERPROFILE%\occt-wasm-build`):

```text
occt-wasm-build/
  src/OCCT/                    git clone V8_0_0
  src/freetype-2.13.3/         from .tar.gz (not .tar.xz on Windows)
  build/freetype/
  build/occt/
  install/                     CMAKE_INSTALL_PREFIX
    lib/cmake/opencascade/     → OpenCASCADE_DIR
    freetype/                  FreeType install (used at OCCT configure time)
```

**PowerShell (repo root, after `emsdk_env`):**

```powershell
.\scripts\build-occt-v8-wasm.ps1 -RootDir C:\src\occt-wasm-build
```

Or use `scripts\build-occt-v8-wasm.cmd` if PowerShell script execution is restricted.

**Script flags:** `-SkipDownload`, `-SkipFreeType`, `-SkipOcct`, `-ReconfigureOnly`, `-Jobs 8`, `-OcctTag V8_0_0`, `-BuildType Release`.

**Expect:** 1–3+ hours compile, large disk use. Success prints `OpenCASCADE_DIR=...`.

**Configure EzyCad wasm** (do **not** use `-G Ninja` for EzyCad — known issue in README):

```text
mkdir build_em
cd build_em
emcmake cmake C:\src\EzyCad -Wno-dev -DOpenCASCADE_DIR=C:\src\occt-wasm-build\install\lib\cmake\opencascade
emmake cmake --build . --config Release
```

Serve: `python -m http.server 8000` from the build output directory.

### OCCT wasm CMake flags (reference)

Used by `build-occt-v8-wasm.ps1` — keep in sync if editing the script:

| Variable | Value | Why |
| --- | --- | --- |
| `BUILD_LIBRARY_TYPE` | `Static` | `.a` archives linked into EzyCad.wasm |
| `BUILD_MODULE_Draw` | `OFF` | No Tcl/Tk harness |
| `USE_OPENGL` | `OFF` | Desktop GL driver |
| `USE_GLES2` | `ON` | `TKOpenGles` for WebGL2 |
| `USE_VTK` | `OFF` | Default in 8.0; avoids GLES VTK issues |
| `USE_FREETYPE` | `ON` | Dimension / text rendering |
| `USE_TBB`, `USE_FFMPEG`, `USE_FREEIMAGE`, `USE_OPENVR`, `USE_TK` | `OFF` | Reduce deps and build time |
| `CMAKE_*_FLAGS` | `-fexceptions` | Match EzyCad emscripten link flags |

FreeType wasm configure also disables optional zlib/png/harfbuzz finds to simplify the emscripten build.

### Wasm troubleshooting

| Symptom | Fix |
| --- | --- |
| `running scripts is disabled` | `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` **or** use `build-occt-v8-wasm.cmd` **or** `powershell -ExecutionPolicy Bypass -File ...` |
| `Can't initialize filter; xz` | Script uses `.tar.gz` for FreeType; delete stale `*.tar.xz` under `src/` and re-run |
| `source directory .../build/freetype does not contain CMakeLists.txt` | Fixed: do not name a PowerShell function parameter `$Args` (shadows automatic `$Args`) |
| `emcc` not found | Run `emsdk_env.bat` / `emsdk_env.ps1` in the same shell |
| EzyCad configure hangs on `find_package(OpenCASCADE)` | `emcmake cmake ... --debug-output`; verify `OpenCASCADE_DIR` path |
| OCCT 8 + ghosted dimension labels | Retest `gui.edge_dim_text_render_mode` (Z-layer Topmost); grid compositing changed in 8.0 |

### Legacy wasm path (7.9.x)

Manual FreeType 2.10.1 + OCCT 7.9.0 per [wasm-occ-demo](https://github.com/mathysyon/wasm-occ-demo). Prefer the V8 script for new wasm OCCT work; keep 7.9.x until EzyCad wasm is verified on 8.0.

---

## Wrapper scripts and automation

When adding helper scripts under `scripts/`, follow existing repo conventions.

### Windows `.cmd` (ExecutionPolicy bypass)

`scripts/build-occt-v8-wasm.cmd` wraps the PowerShell script (same pattern as `check-nonascii-src.cmd`):

```bat
@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-occt-v8-wasm.ps1" %*
if errorlevel 1 exit /b 1
```

**Batch + emsdk:**

```bat
call C:\src\emsdk\emsdk_env.bat
cd /d C:\src\EzyCad
scripts\build-occt-v8-wasm.cmd -RootDir C:\src\occt-wasm-build
```

### Unix `.sh`

No first-party wasm shell script yet; a `scripts/build-occt-v8-wasm.sh` may mirror the PowerShell logic:

- `set -euo pipefail`
- Require `emcc`, `git`, `cmake`
- `ROOT="${ROOT:-$HOME/occt-wasm-build}"`
- Download `freetype-$VER.tar.gz` (gzip portable; avoid `.tar.xz` if `xz` missing)
- `emcmake cmake -S ... -B ...` with the same `-D` flags as the `.ps1`
- `emmake cmake --build ... --target install -j"$(nproc)"`
- Print `OpenCASCADE_DIR=$ROOT/install/lib/cmake/opencascade`

Use forward slashes in cmake paths on Unix.

### PowerShell one-liner (no policy change)

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File C:\src\EzyCad\scripts\build-occt-v8-wasm.ps1 -RootDir C:\src\occt-wasm-build
```

### curl downloads (CI)

**Windows desktop 8.0 combined:**

```bash
curl -L -o occt-combined.zip https://github.com/Open-Cascade-SAS/OCCT/releases/download/V8_0_0/occt-combined-release-no-pch.zip
```

**Windows desktop 7.9.1 3rdparty:**

```bash
curl -L -o 3rdparty.zip https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_1/3rdparty-vc14-64.zip
```

### vcpkg (alternative desktop path)

OCCT 8 supports vcpkg (`USE_VTK=ON` only if needed). EzyCad’s `CMakeLists.txt` is written for `find_package(OpenCASCADE)` + manual `OCCT_3RD_PARTY_DIR` DLL copies — vcpkg integration is **not** wired in-tree.

---

## Maintaining OCCT version pins

| File | What to update |
| --- | --- |
| `README.md` | Pin version, download URLs, example `OpenCASCADE_DIR` |
| `CMakeLists.txt` | `OCCT_3RD_PARTY_DIR` paths (freetype/tbb/ffmpeg versions in `DLLS_COMMON`) |
| `scripts/build-occt-v8-wasm.ps1` | `$OcctTag`, `$FreeTypeVersion`, cmake flags |
| `docs/building-occt.md` | This document |
| `src/ply_io.cpp` | Comments if triangulation API changes |

---

## Quick reference: EzyCad CMake variables

| Variable | Platform | Purpose |
| --- | --- | --- |
| `OpenCASCADE_DIR` | All | Path to `OpenCASCADEConfig.cmake` directory |
| `OCCT_3RD_PARTY_DIR` | Windows desktop | Root of 3rdparty bundle for runtime DLLs |
| `CMAKE_BUILD_TYPE` | Native / wasm | `Release` recommended |

EzyCad wasm also sets `TKOpenGles` in `OpenCASCADE_LIBS` when `CMAKE_CXX_COMPILER_ID` is `Emscripten`.
