# This is the top-level CMake file for Shade.
#
# TODO: flesh out this comment block
# TODO: figure out why assimp is generating that damn empty Install target
# TODO: add install targets
# TODO: generate config files for asset paths and the like

cmake_minimum_required(VERSION 3.18)
enable_language(C)
enable_language(CXX)

project(Shade VERSION 0.1.0
              DESCRIPTION "Modern Graphics Programming Experimentation"
              LANGUAGES CXX)


#===============================================================================
#                                   Configs
#===============================================================================
# prevent ZERO_CHECK pseudo-project in Visual Studio
set(CMAKE_SUPPRESS_REGENERATION true)

# allow project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(BUILD_SHARED_LIBS ON)

# place all constructed binaries together to facilitate debugging
set(DEBUGGING_WORKING_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


#===============================================================================
#                           External Dependencies
#===============================================================================
# configure external libraries - Assimp, FMT
add_subdirectory(extern)

# configure ImGui
include(cmake/ImGui.cmake)

# configure the DirectX Shader Compiler (DXC)
include(cmake/DirectXShaderCompiler.cmake)

# configure Magic Enum
include(cmake/MagicEnum.cmake)

# loose headers
set(EXTRA_HEADERS src/d3dx12.h)


#===============================================================================
#                                   Shade
#===============================================================================
include_directories(src)

set(SHADE_SOURCES
    src/Camera.cpp
    src/Common.cpp
    src/Dx12RenderEngine.cpp
    src/GeometryManager.cpp
    src/Mesh.cpp
    src/PipelineState.cpp
    src/RenderEngine.cpp
    src/Scene.cpp
    src/Shade.cpp
    src/Shader.cpp
    src/ShaderToyScene.cpp
    src/Util.cpp
    src/Util3D.cpp
    src/Viewport.cpp
    src/Widgets.cpp
)
set(SHADE_HEADERS
    src/Camera.h
    src/Common.h
    src/Dx12RenderEngine.h
    src/GeometryManager.h
    src/Mesh.h
    src/PipelineState.h
    src/RenderEngine.h
    src/Scene.h
    src/Shade.h
    src/Shader.h
    src/ShaderToyScene.h
    src/Util.h
    src/Util3D.h
    src/Viewport.h
    src/Widgets.h
)
set(SHADE_NODES
    src/nodes/NodeEditor.h
    src/nodes/NodeEditor.cpp
    src/nodes/Node.h
    src/nodes/Node.cpp
    src/nodes/NodeNumeric.cpp
    src/nodes/NodeLink.h
)

# TODO: move shaders to their own subdirectory
set(SHADE_SHADERS src/shaders.hlsl)
source_group("Shaders" FILES ${SHADE_SHADERS})
set_source_files_properties(${SHADE_SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)

# group Shade dependencies
set(SHADE_SOURCES_ALL
    ${SHADE_SOURCES} ${SHADE_HEADERS} ${SHADE_NODES}
    ${IMGUI_SOURCES} ${IMGUI_HEADERS}
    ${IMNODES_SOURCES} ${IMNODES_HEADERS}
    ${EXTRA_HEADERS}
)

# add primary executable and set it's language version
message("SHADE_SHADERS: ${SHADE_SHADERS}")
add_executable(Shade WIN32 ${SHADE_SOURCES_ALL} ${SHADE_SHADERS})
set_property(TARGET Shade PROPERTY CXX_STANDARD 17)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Shade)

# link library dependencies
target_link_libraries(Shade assimp)
target_link_libraries(Shade fmt)
target_link_libraries(Shade d3d12.lib)
target_link_libraries(Shade dxgi.lib)
target_link_libraries(Shade dxguid.lib)
target_link_libraries(Shade dxcompiler.lib)
target_link_libraries(Shade dwmapi.lib)


#===============================================================================
#                                   Install
#===============================================================================
# TODO: install targets

# set debugger's working directory to project root to simplify asset paths
set_property(TARGET Shade
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${DEBUGGING_WORKING_DIR}
)

# copy DLLs into working directory to facilitate debugging
message("ASSIMP: ${ASSIMP_LIBRARY_OUTPUT_DIRECTORY}")
message("DXC: ${DXC_BINARY_DIR}")
add_custom_command(TARGET Shade POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ASSIMP_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/assimp-vc142-mt$<$<CONFIG:Debug>:d>.dll
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
)
add_custom_command(TARGET Shade POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DXC_BINARY_DIR}/dxcompiler.dll
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
)
