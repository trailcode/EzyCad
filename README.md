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
- A C++20 compatible compiler (e.g., MSVC, GCC, or Clang)  
- OpenGL development libraries  
- Open CASCADE Technology (OCCT 7.9.0) https://github.com/Open-Cascade-SAS/OCCT

### OCCT Build
#### For VS2022
1. Create a build dir, e.g.: `C:\src\OCCT-7_9_0_build`
2. In this dir, run `cmake-gui` from the: x86 Native Tools Command Prompt for VS 2022
3. Press `Configure`
3. Set `3RDPARTY_DIR` equal to unzipped of: https://github.com/Open-Cascade-SAS/OCCT/releases/download/V7_9_0/3rdparty-vc14-64.zip
4. Press `Configure`, then `Generate`
5. In the `cmd` window: `ninja`

### Steps to Build  
1. Clone the repository:
2. Create a build directory:
3. Configure the project using CMake:
    E.G. `cmake .. -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_install\cmake`
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
- Configure the project with Emscripten:
    E.G. `emcmake cmake .. -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade`
    If you have `ninja` `emcmake cmake .. -G Ninja -DOpenCASCADE_DIR=C:\src\OCCT-7_9_0_em_install\lib\cmake\opencascade`
    
- Build the project:
- Serve the WebAssembly build locally: `python.exe -m http.server 8000`
- ImGUI in `third_party` has been modified as explained in:
  https://github.com/ocornut/imgui/issues/7519#issuecomment-2629628233 to improve font rendering.

### Artwork
- Icons from: https://wiki.freecad.org/Artwork
