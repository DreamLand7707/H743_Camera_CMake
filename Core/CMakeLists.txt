
set(Core_Name Camera_Core)

# Core project settings
project(${Core_Name} LANGUAGES C CXX ASM)

# Create an executable object type
add_executable(${Core_Name})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${Core_Name} PRIVATE

    # Add user defined library search paths
)

# Add sources to executable
target_sources(${Core_Name} PRIVATE

    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/Camera_1_bit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/sdram_drl.c

    ${CMAKE_CURRENT_SOURCE_DIR}/Conflict-Code/Core/Src/dcmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Conflict-Code/Driver/Src/stm32h7xx_hal_dcmi.c

    ${CMAKE_CURRENT_SOURCE_DIR}/Core/lvgl/examples/porting/lv_port_disp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/lvgl/examples/porting/lv_port_fs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/lvgl/examples/porting/lv_port_indev.c

    ${CMAKE_CURRENT_SOURCE_DIR}/Core/soft_iic/soft_iic.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/initial_task_routine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/lcd_touch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/my_heap_manage.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/timer_delay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/touch_sence_routine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/file_explorer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/bsp_fatfs_callback.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/path_func.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/memory_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code/std_new_delete.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/SEGGER-RTT/SEGGER_RTT_ASM_ARMv7M.S
    ${CMAKE_CURRENT_SOURCE_DIR}/SEGGER-RTT/SEGGER_RTT_printf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/SEGGER-RTT/SEGGER_RTT.c

    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcAssert.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcCounter.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcDiagnostics.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcEntryTable.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcError.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcEvent.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcEventBuffer.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcExtension.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcHardwarePort.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcHeap.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcInternalEventBuffer.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcInterval.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcISR.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcKernelPort.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcMultiCoreEventBuffer.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcObject.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcPrint.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcSnapshotRecorder.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcStackMonitor.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcStateMachine.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcStaticBuffer.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcStreamingRecorder.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcString.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcTask.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Src/trcTimestamp.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/streamport/trcStreamPort.c
)

# Add include paths
target_include_directories(${Core_Name} PRIVATE

    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/soft_iic
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/user-code

    ${CMAKE_CURRENT_SOURCE_DIR}/Conflict-Code/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Conflict-Code/Driver/Inc

    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Inc
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/Config
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/streamport/config
    # ${CMAKE_CURRENT_SOURCE_DIR}/Core/trace/streamport/include

    ${CMAKE_CURRENT_SOURCE_DIR}/SEGGER-RTT

    ${CMAKE_CURRENT_SOURCE_DIR}/Core/lvgl/examples/porting
    ${CMAKE_CURRENT_SOURCE_DIR}/../LVGL
    ${CMAKE_CURRENT_SOURCE_DIR}/../LVGL/lvgl/src
)

# Add project symbols (macros)
target_compile_definitions(${Core_Name} PRIVATE

    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${Core_Name} PRIVATE
    stm32cubemx

    # Add user defined libraries
)
