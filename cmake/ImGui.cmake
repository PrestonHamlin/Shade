# ImGui is a source-only library intended to be directly included, rather than
#   compiled and linked against. As such, it does not have a build system and
#   we will simply include this CMake file.

set(IMGUI_SOURCE_PATH ${PROJECT_SOURCE_DIR}/extern/imgui)
set(IMGUI_SOURCE_PATH_BACKENDS ${IMGUI_SOURCE_PATH}/backends)
set(IMGUI_SOURCE_DIRS ${IMGUI_SOURCE_PATH} ${IMGUI_SOURCE_PATH_BACKENDS})

set(IMGUI_SOURCES
    ${IMGUI_SOURCE_PATH}/imgui.cpp
    ${IMGUI_SOURCE_PATH}/imgui_demo.cpp
    ${IMGUI_SOURCE_PATH}/imgui_draw.cpp
    ${IMGUI_SOURCE_PATH}/imgui_tables.cpp
    ${IMGUI_SOURCE_PATH}/imgui_widgets.cpp
    ${IMGUI_SOURCE_PATH}/backends/imgui_impl_dx12.cpp
    ${IMGUI_SOURCE_PATH}/backends/imgui_impl_win32.cpp
)
set(IMGUI_HEADERS
    ${IMGUI_SOURCE_PATH}/imgui.h
    ${IMGUI_SOURCE_PATH}/backends/imgui_impl_dx12.h
    ${IMGUI_SOURCE_PATH}/backends/imgui_impl_win32.h
)
source_group("ImGui" FILES ${IMGUI_SOURCES} ${IMGUI_HEADERS})

include_directories(${IMGUI_SOURCE_DIRS})
