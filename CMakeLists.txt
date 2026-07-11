# Require a minimum CMake version of 3.14.0
# - Needed for improved C++17 support
# - Better handling of target_link_libraries
# - Enhanced find_package functionality
# - Improved generator expressions
# - Modern CMake best practices
# FATAL_ERROR will stop processing if a lower version is used
cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(EzyCad)

# Set C++20 as the required standard for all targets
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Add custom CMake modules path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Detect Emscripten early (for optional branching)
if(CMAKE_CXX_COMPILER MATCHES "em\\+\\+")
  set(EMSCRIPTEN_BUILD TRUE)
endif()

# Python (optional embedded console) — native only.
set(EZYCAD_HAVE_PYTHON FALSE)
set(EZYCAD_PYTHON_LINK_MODE "")
set(EZYCAD_PYTHON_RUNTIME_DLL "")
if(NOT EMSCRIPTEN_BUILD)
  option(EZYCAD_ENABLE_PYTHON "Embed Python console (requires Python 3 development libraries)" ON)
  if(EZYCAD_ENABLE_PYTHON)
    find_package(Python3 3.8 QUIET COMPONENTS Interpreter Development)
    if(Python3_FOUND)
      include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EzyCadPython.cmake")
      ezycad_configure_embedded_python()
      if(EZYCAD_HAVE_PYTHON)
        message(STATUS "Python console: enabled (${Python3_VERSION}, ${Python3_EXECUTABLE})")
      endif()
    else()
      message(STATUS "Python console: disabled (Python 3 development libraries not found; set EZYCAD_ENABLE_PYTHON=OFF to silence)")
    endif()
  endif()
endif()

# Lua (for script console) - built for native and Emscripten (C sources compile with emcc)
# Tarball has no CMake; we build the lib from sources
include(FetchContent)
if(EZYCAD_HAVE_PYTHON)
  set(PYBIND11_FINDPYTHON ON)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
  )
  FetchContent_MakeAvailable(pybind11)
endif()
FetchContent_Declare(
  lua
  URL https://www.lua.org/ftp/lua-5.4.6.tar.gz
)
FetchContent_Populate(lua)
set(LUA_SRC_DIR ${lua_SOURCE_DIR}/lua-5.4.6/src)
if(NOT EXISTS ${LUA_SRC_DIR})
  set(LUA_SRC_DIR ${lua_SOURCE_DIR}/src)
endif()
if(NOT EXISTS ${LUA_SRC_DIR})
  message(FATAL_ERROR "Lua source not found in ${lua_SOURCE_DIR}")
endif()
set(LUA_LIB_SOURCES
  ${LUA_SRC_DIR}/lapi.c ${LUA_SRC_DIR}/lcode.c ${LUA_SRC_DIR}/lctype.c
  ${LUA_SRC_DIR}/ldebug.c ${LUA_SRC_DIR}/ldo.c ${LUA_SRC_DIR}/ldump.c
  ${LUA_SRC_DIR}/lfunc.c ${LUA_SRC_DIR}/lgc.c ${LUA_SRC_DIR}/llex.c
  ${LUA_SRC_DIR}/lmem.c ${LUA_SRC_DIR}/lobject.c ${LUA_SRC_DIR}/lopcodes.c
  ${LUA_SRC_DIR}/lparser.c ${LUA_SRC_DIR}/lstate.c ${LUA_SRC_DIR}/lstring.c
  ${LUA_SRC_DIR}/ltable.c ${LUA_SRC_DIR}/ltm.c ${LUA_SRC_DIR}/lundump.c
  ${LUA_SRC_DIR}/lvm.c ${LUA_SRC_DIR}/lzio.c
  ${LUA_SRC_DIR}/lauxlib.c ${LUA_SRC_DIR}/lbaselib.c ${LUA_SRC_DIR}/lcorolib.c
  ${LUA_SRC_DIR}/ldblib.c ${LUA_SRC_DIR}/liolib.c ${LUA_SRC_DIR}/lmathlib.c
  ${LUA_SRC_DIR}/loadlib.c ${LUA_SRC_DIR}/loslib.c ${LUA_SRC_DIR}/lstrlib.c
  ${LUA_SRC_DIR}/ltablib.c ${LUA_SRC_DIR}/lutf8lib.c ${LUA_SRC_DIR}/linit.c
)
add_library(lua STATIC ${LUA_LIB_SOURCES})
target_include_directories(lua PUBLIC ${LUA_SRC_DIR})
target_compile_features(lua PUBLIC c_std_99)

# Set binary directory paths
# Combines CMAKE_BINARY_DIR and project-specific binary dir for flexibility
set(BINARY_DIR ${${PROJECT_NAME}_BINARY_DIR})

