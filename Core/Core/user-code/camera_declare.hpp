#pragma once

#include "prj_header.hpp"

enum class camera_resolution {
    reso_max_resolution,
    reso_1080p,
    reso_vga, // 640*480
};

enum class camera_format {
    format_RGB,
    format_YCbCr,
    format_JPEG
};

enum class message_process : uint32_t {
    ERROR_STOP  = 1 << 0,
    CANNOT_GET  = 1 << 1,
    DMA_ABORT   = 1 << 2,
    MDMA_ABORT  = 1 << 3,
    OK          = 1 << 4,
    CAPTURE_END = 1 << 5,
    DATA_END    = 1 << 6
};

struct Camera_DCMI_Data {
    DCMI_HandleTypeDef instance;
    DMA_HandleTypeDef  first_stage_dma;
    MDMA_HandleTypeDef second_stage_dma;
    //
    uintptr_t double_buffer_first;
    uintptr_t double_buffer_second;
    uintptr_t final_buffer;
    size_t    final_buffer_len;
    size_t    half_middle_buffer_words;
    size_t    double_buffer_transmit_count;
    size_t    per_block_word;
    size_t    block_count;
    bool      single_buffer_fine;
    bool      dma_not_full;
    //
    EventGroupHandle_t eg;
};

struct Camera_DCMI_HandleType {
    Camera_DCMI_Data                  data;
    std::vector<MDMA_LinkNodeTypeDef> the_node_list;
};

extern OV5640_IO_t             ov5640_io;
extern OV5640_Object_t         ov5640;
extern soft_sccb_handle        my_sccb;
extern Camera_DCMI_HandleType *target_dcmi;

extern camera_resolution       current_resolution;
extern camera_format           current_format;
extern bool                    PCF8574_init;

extern Camera_DCMI_HandleType  RGB_hdcmi;   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
extern Camera_DCMI_HandleType  YCbCr_hdcmi; // YCbCr(4:2:2) => Encode Inside(JPEG)
extern Camera_DCMI_HandleType  JPEG_hdcmi;  // JPEG(4:2:2)

//
void    dcmi_rgb_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void    dcmi_ycbcr_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void    dcmi_jpeg_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void    dcmi_msp_deinit(DCMI_HandleTypeDef *dcmiHandle);
void    dcmi_data_structure_init();
void    dcmi_io_deinit_ov5640();
int32_t ov5640_init();
int32_t ov5640_deinit();
void    OV5640_PWDN_Set(uint8_t sta);
int32_t ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length);
int32_t ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length);
//
extern "C" void DCMI_IRQHandler(void);

LV_FONT_DECLARE(photo_folder_setting)
#define OV5640_ADDR          0x78
#define OV5640_RST(n)        (HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, (n) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define DCMI_PWDN_IO         2
#define MY_TAKE_PHOTO_SYMBOL "\xE2\x97\x89"

#define ALINTEK_BOARD

#define FIRST_STAGE_DMA_ERROR        0b000000000001u
#define FIRST_STAGE_DMA_ABORT        0b000000000010u
#define FIRST_STAGE_DMA_CPLT         0b000000000100u
#define FIRST_STAGE_DMA_M1_CPLT      0b000000001000u
#define FIRST_STAGE_DMA_MASK         0b000000001111u

#define SECOND_STAGE_DMA_ERROR       0b000000010000u
#define SECOND_STAGE_DMA_ABORT       0b000000100000u
#define SECOND_STAGE_DMA_CPLT        0b000001000000u
#define SECOND_STAGE_DMA_REPEAT_CPLT 0b000010000000u
#define SECOND_STAGE_DMA_MASK        0b000011110000u

#define CAMERA_CAPTURE_OK            0b000100000000u
#define CAMERA_CAPTURE_ERROR         0b001000000000u
#define CAMERA_CAPTURE_MASK          0b001100000000u

#define CAMERA_CAPTURE_ALL_MASK      ((FIRST_STAGE_DMA_MASK) | (SECOND_STAGE_DMA_MASK) | (CAMERA_CAPTURE_MASK))
