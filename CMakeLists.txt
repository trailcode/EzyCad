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
set(BOOST_VERSION "1.87.0")

if(MSVC_VERSION)
  find_program(NUGET nuget)
  if(NOT NUGET)
    message(FATAL_ERROR "Cannot find nuget command line tool.\nInstall it with e.g. choco install nuget.commandline")
  endif()
  execute_process(COMMAND ${NUGET} install glfw                     -Version ${GLFW_VERSION}    -o thirdParty)
  execute_process(COMMAND ${NUGET} install unofficial-flayan-glew   -Version ${GLEW_VERSION}    -o thirdParty)
  execute_process(COMMAND ${NUGET} install boost                    -Version ${BOOST_VERSION}   -o thirdParty)
endif()


INCLUDE_DIRECTORIES(${PROJECT_NAME} ${BINARY_DIR}/thirdParty/boost.${BOOST_VERSION}/lib/native/include)

file(GLOB imguiSrc
  "third_party/imgui/imgui_impl_glfw.cpp"
  "third_party/imgui/imgui_impl_opengl3.cpp"
  "third_party/imgui/imgui.cpp"
  "third_party/imgui/imgui_demo.cpp"
  "third_party/imgui/imgui_draw.cpp"
  "third_party/imgui/imgui_widgets.cpp"
  "third_party/imgui/imgui_tables.cpp"
  "third_party/imgui/emscripten/emscripten_mainloop_stub.h")

file(GLOB srcs "src/*.cpp" "src/*.h" "src/*.inl")
#list(FILTER srcs EXCLUDE REGEX ".*main\\.cpp$")
get_filename_component(full_path_main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ABSOLUTE)
list(REMOVE_ITEM srcs ${full_path_main_cpp})

# Define source files for the application
# Including ImGui sources and main application file
set(App_SRCS ${imguiSrc} ${srcs})

# Configure platform-specific GUI settings
# WIN32 for Windows GUI application
# MACOSX_BUNDLE for MacOS GUI application
if(WIN32)
    set(GUI_TYPE WIN32)
endif(WIN32)
if(APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
endif(APPLE)

#add_executable(${PROJECT_NAME} 
#    ${App_SRCS}
#    ${imguiImplSrcs}
#    third_party/tinyfiledialogs/tinyfiledialogs.c)

add_library(${PROJECT_NAME}_lib ${App_SRCS} ${imguiImplSrcs} third_party/tinyfiledialogs/tinyfiledialogs.c)

# Open CASCADE Technology
find_package(OpenCASCADE REQUIRED NO_DEFAULT_PATH)
if(OpenCASCADE_FOUND)
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_DIR}\"" )
  message (STATUS "Using l OpenCASCADE from \"${OpenCASCADE_LIBRARY_DIR}\"" )
  
  INCLUDE_DIRECTORIES(${PROJECT_NAME} ${OpenCASCADE_INCLUDE_DIR})
  LINK_DIRECTORIES(${PROJECT_NAME} ${OpenCASCADE_LIBRARY_DIR})
else()
  message(WARNING "Could not find OpenCASCADE, please set OpenCASCADE_DIR variable." )
  set(OCCT_LIBRARY_DIR)
  set(OCCT_BIN_DIR)
endif()

file(GLOB ICON_ASSETS "${CMAKE_SOURCE_DIR}/icons/*.png")

set(OpenCASCADE_LIBS
    TKFillet
    TKOffset
    TKRWMesh
    TKDEGLTF
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
    TKDESTEP)

