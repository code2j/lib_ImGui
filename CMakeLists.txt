cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)

project(ImGui)


# 옵션
set(OpenGL_GL_PREFERENCE GLVND)



# lib_ImGui 소스 코드
add_library(libimgui
        src/imgui.cpp
        src/imgui_draw.cpp
        src/imgui_widgets.cpp
        src/imgui_tables.cpp
        src/imgui_demo.cpp

        src/implot_items.cpp
        src/implot.cpp

        backends/imgui_impl_glfw.cpp
        backends/imgui_impl_opengl3.cpp

        src/implot3d.cpp
        src/implot3d_items.cpp
        src/implot3d_meshes.cpp

        src/ImCoolBar.cpp
        src/ImNotification.cpp
        src/imgui_background.cpp
)


# include 경로 설정
target_include_directories(libimgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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


# libimgui에 필요한 라이브러리 연결
target_link_libraries(libimgui PUBLIC
        glfw
        OpenGL::GL
        X11::X11
)


# 예제 코드
if (PROJECT_IS_TOP_LEVEL)
        add_executable(example_imgui example/example_main.cpp)
        target_link_libraries(example_imgui libimgui)
endif()