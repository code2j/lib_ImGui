cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)


# ==================================================================
# ImGui 소스
# ==================================================================
file(GLOB IMGUI_SRC "*.cpp" "*.h" "*.hpp")
add_library(ImGui
        ${IMGUI_SRC}
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
        IMGUI_ROOT="${CMAKE_CURRENT_LIST_DIR}"
)
