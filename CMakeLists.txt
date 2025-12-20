cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)

project(ImGui)


# 옵션
set(IMGUI_EXAMPLE TRUE)
set(OpenGL_GL_PREFERENCE GLVND)



# lib_ImGui 소스 코드
add_library(lib_imgui
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp

    implot_items.cpp
    implot.cpp

    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.cpp

    implot3d.cpp
    implot3d_items.cpp
    implot3d_meshes.cpp

    ImCoolBar.cpp
    ImNotification.cpp
    imgui_background.cpp
)


# include 경로 설정
target_include_directories(lib_imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)


# 외부 라이브러리 연결 (ImGui 백엔드용)
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    message(FATAL_ERROR "\n"
            "-------------------------------------------------------\n"
            " [Error] GLFW library not found!\n"
            " GLFW 라이브러리를 찾을 수 없습니다.\n"
            " 아래 명령어를 사용하여 설치해 주세요:\n\n"
            " sudo apt install libglfw3-dev\n"
            "-------------------------------------------------------"
    )
endif()


# 라이브러리 찾기
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)


# 라이브러리 연결
target_link_libraries(lib_imgui PUBLIC
    glfw
    OpenGL::GL
    X11::X11
)


# 예제 코드
if (IMGUI_EXAMPLE)
    add_executable(example_imgui example_main.cpp)
    target_link_libraries(example_imgui lib_imgui)
endif()