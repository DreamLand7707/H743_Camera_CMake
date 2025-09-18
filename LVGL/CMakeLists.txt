set(All_Include_Dir
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/FATFS/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/FATFS/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Utilities/JPEG
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Middlewares/Third_Party/FreeRTOS/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Middlewares/Third_Party/FatFs/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Drivers/CMSIS/Include

    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/lvgl/examples/porting
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/lvgl/soft_iic

    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Conflict-Code/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Conflict-Code/Driver/Inc

    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/trace/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/trace/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/trace/streamport/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/trace/streamport/include

    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/user-code

    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Core/SEGGER-RTT

    ${CMAKE_CURRENT_SOURCE_DIR}/LVGL
    ${CMAKE_CURRENT_SOURCE_DIR}/LVGL/lvgl/src
)

set(src_dir "${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src")

file(
    GLOB_RECURSE All_Source
    LIST_DIRECTORIES FALSE
    "${src_dir}/*.c"
)

add_library(lvgl_static_lib STATIC)

target_sources(lvgl_static_lib PRIVATE "${All_Source}")
target_include_directories(lvgl_static_lib PRIVATE "${All_Include_Dir}")
