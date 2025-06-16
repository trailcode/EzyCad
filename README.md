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
- Install Emscripten and activate its environment.
- Build FreeType (2.10.1) for Emscripten using the instructions: https://stackoverflow.com/questions/61049517/build-latest-freetype-with-emscripten
  - Add exception support:
    - `emcmake cmake .. -DCMAKE_CXX_FLAGS="-fexceptions" -DCMAKE_EXE_LINKER_FLAGS="-fexceptions" -DCMAKE_INSTALL_PREFIX=c:/src/freetype-2.10.1_em_install -DCMAKE_POLICY_VERSION_MINIMUM=3.5`
- Build OCCT (7.9.0) for Emscripten using the instructions: https://github.com/mathysyon/wasm-occ-demo
  - Add exception support:
    - CMAKE_CXX_FLAGS -fexceptions
    - CMAKE_EXE_LINKER_FLAGS -fexceptions
- Configure the EzyCad project with Emscripten:
  - `mkdir build_em`
  - `emcmake cmake .. -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade`
  - If you have Ninja:
    - Debug:
      1. `emcmake cmake .. -G Ninja -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade -CMAKE_BUILD_TYPE=Debug`
      2. `ninja`
      3. Approximately 50MB `EzyCad.wasm` file.
    - Release:
      1. `emcmake cmake .. -G Ninja -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade -CMAKE_BUILD_TYPE=Release`
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
- Additional resources, including video tutorials and online documentation, are linked in [usage.md](usage.md).
