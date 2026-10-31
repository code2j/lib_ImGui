cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)

project(main C CXX)


# ==================================================================
# 라이브러리 패키지
# ==================================================================
add_subdirectory(raylib)
add_subdirectory(ImGui)


target_link_libraries(ImGui
        PUBLIC raylib
)
target_include_directories(ImGui
        PUBLIC "${CMAKE_CURRENT_LIST_DIR}"
)