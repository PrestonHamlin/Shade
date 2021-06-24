# This is the top-level CMake file for Shade.
#
# TODO: flesh out this comment block
# TODO: figure out why assimp is generating that damn empty Install target
# TODO: add a deploy/install mechanism for debugging app

cmake_minimum_required(VERSION 3.18)
enable_language(C)
enable_language(CXX)

project(Shade VERSION 0.1.0
              DESCRIPTION "Modern Graphics Programming Experimentation"
              LANGUAGES CXX)

# prevent ZERO_CHECK pseudo-project in Visual Studio
set(CMAKE_SUPPRESS_REGENERATION true)
# allow project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


set(SHADE_SOURCES
    src/Dx12RenderEngine.cpp
    src/Mesh.cpp
    src/PipelineState.cpp
    src/RenderEngine.cpp
    src/Scene.cpp
    src/Shade.cpp
    src/Shader.cpp
    src/ShaderToyScene.cpp
    src/Util.cpp
    src/Viewport.cpp
    src/Widgets.cpp
)
set(SHADE_HEADERS
    src/Dx12RenderEngine.h
    src/Mesh.h
    src/PipelineState.h
    src/RenderEngine.h
    src/Scene.h
    src/Shade.h
    src/Shader.h
    src/ShaderToyScene.h
    src/Util.h
    src/Viewport.h
    src/Widgets.h
)

# TODO: move shaders to their own subdirectory
set(SHADE_SHADERS src/shaders.hlsl)

set(EXTRA_HEADERS src/d3dx12.h)

# TODO: move into child CMakeLists.txt?
set(IMGUI_SOURCES
    extern/imgui/imgui.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/backends/imgui_impl_dx12.cpp
    extern/imgui/backends/imgui_impl_win32.cpp
)
set(IMGUI_HEADERS
    extern/imgui/imgui.h
    extern/imgui/backends/imgui_impl_dx12.h
    extern/imgui/backends/imgui_impl_win32.h
)

set(ALL_SOURCES ${SHADE_SOURCES} ${IMGUI_SOURCES})
set(ALL_HEADERS ${SHADE_HEADERS} ${IMGUI_HEADERS})

# try and prevent IDE from compiling shaders
set_source_files_properties(${SHADE_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
source_group("Shaders" FILES ${SHADE_SHADERS})
source_group("ImGui" FILES ${IMGUI_SOURCES} ${IMGUI_HEADERS})

# add primary executable and set it's language version
add_executable(Shade WIN32 ${ALL_SOURCES} ${ALL_HEADERS} ${SHADE_SHADERS})
set_property(TARGET Shade PROPERTY CXX_STANDARD 17)

set(BUILD_SHARED_LIBS ON)

# configure and add assimp library
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_INSTALL OFF)
add_subdirectory(extern/assimp)
# add_subdirectory(extern/assimp EXCLUDE_FROM_ALL)
set_target_properties(assimp PROPERTIES FOLDER "Libraries")
set_target_properties(zlibstatic PROPERTIES FOLDER "Libraries")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Libraries")


add_subdirectory(extern/fmt)
set_target_properties(fmt PROPERTIES FOLDER "Libraries")
# add_subdirectory(extern/fmt EXCLUDE_FROM_ALL)
# ImGui is a source-only library intended to be directly included, rather than linked.

include_directories(extern/assimp/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/extern/assimp/include)
include_directories(extern/fmt/include)
include_directories(extern/imgui)
include_directories(extern/imgui/backends)
include_directories(extern/dxc)

# DXC is a dependency, but the project is massive. The SDK copies tend to suck.
# TODO: grab release archive off GitHub and extract it
# find_library(DXC_LOCATION dxcompiler REQUIRED PATHS extern/dxc)
message("DXC: ${DXC_LOCATION}")
link_directories(${PROJECT_SOURCE_DIR}/extern/dxc)

target_link_libraries(Shade assimp)
target_link_libraries(Shade fmt)
target_link_libraries(Shade d3d12.lib)
target_link_libraries(Shade dxgi.lib)
target_link_libraries(Shade dxguid.lib)
# target_link_libraries(Shade dxcompiler.lib)
# TODO: copy dxcompiler DLL to build/install directory, since the lib is just a stub
target_link_libraries(Shade ${PROJECT_SOURCE_DIR}/extern/dxc/dxcompiler.lib)
target_link_libraries(Shade dwmapi.lib)

# until a deployment mechanism is worked out, resources are relative to root
set_property(TARGET Shade PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
