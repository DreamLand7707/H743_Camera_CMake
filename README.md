# STM32H743 Camera



本项目使用正点原子Apollo H743的板子作为参考设计



## 涉及到的板级硬件

1. SDRAM，容量32MB（型号同参考设计）
2. DVP接口插座（用于连接OV5640摄像头）
3. TF卡插座（连接Micro-SD卡）
4. RGB屏幕接口（因为设计使用了Apollo核心板，所以使用的是RGB565）
5. RGB屏幕IIC接口和中断线（触摸）
6. 按钮x1（按下后可以拍照）

## 涉及到的SoC级硬件

### 内存

1. AXI SRAM（主内存空间，放置FreeRTOS任务代码，分配内存等操作）
2. SRAM2（作为从DCMI->SDRAM的中间跳板）
3. SDRAM（存储大块数据，作为显存，图片解码数据缓冲区，摄像头图片缓冲区）
4. DTCM（调试时用于作为SEGGER RTT的缓冲区）
5. ICACHE DCACHE（用于提升CPS）

### 外设

1. LTDC（用于驱动RGB屏幕）
2. USART1（打印信息）
3. DCMI（连接摄像头）
4. SDMMC2（1Bit，硬件流控模式）
5. FMC（驱动SDRAM）
6. JPEG（用于查看图片，解码JPEG图片）
7. RTC（保存图片用于设定日期——其实根本没有低功耗设计，只有断电，所以RTC作用不大）
8. EXTI（GPIO中断）

### DMA

1. DMA1 Stream0，用于DCMI->SRAM2（DCMI只能和DMA1搭配使用）
2. MDMA Channel2，用于SRAM2->SDRAM（直接用DMA1=>SDRAM总线带宽不够用，必须使用SRAM2作为跳板后用MDMA才可以）
3. SDMMC2 IDMA（用于大量存储/读取数据）

### 定时器

1. TIM3（用于FreeRTOS Run Stats统计信息）

### GPIO

1. T_CS（I8）用于触摸屏的触摸功能时序
2. T_PEN（H7）触摸屏的中断线
3. T_SCK T_SDA（H6 I3）触摸屏IIC接口
4. LCD_BL（B5）屏幕的背光
5. SHOT（H2）按下拍照
6. SDCARD_KEY（H3）SD卡插拔检测，但功能没有实现，只预留了接口
7. DCMI_RESET DCMI_SDA DCMI_SCL（摄像头的复位和SCCB接口）

## 涉及到的软件（开源项目或其余库）

1. FreeRTOS（使用CubeMX生成）
2. FatFS（使用CubeMX生成）
3. LVGL（手动移植，版本9.2.2，正好是不支持DMA2D的那个版本）
4. SEGGER RTT（版本7.94）

在代码库中会有很多多余的文件，但只要没出现在这个列表里，就是没用上。

## 涉及到的构建软件

1. CMake（3.22版本及以上）
2. arm-none-eabi-gcc arm-none-eabi-g++（至少要支持C++20）
3. ninja

## 涉及到的硬件模块

1. RGB屏幕（分辨率480x272，触摸芯片需兼容GT9174/GT1151Q/GT1158）
2. TF卡，支持高速单线SD模式即可
3. OV5640摄像头模块，支持闪光灯版本
4. SDRAM 32MB



## 板级连线

这里仅讨论和apollo板不同的部分，相同的部分不予介绍

1. 拍照按钮：H2
2. SDMMC2：PB14->SDMMC2_D0，PD6->SDMMC2_CK，PD7->SDMMC2_CMD
3. SDCARD_KEY：H3



## 功能简述

1. 支持浏览SD卡中的文件，支持文件夹查看，删除文件，删除文件夹。
2. 支持查看SD卡中的图片文件，支持大小图查看。
3. 支持查看SD卡中的图片文件的基本信息，包括分辨率，文件大小，像素格式。
4. 支持多种拍照分辨率，最高300万。
5. 支持调整拍照参数，包括色偏，夜间模式，特效等OV5640自带模式
6. 支持自定义拍照文件的路径和文件名字
7. 支持闪光灯
8. 支持SD卡拔出（需要手动弹出，且不会自动检测插入）



