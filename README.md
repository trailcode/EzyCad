[🚀 Try EzyCad in your browser! (WebAssembly version) 🚀](https://trailcode.github.io/EzyCad/EzyCad.html)

# EzyCad  

EzyCad (Easy CAD) is a modern CAD (Computer-Aided Design) application designed for
creating and manipulating 2D and 3D models. It leverages OpenGL, ImGui,
and Open CASCADE Technology (OCCT) to provide a robust and interactive
user interface for CAD operations.  

## Features  
- 2D and 3D modeling capabilities.  
- Integration with Open CASCADE for advanced geometric operations.  
- Cross-platform support with Emscripten for WebAssembly builds.  
- Interactive GUI built with ImGui.  

## Building Instructions  

### Prerequisites  
Ensure the following dependencies are installed:  
- CMake (minimum version 3.14.0)
- Nuget command line utility
- A C++20 compatible compiler (e.g., MSVC, GCC, or Clang)  
- OpenGL development libraries  
- Open CASCADE Technology (OCCT 7.9.1) https://github.com/Open-Cascade-SAS/OCCT

### OCCT Build

#### For VS2022
* See: https://dev.opencascade.org/doc/overview/html/build_upgrade__building_occt.html
* OCCT 3rd-party binaries: https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_1/3rdparty-vc14-64.zip
* Currently building EzyCad has only been tested with the Release build of OCCT.
* Or download pre-build binaries: https://github.com/Open-Cascade-SAS/OCCT/releases/tag/V7_9_1

### Steps to Build  
1. Clone the repository:
2. Create a build directory:
    E.G. `C:\src\EzyCad_build`
3. Configure the project in the build directory using CMake:
    - E.G. `cmake C:\src\EzyCad -DOpenCASCADE_DIR=C:\bin\OCCT-7_9_1_install\cmake -DOCCT_3RD_PARTY_DIR=C:\bin\3rdparty-vc14-64`
    - `OCCT_3RD_PARTY_DIR` should point to the OCCT 3rdparty distribution.
    - Cmake will use `nuget` to download additional dependices.
4. Build the project

### Notes for Windows Users  
- Ensure `nuget` is installed for fetching dependencies like GLFW, GLEW, and Boost.  
- Use Visual Studio as the IDE for debugging and building.

### Notes for Emscripten Builds  
- Install Emscripten and activate its environment.
- Build FreeType(2.10.1) for Emscripten using the instructions: https://stackoverflow.com/questions/61049517/build-latest-freetype-with-emscripten
    - Add exception support:
        - `emcmake cmake .. -DCMAKE_CXX_FLAGS="-fexceptions" -DCMAKE_EXE_LINKER_FLAGS="-fexceptions" -DCMAKE_INSTALL_PREFIX=c:/src/freetype-2.10.1_em_install -DCMAKE_POLICY_VERSION_MINIMUM=3.5`
- Build OCCT(7.9.0) for Emscripten using the instructions: https://github.com/mathysyon/wasm-occ-demo
    - Add exception support:
        - CMAKE_CXX_FLAGS -fexceptions
        - CMAKE_EXE_LINKER_FLAGS -fexceptions
- Configure the EsyCad project with Emscripten:
    - `mkdir build_em`
    - `emcmake cmake .. -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade`
    - If you have ninja:
        - Debug: 
            1. `emcmake cmake .. -G Ninja -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade -DCMAKE_BUILD_TYPE=Debug`
            2. `ninja`
            3. At time of writing approx 50mb `EsyCas.wasm` file.
        - Release: 
            1. `emcmake cmake .. -G Ninja -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade -DCMAKE_BUILD_TYPE=Release` 
            2. `ninja`
            3. At time of writing approx 19mb `EsyCas.wasm` file.
    
- Build the project:
- Serve the WebAssembly: `python.exe -m http.server 8000`
- Or build and serve: `ninja && python.exe -m http.server 8000`
- ImGUI in `third_party` has been modified as explained in:
  https://github.com/ocornut/imgui/issues/7519#issuecomment-2629628233 to improve font rendering.

### Artwork
- Icons from: https://wiki.freecad.org/Artwork