# Configure Emscripten-specific settings if using Emscripten compiler
if(CMAKE_CXX_COMPILER_ID STREQUAL "Emscripten")
    # Set compiler flags for Emscripten build
    message("flags " ${CMAKE_CXX_FLAGS} )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -s USE_SDL=2 \
            -s USE_BOOST_HEADERS=1 \
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
            -s EXPORTED_FUNCTIONS=\"['_main', '_on_file_selected', '_on_save_file_selected']\" \
            -s EXPORTED_RUNTIME_METHODS=\"['ccall']\" \
            --preload-file ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/DroidSans.ttf@/DroidSans.ttf \
            --preload-file ${CMAKE_SOURCE_DIR}/imgui.ini@/imgui.ini \
            --shell-file ${CMAKE_SOURCE_DIR}/web/EzyCad.html"
        )
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} \
            -s USE_SDL=2 \
            -s USE_BOOST_HEADERS=1 \
            -fexceptions \
            -gsource-map \
            --source-map-base http://localhost:8000/ \
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
            -s EXPORTED_FUNCTIONS=\"['_main', '_on_file_selected', '_on_save_file_selected']\" \
            -s EXPORTED_RUNTIME_METHODS=\"['ccall']\" \
            --preload-file ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/DroidSans.ttf@/DroidSans.ttf \
            --preload-file ${CMAKE_SOURCE_DIR}/imgui.ini@/imgui.ini \
            --shell-file ${CMAKE_SOURCE_DIR}/web/EzyCad.html"
        )
    endif()

    # Add each ICON_ASSETS file to the preload flags
    foreach(icon_file ${ICON_ASSETS})
        get_filename_component(icon_filename ${icon_file} NAME)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
            --preload-file ${icon_file}@/${icon_filename}")
    endforeach()

    list(APPEND OpenCASCADE_LIBS TKOpenGles)

    add_executable(${PROJECT_NAME}
        "src/main.cpp"
        ${App_SRCS}
        ${imguiImplSrcs}
        third_party/tinyfiledialogs/tinyfiledialogs.c)

    target_link_libraries(
        ${PROJECT_NAME}
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

    target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)
    target_link_libraries(${PROJECT_NAME} ${BINARY_DIR}/thirdParty/unofficial-flayan-glew.${GLEW_VERSION}/build/native/lib/x64/dynamic/glew32.lib)
    target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARY})
    
    target_link_libraries(${PROJECT_NAME} ${OpenCASCADE_LIBS})

    include_directories(${BINARY_DIR}/thirdParty/unofficial-flayan-glew.${GLEW_VERSION}/build/native/include)
    include_directories(${GLFW3_INCLUDE_PATH})

    # MSVC-specific configuration
    if(MSVC_VERSION)
      message("Compiler is MSVC")

      # Set working directory for Visual Studio debugging
      set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

      # Configure MSVC compiler flags
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc /MP")

      set(DLLS_COMMON 
          ${BINARY_DIR}/thirdParty/unofficial-flayan-glew.redist.${GLEW_VERSION}/build/native/bin/x64/dynamic/glew32.dll
          ${BINARY_DIR}/thirdParty/glfw.${GLFW_VERSION}/build/native/bin/dynamic/v142/x64/glfw3.dll
          ${OpenCASCADE_BINARY_DIR}/TKBO.dll
          ${OpenCASCADE_BINARY_DIR}/TKBool.dll
          ${OpenCASCADE_BINARY_DIR}/TKXSBase.dll
          ${OpenCASCADE_BINARY_DIR}/TKDE.dll
          C:/src/3rdparty-vc14-64/freetype-2.13.3-x64/bin/freetype.dll
      )
      foreach(lib ${OpenCASCADE_LIBS})
        list(APPEND DLLS_COMMON ${OpenCASCADE_BINARY_DIR}/${lib}.dll)
      endforeach(lib)
      set(FONT_ASSETS ${CMAKE_SOURCE_DIR}/third_party/imgui/fonts/DroidSans.ttf)
      
      set(CONFIG_ASSETS ${CMAKE_SOURCE_DIR}/imgui.ini)

      # Copy DLLs and assets to Release configurations
      foreach(file ${DLLS_RELEASE} ${DLLS_COMMON} ${FONT_ASSETS} ${ICON_ASSETS} ${CONFIG_ASSETS})
        file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Release")
        file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo")
        file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/MinSizeRel")
      endforeach(file)
      
      # Copy DLLs and assets to Debug configuration
      foreach(file ${DLLS_DEBUG} ${DLLS_COMMON} ${FONT_ASSETS} ${ICON_ASSETS} ${CONFIG_ASSETS})
          file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/Debug")
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
include_directories("third_party/json/include")

# Define preprocessor macros for GLM
add_definitions(-DGLM_ENABLE_EXPERIMENTAL)
add_definitions(-DGLM_FORCE_RADIANS)

# Link OpenGL libraries
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${OPENGL_gl_LIBRARY})

source_group("imgui" FILES ${imguiSrc})
source_group("src" FILES ${srcs})

# Use the file command to glob or list the .clang-format file
file(GLOB CONFIG_FILES "${CMAKE_SOURCE_DIR}/.clang-format")

# Add a custom target to include the .clang-format file in the project
add_custom_target(ClangFormat ALL
    SOURCES ${CONFIG_FILES}
)

# Add README to VS solution
if(MSVC)
    # Create a source group for documentation files
    set(DOC_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/README.md
        ${CMAKE_CURRENT_SOURCE_DIR}/usage.md
    )
    #source_group("Documentation" FILES ${DOC_FILES})
    
    # Add to project but exclude from build
    target_sources(${PROJECT_NAME} PRIVATE ${DOC_FILES})
    set_source_files_properties(${DOC_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
endif()

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
    file(GLOB TEST_SRCS "tests/*.cpp" "tests/*.h")
    add_executable(${PROJECT_NAME}_tests ${TEST_SRCS})
    
    # Add src directory to include paths for tests
    target_include_directories(${PROJECT_NAME}_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    
    # Link against Google Test and your project's libraries
    target_link_libraries(${PROJECT_NAME}_tests
        GTest::gtest_main
        GTest::gmock_main
        ${GLFW3_LIBRARY}
        ${PROJECT_NAME}_lib
        ${OpenCASCADE_LIBS}
        ${OPENGL_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
    )
    
    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(${PROJECT_NAME}_tests)
endif()
