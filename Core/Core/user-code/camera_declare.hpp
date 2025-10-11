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

extern DMA_HandleTypeDef   my_hdma_dcmi;
extern OV5640_IO_t         ov5640_io;
extern OV5640_Object_t     ov5640;
extern soft_sccb_handle    my_sccb;
extern DCMI_HandleTypeDef *target_dcmi;

extern camera_resolution   current_resolution;
extern camera_format       current_format;
extern bool                PCF8574_init;

extern DCMI_HandleTypeDef  RGB_hdcmi;   // RGB(565) => Convert(YCbCr 4:4:4) => Encode Inside(JPEG)
extern DCMI_HandleTypeDef  YCbCr_hdcmi; // YCbCr(4:2:2) => Encode Inside(JPEG)
extern DCMI_HandleTypeDef  JPEG_hdcmi;  // JPEG(4:2:2)

void                       dcmi_rgb_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void                       dcmi_ycbcr_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void                       dcmi_jpeg_mspinit(DCMI_HandleTypeDef *dcmiHandle);
void                       dcmi_msp_deinit(DCMI_HandleTypeDef *dcmiHandle);
void                       dcmi_data_structure_init();
void                       dcmi_io_deinit_ov5640();
int32_t                    ov5640_init();
int32_t                    ov5640_deinit();
void                       OV5640_PWDN_Set(uint8_t sta);
int32_t                    ov5640_write_series_reg(uint16_t address, uint16_t reg, uint8_t *data, uint16_t length);
int32_t                    ov5640_read_series_reg(uint16_t address, uint16_t reg, uint8_t *pdata, uint16_t length);
//
extern "C" void DCMI_IRQHandler(void);

LV_FONT_DECLARE(photo_folder_setting)
#define OV5640_ADDR          0x78
#define OV5640_RST(n)        (HAL_GPIO_WritePin(DCMI_RESET_GPIO_Port, DCMI_RESET_Pin, (n) ? GPIO_PIN_SET : GPIO_PIN_RESET))
#define DCMI_PWDN_IO         2
#define MY_TAKE_PHOTO_SYMBOL "\xE2\x97\x89"
