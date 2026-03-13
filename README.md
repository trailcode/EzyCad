# EzyCad

EzyCad (Easy CAD) is a CAD application for hobbyist machinists to design and edit 2D and 3D models for machining projects. It supports creating precise parts with tools for sketching, extruding, and applying geometric operations, using OpenGL, ImGui, and Open CASCADE Technology (OCCT). Export models to formats like STEP or STL for CNC machines or 3D printers, and try it in your browser with the WebAssembly version.

[Try EzyCad in your browser! (WebAssembly version)](https://trailcode.github.io/EzyCad/EzyCad.html)

## Features
- 2D and 3D modeling capabilities.
- Integration with Open CASCADE for geometric operations.
- Cross-platform support with Emscripten for WebAssembly builds.
- Interactive GUI built with ImGui.

## Usage Guide
See [usage.md](usage.md) in the repository for a detailed guide on using EzyCad. It covers the user interface, modeling tools (e.g., sketching, extrusions, Boolean operations), keyboard shortcuts, and view controls. Example workflows, such as creating 2D sketches and 3D shapes, are included to help you get started.

## Building Instructions

### Prerequisites
Ensure the following dependencies are installed:
- CMake (minimum version 3.14.0)
- Nuget command line utility
- A C++ 20 compatible compiler (e.g., MSVC, GCC, or Clang)
- OpenGL development libraries
- Open CASCADE Technology (OCCT 7.9.1) https://github.com/Open-Cascade-SAS/OCCT

### OCCT Build

#### For VS2022
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
- ImGui in `third_party` has been modified as explained in: https://github.com/ocornut/imgui/issues/7519#issuecomment-2629628233 to improve font rendering.

### Artwork
- Icons from: https://wiki.freecad.org/Artwork

## Support and Contributions
- Report issues or suggest features on the GitHub repository.
- Contribute by developing features and fixing bugs. Pull requests are welcome! 💪
- Additional resources, including video tutorials and online documentation, are linked in [usage.md](usage.md).

### We need development help
EzyCad is maintained by a small team and we would love more contributors. If you can help with features, bug fixes, documentation, or testing—please jump in. Every contribution helps move the project forward.

**Code style:** When contributing, please follow the project’s style guide: [ezycad-style.md](ezycad-style.md). Both human developers and AI coding agents (e.g. Cursor, GitHub Copilot, ChatGPT) should adhere to it so that patches stay consistent and reviewable.
