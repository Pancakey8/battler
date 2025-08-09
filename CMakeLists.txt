cmake_minimum_required(VERSION 3.16)
project(battler LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
add_subdirectory(vendors/SDL)

file(GLOB SRC src/*.c)
add_executable(battler main.c "${SRC}")
target_compile_options(battler PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(battler PRIVATE -fsanitize=address,leak)
target_include_directories(battler PRIVATE include/)
target_link_libraries(battler PRIVATE SDL3::SDL3)
target_link_libraries(battler PRIVATE m)
