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
        src/ui_core.cpp
        src/ui_ini.cpp
        src/ui_loader_skybox.cpp
        src/ui_loader_texture.cpp
        src/ui_logger.cpp
        src/ui_notify.cpp
        src/ui_toggle.cpp
        src/ui_axis.cpp



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
# 컴파일 정의
# ==================================================================
target_compile_definitions(imgui
        PUBLIC
        "IMGUI_ROOT=\"${CMAKE_CURRENT_SOURCE_DIR}\""
)


# ==================================================================
# 예제 빌드
# ==================================================================
project(main)

if(${PROJECT_IS_TOP_LEVEL})
add_executable(main
        example/example_main.cpp
)

target_link_libraries(main
        imgui
)



endif ()
