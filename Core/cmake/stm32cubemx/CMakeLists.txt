cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	USE_PWR_LDO_SUPPLY 
	USE_HAL_DRIVER 
	STM32H743xx
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LIBJPEG/App
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LIBJPEG/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/JPEG
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Conflict-Code/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Conflict-Code/Driver/Inc
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/trace/Inc
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/trace/Config
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/trace/streamport/config
    # ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/trace/streamport/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../SEGGER-RTT
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/lvgl/examples/porting
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../LVGL
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../LVGL/lvgl/src
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LIBJPEG/Target/jdata_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../LIBJPEG/App/libjpeg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/App/fatfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/Target/bsp_driver_sd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/Target/sd_diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../FATFS/Target/fatfs_platform.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/freertos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/dcmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/dma2d.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/fmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/jpeg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/ltdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/mdma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sdmmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/usart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32h7xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32h7xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/stm32h7xx_hal_timebase_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32h743xx.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Core/Src/system_stm32h7xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dcmi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma2d.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_fmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_nor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_nand.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sdram.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_jpeg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_ltdc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dsi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rtc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_delayblock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mmc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mmc_ex.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
)

# Drivers Midllewares


set(Utilities_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Utilities/JPEG/jpeg_utils.c
)
set(LibJPEG_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcapimin.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcapistd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdapimin.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdapistd.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcomapi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcparam.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jctrans.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdtrans.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcinit.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcmaster.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcmainct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcprepct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jccoefct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jccolor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcsample.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcdctmgr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jfdctint.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jfdctfst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jfdctflt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jchuff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcarith.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jcmarker.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdatadst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdmaster.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdinput.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdmainct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdcoefct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdpostct.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdmarker.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdhuff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdarith.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jddctmgr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jidctint.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jidctfst.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jidctflt.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdsample.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdcolor.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdmerge.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jquant1.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jquant2.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jdatasrc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jaricom.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jerror.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jmemmgr.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jutils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/LibJPEG/source/jmemnobs.c
)
set(FreeRTOS_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/croutine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/event_groups.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/list.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/queue.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/stream_buffer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/timers.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS/cmsis_os.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/portable/MemMang/heap_4.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
)
set(FatFs_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/diskio.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/ff.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/ff_gen_drv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/option/syscall.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/Third_Party/FatFs/src/option/cc936.c
)

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    Utilities
	LibJPEG
	FreeRTOS
	FatFs
	
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)
# Validate that STM32CubeMX code is compatible with C++ standard
if(CMAKE_CXX_STANDARD LESS 11)
    message(ERROR "Generated code requires C++11 or higher")
endif()

# Create Utilities static library
add_library(Utilities OBJECT)
target_sources(Utilities PRIVATE ${Utilities_Src})
target_link_libraries(Utilities PUBLIC stm32cubemx)

# Create LibJPEG static library
add_library(LibJPEG OBJECT)
target_sources(LibJPEG PRIVATE ${LibJPEG_Src})
target_link_libraries(LibJPEG PUBLIC stm32cubemx)

# Create FreeRTOS static library
add_library(FreeRTOS OBJECT)
target_sources(FreeRTOS PRIVATE ${FreeRTOS_Src})
target_link_libraries(FreeRTOS PUBLIC stm32cubemx)

# Create FatFs static library
add_library(FatFs OBJECT)
target_sources(FatFs PRIVATE ${FatFs_Src})
target_link_libraries(FatFs PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
target_sources(${Core_Name} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${Core_Name} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${Core_Name} PRIVATE ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${Core_Name} PROPERTIES ADDITIONAL_CLEAN_FILES ${Core_Name}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