# Detect if we're using Emscripten compiler (for WebAssembly builds)
if(CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?(\.bat)?$")
  message(" * C++ compiler: Emscripten")
  set(CMAKE_CXX_COMPILER_ID "Emscripten")
else()
  message(" * C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

set(GLFW_VERSION  "3.3.0.1")
set(GLEW_VERSION  "2.1.0")

if(MSVC_VERSION AND NOT CMAKE_TOOLCHAIN_FILE)
  find_program(NUGET nuget)
  if(NOT NUGET)
    message(FATAL_ERROR "Cannot find nuget command line tool.\nInstall it with e.g. choco install nuget.commandline")
  endif()
  # Output must be under the build tree (CMAKE_BINARY_DIR), not the shell CWD, or clean out-of-source configures fail.
  execute_process(COMMAND ${NUGET} install glfw                     -Version ${GLFW_VERSION}    -OutputDirectory "${CMAKE_BINARY_DIR}/thirdParty")
  execute_process(COMMAND ${NUGET} install unofficial-flayan-glew   -Version ${GLEW_VERSION}    -OutputDirectory "${CMAKE_BINARY_DIR}/thirdParty")
endif()

# imgui_demo.cpp omitted: large and unused here; add back if you need ImGui::ShowDemoWindow.
file(GLOB imguiSrc
  "third_party/imgui/imgui_impl_glfw.cpp"
  "third_party/imgui/imgui_impl_opengl3.cpp"
  "third_party/imgui/imgui.cpp"
  "third_party/imgui/imgui_draw.cpp"
  "third_party/imgui/imgui_widgets.cpp"
  "third_party/imgui/imgui_tables.cpp"
  "third_party/imgui/emscripten/emscripten_mainloop_stub.h")

file(GLOB srcs CONFIGURE_DEPENDS "src/*.cpp" "src/*.h" "src/*.inl")
#list(FILTER srcs EXCLUDE REGEX ".*main\\.cpp$")
get_filename_component(full_path_main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ABSOLUTE)
list(REMOVE_ITEM srcs ${full_path_main_cpp})

# Ensure utl_settings.cpp is always in the library (persistence for native + Emscripten)
set(utl_settings_cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/utl_settings.cpp)
list(REMOVE_ITEM srcs "src/utl_settings.cpp" ${utl_settings_cpp})
list(APPEND srcs ${utl_settings_cpp})

set(gui_settings_cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_settings.cpp)
list(REMOVE_ITEM srcs "src/gui_settings.cpp" ${gui_settings_cpp})
list(APPEND srcs ${gui_settings_cpp})

set(gui_mode_cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_mode.cpp)
list(REMOVE_ITEM srcs "src/gui_mode.cpp" ${gui_mode_cpp})
list(APPEND srcs ${gui_mode_cpp})

# Define source files for the application
# Including ImGui sources and main application file
set(App_SRCS ${imguiSrc} ${srcs})
set(EZYCAD_IMGUI_TEXT_EDITOR_CPP "${CMAKE_SOURCE_DIR}/third_party/ImGuiColorTextEdit/TextEditor.cpp")
if(NOT EXISTS "${EZYCAD_IMGUI_TEXT_EDITOR_CPP}")
  # Pinned commit (upstream has no release tags); bump when intentionally upgrading the editor.
  FetchContent_Declare(
    ImGuiColorTextEdit
    GIT_REPOSITORY https://github.com/BalazsJako/ImGuiColorTextEdit.git
    GIT_TAG        ca2f9f1462e3b60e56351bc466acda448c5ea50d
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(ImGuiColorTextEdit)
  set(_ige_fetch "${imguicolortextedit_SOURCE_DIR}")
  set(_ige_stage "${CMAKE_BINARY_DIR}/ImGuiColorTextEdit_fetch/ImGuiColorTextEdit")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/ImGuiColorTextEdit_fetch")
  file(COPY "${_ige_fetch}/" DESTINATION "${_ige_stage}")
  set(EZYCAD_IMGUI_TEXT_EDITOR_CPP "${_ige_stage}/TextEditor.cpp")
  include_directories("${CMAKE_BINARY_DIR}/ImGuiColorTextEdit_fetch")
  if(NOT EXISTS "${EZYCAD_IMGUI_TEXT_EDITOR_CPP}")
    message(FATAL_ERROR "ImGuiColorTextEdit fetch did not provide TextEditor.cpp (see third_party/README.md).")
  endif()
endif()
list(APPEND App_SRCS "${EZYCAD_IMGUI_TEXT_EDITOR_CPP}")

# Configure platform-specific GUI settings
# WIN32 for Windows GUI application
# MACOSX_BUNDLE for MacOS GUI application
if(WIN32)
    set(GUI_TYPE WIN32)
endif(WIN32)
if(APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
endif(APPLE)

add_library(${PROJECT_NAME}_lib ${App_SRCS} ${imguiImplSrcs} third_party/tinyfiledialogs/tinyfiledialogs.c)
target_link_libraries(${PROJECT_NAME}_lib PUBLIC lua)
if(EZYCAD_HAVE_PYTHON)
  target_compile_definitions(${PROJECT_NAME}_lib PRIVATE EZYCAD_HAVE_PYTHON=1)
  if(EZYCAD_PYTHON_LINK_MODE STREQUAL "imported_target")
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC ezycad_embed_python)
    target_link_libraries(${PROJECT_NAME}_lib PRIVATE pybind11::pybind11)
  elseif(EZYCAD_PYTHON_LINK_MODE STREQUAL "python3_target")
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC Python3::Python)
    target_link_libraries(${PROJECT_NAME}_lib PRIVATE pybind11::embed)
  else()
    message(FATAL_ERROR "EZYCAD_HAVE_PYTHON is ON but EZYCAD_PYTHON_LINK_MODE is not set.")
  endif()
endif()

# Open CASCADE Technology
find_package(OpenCASCADE REQUIRED NO_DEFAULT_PATH)
if(OpenCASCADE_FOUND)
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_DIR}\"" )
  
  INCLUDE_DIRECTORIES(${PROJECT_NAME} ${OpenCASCADE_INCLUDE_DIR})
  LINK_DIRECTORIES(${PROJECT_NAME} ${OpenCASCADE_LIBRARY_DIR})
