cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)

project(main C CXX)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)


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

    target_compile_features(main PRIVATE cxx_std_20)

    target_compile_definitions(main
            PRIVATE
            ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    ament_target_dependencies(main rclcpp sensor_msgs)

    install(TARGETS main DESTINATION lib/${PROJECT_NAME})

    target_link_libraries(main
            ImGui
    )
endif()
ament_package()
