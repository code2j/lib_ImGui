cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)

project(main C CXX)


# ==================================================================
# 라이브러리 패키지
# ==================================================================
add_subdirectory(ImGui)


# ==================================================================
# 메인 실행 파일
# ==================================================================
if(${PROJECT_IS_TOP_LEVEL})
    add_executable(main
            main.cpp
    )

    target_compile_definitions(main
            PRIVATE
            ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_link_libraries(main
            PUBLIC
            ImGui
    )
endif()