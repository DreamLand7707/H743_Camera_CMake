#pragma once

#include <timer_delay.hpp>
#include <timer2_delay.hpp>
#include "main.h"
#include "dma2d.h"
#include "jpeg.h"
#include "ltdc.h"
#include "memorymap.h"
#include "sdmmc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "fmc.h"
#include "dcmi.h"
#include "fatfs.h"
#include "cmsis_os.h"

#include "lvgl.h"
#include "ansi.h"
#include "lvgl/examples/porting/lv_port_disp.h"
#include "lvgl/examples/porting/lv_port_indev.h"
#include "lvgl/examples/porting/lv_port_fs.h"
#include "lcd_touch.hpp"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <memory>
#include <string>
#include "cpp_public.hpp"
#include "ov5640.h"
#include "ov5640_reg.h"
#include "soft_sccb.hpp"
#include "pcf8574.hpp"

enum class manage_command_type : uint32_t {
    reload,
    decode_complete,
    terminate,
    full_picture,
    small_picture,
    ins_card,
    pop_card,
    to_camera,
    to_file_explorer
};

enum class lvgl_command_type : uint32_t {
    reload,
    nothing,
    full_picture,
    small_picture,
    ins_card,
    pop_card,
    to_camera,
    to_file_explorer
};

struct jpeg_output_buffer_req {
    uint8_t *data_out;
    uint32_t length;
};

struct main_manage_command {
    size_t              ref;
    std::string        *path;
    TimeOut_t           decode_time;
    manage_command_type type;
};

struct lvgl_manage_command {
    lvgl_command_type type;
};

BaseType_t                send_command_to_main_manage(std::string *path, size_t ref_cnt, manage_command_type type, BaseType_t time);
void                      picture_scaling(const void *src, void *dst, uint32_t src_w, uint32_t src_h, uint32_t &dst_w, uint32_t &dst_h);
extern DCMI_HandleTypeDef RGB_hdcmi;
extern DCMI_HandleTypeDef YCbCr_hdcmi;
extern DCMI_HandleTypeDef JPEG_hdcmi;
extern SemaphoreHandle_t  camera_interface_changed;
