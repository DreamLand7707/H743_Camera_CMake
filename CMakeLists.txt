cmake_minimum_required(VERSION 3.22)


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Core")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/LVGL")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME Camera)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

target_link_libraries(Camera PRIVATE lvgl_static_lib)
