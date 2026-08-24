cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)


# ==================================================================
# ImGui 소스
# ==================================================================
add_library(ImGui
        # Backends
        imgui_impl_glfw.cpp
        imgui_impl_opengl3.cpp
        # ImGui
        imgui.cpp
        imgui_draw.cpp
        imgui_widgets.cpp
        imgui_tables.cpp
        imgui_demo.cpp
        # ImPlot
        implot_items.cpp
        implot.cpp
        # ImPlot3D
        implot3d.cpp
        implot3d_items.cpp
        implot3d_meshes.cpp
        # My code
        ui_core.cpp
        ui_ini.cpp
        ui_loader_skybox.cpp
        ui_loader_texture.cpp
        ui_logger.cpp
        ui_notify.cpp
        ui_axis.cpp
        ui_points.cpp
        ui_widgets.cpp
)


# ==================================================================
# ImGui 헤더
# ==================================================================
target_include_directories(ImGui
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

# ==================================================================
# ImGui 컴파일 정의
# ==================================================================
target_compile_definitions(ImGui
        PUBLIC
        IMGUI_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/.."
)
