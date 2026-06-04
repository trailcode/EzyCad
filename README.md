# EzyCad

[![GitHub](https://img.shields.io/github/stars/trailcode/EzyCad?style=social)](https://github.com/trailcode/EzyCad)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/trailcode/EzyCad)](https://github.com/trailcode/EzyCad/releases/latest)
[![Documentation](https://img.shields.io/badge/docs-readthedocs-blue)](https://ezycad.readthedocs.io/en/latest/usage.html)
[![WebAssembly](https://img.shields.io/badge/run_in-browser-WebAssembly-green)](https://trailcode.github.io/EzyCad/EzyCad.html)

**Repository:** [https://github.com/trailcode/EzyCad](https://github.com/trailcode/EzyCad)

![EzyCad splash screen](doc/gen/AI-gen-splashscreen_05_01_2026_512.png)

EzyCad (Easy CAD) is an open-source CAD application for hobbyist machinists to design and edit 2D and 3D models for machining projects. It supports creating precise parts with tools for sketching, extruding, and applying geometric operations, using OpenGL, Dear ImGui, and Open CASCADE Technology (OCCT). Export models to formats like STEP or STL for CNC machines or 3D printers, or [run EzyCad in your browser (WebAssembly)](https://trailcode.github.io/EzyCad/EzyCad.html). Project home: [trailcode.github.io/EzyCad](https://trailcode.github.io/EzyCad/).

> **Not EZCAD laser software:** [EzyCad](https://github.com/trailcode/EzyCad) (with a **y**) is hobbyist mechanical CAD built on OCCT — unrelated to EZCAD2/EZCAD3 laser marking products.

## Downloads

Prebuilt **Windows** binaries (portable .zip containing `EzyCad.exe` + all required DLLs and assets) are published on the [GitHub Releases page](https://github.com/trailcode/EzyCad/releases).

- Go to [Releases](https://github.com/trailcode/EzyCad/releases) → expand "Assets" under the latest version.
- Download `EzyCad-vX.Y.Z-windows-x64.zip` (or the equivalent name), unzip, and run `EzyCad.exe`.
- No installer is required; the zip is self-contained.

This is the standard approach used by the vast majority of GitHub projects (Godot, FreeCAD, OBS, Notepad++, etc.) for making binaries easily discoverable and downloadable by end users without requiring them to build from source or navigate CI artifacts.

**WebAssembly (browser) version:** [Run EzyCad directly in your browser](https://trailcode.github.io/EzyCad/EzyCad.html) (no download needed).

For source builds or other platforms, see the [Building Instructions](#building-instructions) below.

## Features
- 2D and 3D modeling capabilities.
- Integration with Open CASCADE for geometric operations.
- Cross-platform support with Emscripten for WebAssembly builds.
- Interactive GUI built with ImGui.

## Usage Guide
**Online:** [ezycad.readthedocs.io](https://ezycad.readthedocs.io/en/latest/usage.html) (built from this repo on [Read the Docs](https://readthedocs.org/)).

**Source:** [usage.md](usage.md) and related guides (`usage-sketch.md`, `usage-settings.md`, etc.) at the repository root. They cover the user interface, modeling tools, keyboard shortcuts, and view controls.

## Changelog
Release history is in [CHANGELOG.md](CHANGELOG.md) (repository root, next to this file and [usage.md](usage.md)). With the CMake Visual Studio generator, those files also appear under a Documentation folder on the EzyCad target in Solution Explorer.

## Building Instructions

### Prerequisites
Ensure the following dependencies are installed:
- CMake (minimum version 3.14.0)
- Nuget command line utility
- A C++ 20 compatible compiler (e.g., MSVC, GCC, or Clang)
- OpenGL development libraries
- Open CASCADE Technology (OCCT 7.9.1) https://github.com/Open-Cascade-SAS/OCCT

### OCCT Build

Full guide: **[docs/building-occt.md](docs/building-occt.md)** (Windows prebuilts, wasm/Emscripten, troubleshooting).

#### For VS2022 (summary)
- See: https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html
- OCCT 3rd-party binaries: https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_1/3rdparty-vc14-64.zip
- Currently building EzyCad has only been tested with the Release build of OCCT.
- Or download pre-built binaries: https://github.com/Open-Cascade-SAS/OCCT/releases/tag/V7_9_1

### Steps to Build
1. Clone the repository.
2. Create a build directory, e.g., `C:\src\EzyCad_build`.
3. Configure the project in the build directory using CMake:
   - E.g., `cmake C:\src\EzyCad -DOpenCASCADE_DIR=C:\bin\OCCT-7_9_1_install\cmake -DOCCT_3RD_PARTY_DIR=C:\bin\3rdparty-vc14-64`
   - `OCCT_3RD_PARTY_DIR` should point to the OCCT 3rd-party distribution.
   - CMake will use `nuget` to download additional dependencies.
4. Build the project.

### Notes for Windows Users
- Ensure `nuget` is installed for fetching dependencies like GLFW, GLEW, and Boost.
- Use Visual Studio as the IDE for debugging and building.

### Notes for Emscripten Builds
- **Known issue:** The Emscripten build with the Ninja generator (`-G Ninja`) is currently not working. Use the default generator (e.g. `emcmake cmake ..` without `-G Ninja`, then `emmake make`) or another generator that works with your Emscripten setup.
- Install Emscripten and activate its environment.
- **OCCT 8.0.0 for wasm:** `scripts\build-occt-v8-wasm.cmd` or `.\scripts\build-occt-v8-wasm.ps1` after `emsdk_env` — see [docs/building-occt.md](docs/building-occt.md#webassembly-emscripten). Experimental; manual 7.9.x steps below remain for reference.
- Build FreeType (2.10.1) for Emscripten using the instructions: https://stackoverflow.com/questions/61049517/build-latest-freetype-with-emscripten
  - Add exception support:
    - `emcmake cmake .. -DCMAKE_CXX_FLAGS="-fexceptions" -DCMAKE_EXE_LINKER_FLAGS="-fexceptions" -DCMAKE_INSTALL_PREFIX=c:/src/freetype-2.10.1_em_install -DCMAKE_POLICY_VERSION_MINIMUM=3.5`
- Build OCCT (7.9.0) for Emscripten using the instructions: https://github.com/mathysyon/wasm-occ-demo
  - Add exception support:
    - CMAKE_CXX_FLAGS -fexceptions
    - CMAKE_EXE_LINKER_FLAGS -fexceptions
- Configure the EzyCad project with Emscripten:
  - `mkdir build_em` then `cd build_em`
  - Add **-Wno-dev** to suppress any remaining CMake developer warnings. (CMP0167/FindBoost is already handled in CMakeLists.txt.)  
    `emcmake cmake .. -Wno-dev -DOpenCASCADE_DIR=C:/src/OCCT-7_9_0_em_install/lib/cmake/opencascade`
  - If configure **freezes** after that warning, the hang is often in `find_package(OpenCASCADE)` or Emscripten compiler detection. Run with `--debug-output` to see where it stops, e.g.:  
    `emcmake cmake .. -Wno-dev -DOpenCASCADE_DIR=C:/src/OCCT-7_9_0_em_install/lib/cmake/opencascade --debug-output`
  - If you have Ninja (see known issue above; may not work):
    - Debug:
      1. `emcmake cmake .. -Wno-dev -G Ninja -DOpenCASCADE_DIR=C:/src/OCCT-7_9_0_em_install/lib/cmake/opencascade -DCMAKE_BUILD_TYPE=Debug`
      2. `ninja`
      3. Approximately 50MB `EzyCad.wasm` file.
    - Release:
      1. `emcmake cmake .. -Wno-dev -G Ninja -DOpenCASCADE_DIR=C:/src/OCCT-7_9_0_em_install/lib/cmake/opencascade -DCMAKE_BUILD_TYPE=Release`
      2. `ninja`
      3. Approximately 19MB `EzyCad.wasm` file.
- Build the project.
- Serve the WebAssembly: `python.exe -m http.server 8000`
- Or build and serve: `ninja && python.exe -m http.server 8000`
- **GitHub Pages HTML:** After changing `web/index.html` or `web/EzyCad.html`, sync to [trailcode.github.io](https://github.com/trailcode/trailcode.github.io) with `scripts/sync-github-pages-html.ps1` (see script header).
- Dear ImGui under `third_party/imgui/` carries EzyCad-specific changes (font rendering); see [In-tree third-party libraries](#in-tree-third-party-libraries) at the end of this README.

### Artwork
- Icons from: https://wiki.freecad.org/Artwork

## Support and Contributions
- **Project home:** [trailcode.github.io/EzyCad](https://trailcode.github.io/EzyCad/)
- Report issues or suggest features on the [GitHub repository](https://github.com/trailcode/EzyCad).
- Contribute by developing features and fixing bugs. Pull requests are welcome!
- Additional resources, including video tutorials and online documentation, are linked in [usage.md](usage.md).
- Outreach draft posts (forums, Reddit, awesome lists): [agents/discoverability-outreach.md](agents/discoverability-outreach.md).

### We need development help
EzyCad is maintained by a small team and we would love more contributors. If you can help with features, bug fixes, documentation, or testing - please jump in. Every contribution helps move the project forward.

**Style guides:** [ezycad_code_style.md](ezycad_code_style.md) for C++ in `src/`; [ezycad_doc_style.md](ezycad_doc_style.md) for user guides and [Read the Docs](https://ezycad.readthedocs.io/). Both human developers and AI coding agents should follow the relevant guide. Optional assistant-oriented snippets live under [agents/](agents/) (the repo does not commit `.cursor/`).

## In-tree third-party libraries

**Open CASCADE (OCCT) is not vendored here.** You build or install OCCT (and its binary redistributables) **outside** this tree and pass `OpenCASCADE_DIR` / `OCCT_3RD_PARTY_DIR` into CMake, as in [Building Instructions](#building-instructions) above.

The **`third_party/`** folder holds other libraries **shipped inside the EzyCad repository** (typically committed as a vendored snapshot, not fetched by CMake except where noted):

| Component | Location | Role |
| --- | --- | --- |
| **Dear ImGui** | `third_party/imgui/` | Immediate-mode UI used by the application. This tree includes **project-specific changes** for font rendering; see [imgui#7519 (comment)](https://github.com/ocornut/imgui/issues/7519#issuecomment-2629628233). |
| **nlohmann/json** | `third_party/json/` (headers under `include/`) | JSON used by the project; CMake adds `third_party/json/include`. |
| **tinyfiledialogs** | `third_party/tinyfiledialogs/` | Small C helper for native file dialogs on desktop. |
| **ImGuiColorTextEdit** | `third_party/ImGuiColorTextEdit/` | Syntax-highlighted editor widget for the **Lua** and **Python** script consoles ([upstream](https://github.com/BalazsJako/ImGuiColorTextEdit)). |

**ImGuiColorTextEdit:** Prefer a full checkout under `third_party/ImGuiColorTextEdit/` (see `third_party/README.md`). If that folder is missing, CMake **FetchContent** downloads upstream at a **fixed commit** (`ca2f9f1462e3b60e56351bc466acda448c5ea50d`) because the upstream repo has **no release tags**. To upgrade the editor, bump that SHA in `CMakeLists.txt` and refresh any vendored copy.

**Windows note:** GLFW, GLEW, and Boost for MSVC are **not** stored under `third_party/`; NuGet installs them into **`${CMAKE_BINARY_DIR}/thirdParty`** when you configure (see [Notes for Windows Users](#notes-for-windows-users)).