else()
  message(WARNING "Could not find OpenCASCADE, please set OpenCASCADE_DIR variable." )
  set(OCCT_LIBRARY_DIR)
  set(OCCT_BIN_DIR)
endif()

# OCCT 3rd-party DLL path must not contain backticks — a common mistake is pasting
# -DOCCT_3RD_PARTY_DIR=... from Markdown and including the closing ` of `...`.
if(DEFINED OCCT_3RD_PARTY_DIR)
  string(REGEX REPLACE "[`]" "" OCCT_3RD_PARTY_DIR "${OCCT_3RD_PARTY_DIR}")
  string(STRIP "${OCCT_3RD_PARTY_DIR}" OCCT_3RD_PARTY_DIR)
  set(OCCT_3RD_PARTY_DIR "${OCCT_3RD_PARTY_DIR}" CACHE PATH "OCCT 3rd-party DLL root (freetype, ffmpeg, …)" FORCE)
endif()

file(GLOB ICON_ASSETS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/res/icons/*.png")
file(GLOB WEB_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/web/*.html")
set(WEB_FILES_REL)
foreach(f ${WEB_FILES})
  file(RELATIVE_PATH f_rel "${CMAKE_SOURCE_DIR}" "${f}")
  list(APPEND WEB_FILES_REL "${f_rel}")
endforeach()

set(OpenCASCADE_LIBS
    TKFillet
    TKOffset
    TKRWMesh
    TKDEOBJ
    TKDEPLY
    TKBinXCAF
    TKBin
    TKBinL
    TKXCAF
    TKVCAF
    TKCAF
    TKV3d
    TKHLR
    TKMesh
    TKService
    TKShHealing
    TKPrim
    TKTopAlgo
    TKGeomAlgo
    TKBRep
    TKGeomBase
    TKG3d
    TKG2d
    TKMath
    TKLCAF
    TKCDF
    TKernel
    TKDESTEP
    TKDEIGES
    TKDESTL)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
    # Advanced DataExchange modules (STEP, IGES, GLTF, etc.) are only linked for
    # native/desktop builds. The minimal WASM OCCT build from
    # scripts/build-occt-793-wasm.ps1 / build-occt-wasm.ps1 does not include them (to avoid Emscripten
    # incompatibilities with full DE + 3rd-party parsers). If you need them in WASM,
    # rebuild OCCT with more modules and add the missing TKDE* libs here conditionally.
    list(APPEND OpenCASCADE_LIBS TKDEGLTF)
endif()

# Configure Emscripten-specific settings if using Emscripten compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
    # Set compiler flags for Emscripten build
    message("flags " ${CMAKE_CXX_FLAGS} )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -s USE_SDL=2 \
            -fexceptions \
            -O3"
        )
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
            --bind \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s NO_EXIT_RUNTIME=0 \
            -fexceptions \
            -s ASSERTIONS=1 \
            -O3 \
            -s FULL_ES3=1 \
            -s USE_WEBGL2=1 \
            -s USE_GLFW=3 \
            -s USE_ZLIB=1 \
            -s EXPORTED_FUNCTIONS=\"['_main', '_on_file_selected', '_on_import_file_selected', '_on_sketch_underlay_selected', '_on_save_file_selected', '_emscripten_save_settings_on_unload']\" \
            -s EXPORTED_RUNTIME_METHODS=\"['ccall']\" \
            -s FETCH=1 \
            --preload-file ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/DroidSans.ttf@/DroidSans.ttf \
            --preload-file ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/Cousine-Regular.ttf@/Cousine-Regular.ttf \
            --preload-file ${CMAKE_SOURCE_DIR}/imgui.ini@/imgui.ini \
            --preload-file ${CMAKE_SOURCE_DIR}/res/ezycad_settings.json@/res/ezycad_settings.json \
            --preload-file ${CMAKE_SOURCE_DIR}/res/default.ezy@/res/default.ezy \
            --preload-file ${CMAKE_SOURCE_DIR}/res/examples@/res/examples \
            --preload-file ${CMAKE_SOURCE_DIR}/res/scripts/lua@/res/scripts/lua \
            --preload-file ${CMAKE_SOURCE_DIR}/res/about.md@/res/about.md \
            --preload-file ${CMAKE_SOURCE_DIR}/res/AI-gen-splashscreen_05_01_2026_512.png@/res/AI-gen-splashscreen_05_01_2026_512.png \
            --shell-file ${CMAKE_SOURCE_DIR}/web/EzyCad.html"
        )
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} \
            -s USE_SDL=2 \
            -fexceptions \
            -gsource-map \
            -O0"
        )
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
            --bind \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s NO_EXIT_RUNTIME=0 \
            -fexceptions \
            -s ASSERTIONS=1 \
            -gsource-map \
            --source-map-base http://localhost:8000/ \
            -O0 \
            -s FULL_ES3=1 \
            -s USE_WEBGL2=1 \
            -s USE_GLFW=3 \
            -s USE_ZLIB=1 \
            -s EXPORTED_FUNCTIONS=\"['_main', '_on_file_selected', '_on_import_file_selected', '_on_sketch_underlay_selected', '_on_save_file_selected', '_emscripten_save_settings_on_unload']\" \
            -s EXPORTED_RUNTIME_METHODS=\"['ccall']\" \
            -s FETCH=1 \
            --preload-file ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/DroidSans.ttf@/DroidSans.ttf \
            --preload-file ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/Cousine-Regular.ttf@/Cousine-Regular.ttf \
            --preload-file ${CMAKE_SOURCE_DIR}/imgui.ini@/imgui.ini \
            --preload-file ${CMAKE_SOURCE_DIR}/res/ezycad_settings.json@/res/ezycad_settings.json \
            --preload-file ${CMAKE_SOURCE_DIR}/res/default.ezy@/res/default.ezy \
            --preload-file ${CMAKE_SOURCE_DIR}/res/examples@/res/examples \
            --preload-file ${CMAKE_SOURCE_DIR}/res/scripts/lua@/res/scripts/lua \
            --preload-file ${CMAKE_SOURCE_DIR}/res/about.md@/res/about.md \
            --preload-file ${CMAKE_SOURCE_DIR}/res/AI-gen-splashscreen_05_01_2026_512.png@/res/AI-gen-splashscreen_05_01_2026_512.png \
            --shell-file ${CMAKE_SOURCE_DIR}/web/EzyCad.html"
        )
    endif()

    # Add each ICON_ASSETS file to the preload flags
    foreach(icon_file ${ICON_ASSETS})
        get_filename_component(icon_filename ${icon_file} NAME)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
            --preload-file ${icon_file}@/res/icons/${icon_filename}")
    endforeach()

    list(APPEND OpenCASCADE_LIBS TKOpenGles)

    add_executable(${PROJECT_NAME}
        "src/main.cpp"
        ${App_SRCS}
        ${imguiImplSrcs}
        third_party/tinyfiledialogs/tinyfiledialogs.c)

    # Copy EzyCad.html and default settings to the build directory for Emscripten
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/web/EzyCad.html
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/EzyCad.html
        COMMENT "Copying EzyCad.html to build directory"
    )
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/ezycad_settings.json
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/ezycad_settings.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/default.ezy
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/default.ezy
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/about.md
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/about.md
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/AI-gen-splashscreen_05_01_2026_512.png
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/AI-gen-splashscreen_05_01_2026_512.png
        COMMENT "Copying res/ezycad_settings.json and res/default.ezy for Emscripten"
    )
    file(GLOB RES_EXAMPLE_FILES_EM "${CMAKE_SOURCE_DIR}/res/examples/*.ezy")
    if(RES_EXAMPLE_FILES_EM)
      add_custom_command(
          TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/examples
          COMMENT "Creating res/examples for Emscripten"
      )
      foreach(example_file ${RES_EXAMPLE_FILES_EM})
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${example_file}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/examples/
            COMMENT "Copying example for Emscripten"
        )
      endforeach()
    endif()


    target_link_libraries(
        ${PROJECT_NAME}
        lua
        ${OpenCASCADE_LIBS})

else()

    list(APPEND OpenCASCADE_LIBS TKOpenGl)

    set(GLEW_LOCATION ${BINARY_DIR}/thirdParty/unofficial-flayan-glew.${GLEW_VERSION}/build/native)
    set(GLFW_ROOT_DIR ${BINARY_DIR}/thirdParty/glfw.${GLFW_VERSION}/build/native)

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

    # For non-Emscripten builds, find and link required libraries
    find_package(OpenGL REQUIRED)
    find_package(glfw3  REQUIRED)
    
    add_executable(${PROJECT_NAME} "src/main.cpp")

    # Copy default settings and examples so native build finds res/ at runtime
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/res
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/ezycad_settings.json
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/ezycad_settings.json
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/default.ezy
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/default.ezy
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/about.md
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/about.md
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/res/AI-gen-splashscreen_05_01_2026_512.png
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/AI-gen-splashscreen_05_01_2026_512.png
        COMMENT "Copying res/ezycad_settings.json and res/default.ezy for native"
    )
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/icons
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/res/icons
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/icons
        COMMENT "Copying res/icons for native")
    file(GLOB RES_EXAMPLE_FILES "${CMAKE_SOURCE_DIR}/res/examples/*.ezy")
    if(RES_EXAMPLE_FILES)
      add_custom_command(
          TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/examples
          COMMENT "Creating res/examples for native"
      )
      foreach(example_file ${RES_EXAMPLE_FILES})
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${example_file}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/examples/
            COMMENT "Copying example ${example_file}"
        )
      endforeach()
    endif()
    file(GLOB RES_LUA_SCRIPTS "${CMAKE_SOURCE_DIR}/res/scripts/lua/*.lua")
    if(RES_LUA_SCRIPTS)
      add_custom_command(
          TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/scripts/lua
          COMMENT "Creating res/scripts/lua for native"
      )
      foreach(lua_file ${RES_LUA_SCRIPTS})
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${lua_file}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/scripts/lua/
            COMMENT "Copying Lua script ${lua_file}"
        )
      endforeach()
    endif()
    file(GLOB RES_PYTHON_SCRIPTS "${CMAKE_SOURCE_DIR}/res/scripts/python/*.py")
    if(RES_PYTHON_SCRIPTS)
      add_custom_command(
          TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/scripts/python
          COMMENT "Creating res/scripts/python for native"
      )
      foreach(py_file ${RES_PYTHON_SCRIPTS})
        add_custom_command(
            TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${py_file}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>/res/scripts/python/
            COMMENT "Copying Python script ${py_file}"
        )
      endforeach()
    endif()

    target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)
    if(MSVC_VERSION AND NOT CMAKE_TOOLCHAIN_FILE)
      target_link_libraries(${PROJECT_NAME} ${BINARY_DIR}/thirdParty/unofficial-flayan-glew.${GLEW_VERSION}/build/native/lib/x64/dynamic/glew32.lib)
    endif()
    if(NOT MSVC)
      target_link_libraries(${PROJECT_NAME} -Wl,--no-as-needed ${GLFW3_LIBRARY} -Wl,--as-needed)
    else()
      target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARY})
    endif()
    
    target_link_libraries(${PROJECT_NAME} ${OpenCASCADE_LIBS})

    if(EZYCAD_HAVE_PYTHON AND EZYCAD_PYTHON_LINK_MODE STREQUAL "imported_target"
        AND EZYCAD_PYTHON_RUNTIME_DLL AND EXISTS "${EZYCAD_PYTHON_RUNTIME_DLL}")
      add_custom_command(
          TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${EZYCAD_PYTHON_RUNTIME_DLL}"
              $<TARGET_FILE_DIR:${PROJECT_NAME}>
          COMMENT "Copy Python DLL next to EzyCad.exe")
    endif()

    if(MSVC_VERSION AND NOT CMAKE_TOOLCHAIN_FILE)
      include_directories(${BINARY_DIR}/thirdParty/unofficial-flayan-glew.${GLEW_VERSION}/build/native/include)
    endif()
    if(GLFW3_INCLUDE_PATH)
      include_directories(${GLFW3_INCLUDE_PATH})
    endif()

    # MSVC-specific configuration
    if(MSVC_VERSION)
      message("Compiler is MSVC")

      # Set working directory for Visual Studio debugging
      set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

      # Configure MSVC compiler flags
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /MP")

      set(DLLS_COMMON 
          ${OpenCASCADE_BINARY_DIR}/TKBO.dll
          ${OpenCASCADE_BINARY_DIR}/TKBool.dll
          ${OpenCASCADE_BINARY_DIR}/TKXSBase.dll
          ${OpenCASCADE_BINARY_DIR}/TKDE.dll
          ${OCCT_3RD_PARTY_DIR}/freetype-2.13.3-x64/bin/freetype.dll
          ${OCCT_3RD_PARTY_DIR}/freeimage-3.18.0-x64/bin/FreeImage.dll
          ${OCCT_3RD_PARTY_DIR}/openvr-1.14.15-64/bin/win64/openvr_api.dll
          ${OCCT_3RD_PARTY_DIR}/ffmpeg-3.3.4-64/bin/avcodec-57.dll
          ${OCCT_3RD_PARTY_DIR}/ffmpeg-3.3.4-64/bin/avformat-57.dll
          ${OCCT_3RD_PARTY_DIR}/ffmpeg-3.3.4-64/bin/avutil-55.dll
          ${OCCT_3RD_PARTY_DIR}/ffmpeg-3.3.4-64/bin/swscale-4.dll
          ${OCCT_3RD_PARTY_DIR}/tbb-2021.13.0-x64/bin/tbb12.dll
          ${OCCT_3RD_PARTY_DIR}/jemalloc-vc14-64/bin/jemalloc.dll
      )
      if(MSVC_VERSION AND NOT CMAKE_TOOLCHAIN_FILE)
        list(APPEND DLLS_COMMON
          ${BINARY_DIR}/thirdParty/unofficial-flayan-glew.redist.${GLEW_VERSION}/build/native/bin/x64/dynamic/glew32.dll
          ${BINARY_DIR}/thirdParty/glfw.${GLFW_VERSION}/build/native/bin/dynamic/v142/x64/glfw3.dll
        )
      endif()

      # When building with vcpkg toolchain (e.g. the GitHub CI), the nuget glew/glfw
      # were never installed, so also stage the DLLs from the vcpkg manifest install
      # (typically <source>/vcpkg_installed/x64-windows/bin or under the build tree).
      # This ensures the Release/ folder contains a runnable EzyCad.exe.
      if(CMAKE_TOOLCHAIN_FILE)
        set(_vcpkg_bin "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/bin")
        if(NOT EXISTS "${_vcpkg_bin}/glfw3.dll")
          set(_vcpkg_bin "${BINARY_DIR}/vcpkg_installed/x64-windows/bin")
        endif()
        if(NOT EXISTS "${_vcpkg_bin}/glfw3.dll")
          # Fallback to common vcpkg classic location (if any packages landed there)
          set(_vcpkg_bin "C:/vcpkg/installed/x64-windows/bin")
        endif()
        if(EXISTS "${_vcpkg_bin}/glfw3.dll")
          list(APPEND DLLS_COMMON "${_vcpkg_bin}/glfw3.dll")
        endif()
        if(EXISTS "${_vcpkg_bin}/glew32.dll")
          list(APPEND DLLS_COMMON "${_vcpkg_bin}/glew32.dll")
        endif()
      endif()

      foreach(lib ${OpenCASCADE_LIBS})
        list(APPEND DLLS_COMMON ${OpenCASCADE_BINARY_DIR}/${lib}.dll)
      endforeach(lib)
      set(FONT_ASSETS
          ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/DroidSans.ttf
          ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/Cousine-Regular.ttf)
      
      set(CONFIG_ASSETS ${CMAKE_SOURCE_DIR}/imgui.ini)

      # Copy DLLs and assets to Release configurations.
      # Only copy files that actually exist on disk. This prevents configure errors in
      # the vcpkg-toolchain CI case (and prebuilt-OCCT path) where the nuget-populated
      # thirdParty/glew/glfw redist paths are never created (the early nuget block is
      # skipped when CMAKE_TOOLCHAIN_FILE is present).
      foreach(file ${DLLS_RELEASE} ${DLLS_COMMON} ${FONT_ASSETS} ${CONFIG_ASSETS})
        if(file AND EXISTS "${file}")
          file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
          file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")
          file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel")
        endif()
      endforeach(file)

      # Copy DLLs and assets to Debug configuration (same existence guard).
      foreach(file ${DLLS_DEBUG} ${DLLS_COMMON} ${FONT_ASSETS} ${CONFIG_ASSETS})
        if(file AND EXISTS "${file}")
          file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
        endif()
      endforeach(file)

    endif()
endif()

# Platform-specific configurations
if(WIN32)
    # Windows-specific linking (TBB commented out)
    #target_link_libraries(${PROJECT_NAME} debug ${TBB_tbb_LIBRARY_DEBUG} optimized ${TBB_tbb_LIBRARY_RELEASE})
endif(WIN32)
if(APPLE)
    # MacOS-specific linking for OpenGL and system frameworks
    SET(CMAKE_CXX_LINK_FLAGS "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")
endif(APPLE)


# Add include directories for all third-party dependencies
include_directories(.)
include_directories("third_party")
include_directories("third_party/imgui")
include_directories("third_party/imgui_markdown")
include_directories("third_party/json/include")

# Define preprocessor macros for GLM
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DGLM_FORCE_RADIANS)

# Link OpenGL libraries
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${OPENGL_gl_LIBRARY})

source_group("imgui" FILES ${imguiSrc})

set(SRC_SHP)
set(SRC_GUI)
set(SRC_UTL)
set(SRC_SKETCH)
set(SRC_SCR)
set(SRC_OTHER)
foreach(f ${srcs})
  get_filename_component(_src_name ${f} NAME)
  if(_src_name MATCHES "^shp")
    list(APPEND SRC_SHP ${f})
  elseif(_src_name MATCHES "^(gui|occt)")
    list(APPEND SRC_GUI ${f})
  elseif(_src_name MATCHES "^utl(_|\\.)")
    list(APPEND SRC_UTL ${f})
  elseif(_src_name MATCHES "^sketch")
    list(APPEND SRC_SKETCH ${f})
  elseif(_src_name MATCHES "^scr")
    list(APPEND SRC_SCR ${f})
  else()
    list(APPEND SRC_OTHER ${f})
  endif()
endforeach()
source_group("src\\shp" FILES ${SRC_SHP})
source_group("src\\gui" FILES ${SRC_GUI})
source_group("src\\utl" FILES ${SRC_UTL})
source_group("src\\sketch" FILES ${SRC_SKETCH})
source_group("src\\scr" FILES ${SRC_SCR})
source_group("src" FILES ${SRC_OTHER})
source_group("web" FILES ${WEB_FILES_REL})

# Root documentation (README.md, CHANGELOG.md) and docs/ folder contents are made
# visible in the IDE via the modern glob + source_group block near the end of this
# file (consistent with agents/, github-workflows, res/, scripts/). See below.

# Add Google Test only for non-Emscripten builds
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
    # Download and configure Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1  # Adjust version as needed
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Enable testing
    enable_testing()
    
    # Create a test executable
    file(GLOB TEST_SRCS CONFIGURE_DEPENDS "tests/*.cpp" "tests/*.h" "tests/*.inl")
    add_executable(${PROJECT_NAME}_tests ${TEST_SRCS})
    
    # Add src directory to include paths for tests
    target_include_directories(${PROJECT_NAME}_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )

    # Link against Google Test and your project's libraries
    target_link_libraries(${PROJECT_NAME}_tests
        GTest::gtest_main
        GTest::gmock_main
        ${PROJECT_NAME}_lib
        ${OpenCASCADE_LIBS}
        ${OPENGL_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
    )
    if(NOT MSVC)
      target_link_libraries(${PROJECT_NAME}_tests -Wl,--no-as-needed ${GLFW3_LIBRARY} -Wl,--as-needed)
    else()
      target_link_libraries(${PROJECT_NAME}_tests ${GLFW3_LIBRARY})
    endif()


    
    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_tests)
endif()

# web/*.html listed in IDE (single target_sources; Emscripten + native)
if(WEB_FILES_REL)
  target_sources(${PROJECT_NAME} PRIVATE ${WEB_FILES_REL})
endif()

# Runtime assets: show res/ in the IDE (matches POST_BUILD copy to output dir)
file(GLOB_RECURSE RES_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/res/*")
if(RES_FILES)
  source_group(TREE "${CMAKE_SOURCE_DIR}/res" PREFIX "res" FILES ${RES_FILES})
  set(RES_FILES_REL)
  foreach(f ${RES_FILES})
    file(RELATIVE_PATH f_rel "${CMAKE_SOURCE_DIR}" "${f}")
    list(APPEND RES_FILES_REL "${f_rel}")
  endforeach()
  target_sources(${PROJECT_NAME} PRIVATE ${RES_FILES_REL})
  set_source_files_properties(${RES_FILES_REL} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# Utility scripts: show scripts/ in the IDE (not compiled; same pattern as res/)
file(GLOB_RECURSE EZYCAD_SCRIPT_FILES CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  "${CMAKE_SOURCE_DIR}/scripts/*")
if(EZYCAD_SCRIPT_FILES)
  source_group(TREE "${CMAKE_SOURCE_DIR}/scripts" PREFIX "scripts" FILES ${EZYCAD_SCRIPT_FILES})
  set(EZYCAD_SCRIPT_FILES_REL)
  foreach(f ${EZYCAD_SCRIPT_FILES})
    file(RELATIVE_PATH f_rel "${CMAKE_SOURCE_DIR}" "${f}")
    list(APPEND EZYCAD_SCRIPT_FILES_REL "${f_rel}")
  endforeach()
  target_sources(${PROJECT_NAME}_lib PRIVATE ${EZYCAD_SCRIPT_FILES_REL})
  target_sources(${PROJECT_NAME} PRIVATE ${EZYCAD_SCRIPT_FILES_REL})
  set_source_files_properties(${EZYCAD_SCRIPT_FILES_REL} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# GitHub workflow files: show .github/workflows/*.yml in the IDE under 'github-workflows' folder
# (so they are visible in Visual Studio Solution Explorer etc. without being compiled)
file(GLOB_RECURSE GITHUB_WORKFLOW_FILES CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  "${CMAKE_SOURCE_DIR}/.github/workflows/*.yml")
if(GITHUB_WORKFLOW_FILES)
  source_group(TREE "${CMAKE_SOURCE_DIR}/.github/workflows" PREFIX "github-workflows" FILES ${GITHUB_WORKFLOW_FILES})
  set(GITHUB_WORKFLOW_FILES_REL)
  foreach(f ${GITHUB_WORKFLOW_FILES})
    file(RELATIVE_PATH f_rel "${CMAKE_SOURCE_DIR}" "${f}")
    list(APPEND GITHUB_WORKFLOW_FILES_REL "${f_rel}")
  endforeach()
  target_sources(${PROJECT_NAME}_lib PRIVATE ${GITHUB_WORKFLOW_FILES_REL})
  target_sources(${PROJECT_NAME} PRIVATE ${GITHUB_WORKFLOW_FILES_REL})
  set_source_files_properties(${GITHUB_WORKFLOW_FILES_REL} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# Agent / design / issue tracking files: show agents/ in the IDE under 'agents' folder
# (markdown docs, issues/, prs/ subfolders etc. visible in Visual Studio Solution Explorer
# and other IDEs without participating in the build)
file(GLOB_RECURSE EZYCAD_AGENTS_FILES CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  "${CMAKE_SOURCE_DIR}/agents/*")
if(EZYCAD_AGENTS_FILES)
  source_group(TREE "${CMAKE_SOURCE_DIR}/agents" PREFIX "agents" FILES ${EZYCAD_AGENTS_FILES})
  set(EZYCAD_AGENTS_FILES_REL)
  foreach(f ${EZYCAD_AGENTS_FILES})
    file(RELATIVE_PATH f_rel "${CMAKE_SOURCE_DIR}" "${f}")
    list(APPEND EZYCAD_AGENTS_FILES_REL "${f_rel}")
  endforeach()
  target_sources(${PROJECT_NAME}_lib PRIVATE ${EZYCAD_AGENTS_FILES_REL})
  target_sources(${PROJECT_NAME} PRIVATE ${EZYCAD_AGENTS_FILES_REL})
  set_source_files_properties(${EZYCAD_AGENTS_FILES_REL} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# Documentation folder: show docs/ (and the moved usage guides, style guides, building-occt.md etc.)
# in the IDE under a 'docs' folder for both the library and executable targets.
# (non-compiled; uses the same pattern as agents/, github-workflows, res/, scripts/)
file(GLOB_RECURSE EZYCAD_DOCS_FILES CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  "${CMAKE_SOURCE_DIR}/docs/*")
if(EZYCAD_DOCS_FILES)
  source_group(TREE "${CMAKE_SOURCE_DIR}/docs" PREFIX "docs" FILES ${EZYCAD_DOCS_FILES})
  set(EZYCAD_DOCS_FILES_REL)
  foreach(f ${EZYCAD_DOCS_FILES})
    file(RELATIVE_PATH f_rel "${CMAKE_SOURCE_DIR}" "${f}")
    list(APPEND EZYCAD_DOCS_FILES_REL "${f_rel}")
  endforeach()
  target_sources(${PROJECT_NAME}_lib PRIVATE ${EZYCAD_DOCS_FILES_REL})
  target_sources(${PROJECT_NAME} PRIVATE ${EZYCAD_DOCS_FILES_REL})
  set_source_files_properties(${EZYCAD_DOCS_FILES_REL} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# Also surface the two main root docs (README + CHANGELOG) at the project root in IDEs
set(README_FILE ${CMAKE_SOURCE_DIR}/README.md)
if(EXISTS ${README_FILE})
    source_group("" FILES ${README_FILE})
    target_sources(${PROJECT_NAME}_lib PRIVATE ${README_FILE})
    target_sources(${PROJECT_NAME} PRIVATE ${README_FILE})
    set_source_files_properties(${README_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()
set(CHANGELOG_FILE ${CMAKE_SOURCE_DIR}/CHANGELOG.md)
if(EXISTS ${CHANGELOG_FILE})
    source_group("" FILES ${CHANGELOG_FILE})
    target_sources(${PROJECT_NAME}_lib PRIVATE ${CHANGELOG_FILE})
    target_sources(${PROJECT_NAME} PRIVATE ${CHANGELOG_FILE})
    set_source_files_properties(${CHANGELOG_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# src/doc: developer markdown for src modules (non-compiled; nested folders under src\doc in the IDE)
file(GLOB_RECURSE SRC_DOC_FILES CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  "${CMAKE_SOURCE_DIR}/src/doc/*")
if(SRC_DOC_FILES)
  source_group(TREE "${CMAKE_SOURCE_DIR}/src/doc" PREFIX "src\\doc" FILES ${SRC_DOC_FILES})
  set(SRC_DOC_FILES_REL)
  foreach(f ${SRC_DOC_FILES})
    file(RELATIVE_PATH f_rel "${CMAKE_SOURCE_DIR}" "${f}")
    list(APPEND SRC_DOC_FILES_REL "${f_rel}")
  endforeach()
  target_sources(${PROJECT_NAME}_lib PRIVATE ${SRC_DOC_FILES_REL})
  target_sources(${PROJECT_NAME} PRIVATE ${SRC_DOC_FILES_REL})
  set_source_files_properties(${SRC_DOC_FILES_REL} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

# Add .clang-format and .gitignore to EzyCad and EzyCad_lib so they appear in VS and other IDEs (at project root)
set(CLANG_FORMAT_FILE ${CMAKE_SOURCE_DIR}/.clang-format)
if(EXISTS ${CLANG_FORMAT_FILE})
    source_group("" FILES ${CLANG_FORMAT_FILE})
    target_sources(${PROJECT_NAME}_lib PRIVATE ${CLANG_FORMAT_FILE})
    target_sources(${PROJECT_NAME} PRIVATE ${CLANG_FORMAT_FILE})
    set_source_files_properties(${CLANG_FORMAT_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()
set(GITIGNORE_FILE ${CMAKE_SOURCE_DIR}/.gitignore)
if(EXISTS ${GITIGNORE_FILE})
    source_group("" FILES ${GITIGNORE_FILE})
    target_sources(${PROJECT_NAME}_lib PRIVATE ${GITIGNORE_FILE})
    target_sources(${PROJECT_NAME} PRIVATE ${GITIGNORE_FILE})
    set_source_files_properties(${GITIGNORE_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()
