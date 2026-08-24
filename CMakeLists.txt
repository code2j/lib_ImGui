cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)

project(main)

# NVML(NVIDIA GPU) 라이브러리 탐색 (GPU HUD 사용 시 필요)
find_library(NVML_LIB nvidia-ml)

# ==================================================================
# 라이브러리 패키지
# ==================================================================
add_subdirectory(raylib)
add_subdirectory(ImGui)

# 1. ImGui 타겟이 raylib.h를 참조할 수 있도록 연결 (raylib.h 미발견 에러 해결)
target_link_libraries(ImGui PUBLIC raylib)

if(${PROJECT_IS_TOP_LEVEL})
    add_executable(main
            main.cpp
    )

    # 2. IMGUI_ROOT 전처리기 매크로 정의 (IMGUI_ROOT 미선언 에러 해결)

    target_link_libraries(main
            PUBLIC
            ImGui
            raylib
    )

    # 3. NVIDIA GPU 모니터링 라이브러리 연결 (nvml 링킹 에러 방지)
    if(NVML_LIB)
        target_link_libraries(main PRIVATE ${NVML_LIB})
    endif()
endif ()