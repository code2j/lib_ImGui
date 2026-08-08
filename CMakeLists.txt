cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)


# ==================================================================
# 라이브러리 패키지
# ==================================================================
add_subdirectory(raylib)


# ==================================================================
# ImGui 소스
# ==================================================================
add_library(imgui
        # Backends
        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_opengl3.cpp
        # ImGui
        src/imgui.cpp
        src/imgui_draw.cpp
        src/imgui_widgets.cpp
        src/imgui_tables.cpp
        src/imgui_demo.cpp
        # ImPlot
        src/implot_items.cpp
        src/implot.cpp
        # ImPlot3D
        src/implot3d.cpp
        src/implot3d_items.cpp
        src/implot3d_meshes.cpp
        # My code
        src/gui_log.cpp
)


# ==================================================================
# ImGui 헤더
# ==================================================================
target_include_directories(imgui
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/backends
        ${CMAKE_CURRENT_LIST_DIR}/raylib/src
)


# ==================================================================
# ImGui 링크
# ==================================================================
target_link_libraries(imgui
        PUBLIC
        raylib
)


# ==================================================================
# 예제 빌드
# ==================================================================
project(main)

add_executable(main
        example/example_main.cpp
)

target_link_libraries(main
        imgui
)

target_compile_definitions(main
        PRIVATE
        "IMGUI_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}/\""
)


