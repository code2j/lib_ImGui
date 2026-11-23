cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0072 NEW)
project(main LANGUAGES C CXX)

add_subdirectory(ImGui)

if(PROJECT_IS_TOP_LEVEL)
    add_executable(main main.cpp)
    target_compile_features(main PRIVATE cxx_std_20)
    target_link_libraries(main PRIVATE ImGui)
    install(TARGETS main RUNTIME DESTINATION bin)
endif()
