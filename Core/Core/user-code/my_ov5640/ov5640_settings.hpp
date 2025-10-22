#ifndef _OV5640_SETTINGS_H_
#define _OV5640_SETTINGS_H_

#include <cstdint>
#include "ov5640_regs.hpp"
#include "sensor.hpp"

static const ratio_settings_t ratio_table[] = {
    //  mw,   mh,  sx,  sy,   ex,   ey, ox, oy,   tx,   ty
    {2560, 1920,   0,   0, 2623, 1951, 32, 16, 2844, 1968}, //  4x3
    {2560, 1704,   0, 110, 2623, 1843, 32, 16, 2844, 1752}, //  3x2
    {2560, 1600,   0, 160, 2623, 1791, 32, 16, 2844, 1648}, //  16x10
    {2560, 1536,   0, 192, 2623, 1759, 32, 16, 2844, 1584}, //  5x3
    {2560, 1440,   0, 240, 2623, 1711, 32, 16, 2844, 1488}, //  16x9
    {2560, 1080,   0, 420, 2623, 1531, 32, 16, 2844, 1128}, //  21x9
    {2400, 1920,  80,   0, 2543, 1951, 32, 16, 2684, 1968}, //  5x4
    {1920, 1920, 320,   0, 2543, 1951, 32, 16, 2684, 1968}, //  1x1
    {1088, 1920, 736,   0, 1887, 1951, 32, 16, 1884, 1968}  //  9x16
};

#define REG_DLY      0xffff
#define REGLIST_TAIL 0x0000
static const uint16_t OV5640_Common[][2] = {
    {    OV5640_SCCB_SYSTEM_CTRL1, 0x11},
    {        OV5640_SYSTEM_CTROL0, 0x82},
    {    OV5640_SCCB_SYSTEM_CTRL1, 0x03},
    {                      0x3630, 0x36},
    {                      0x3631, 0x0e},
    {                      0x3632, 0xe2},
    {                      0x3633, 0x12},
    {                      0x3621, 0xe0},
    {                      0x3704, 0xa0},
    {                      0x3703, 0x5a},
    {                      0x3715, 0x78},
    {                      0x3717, 0x01},
    {                      0x370b, 0x60},
    {                      0x3705, 0x1a},
    {                      0x3905, 0x02},
    {                      0x3906, 0x10},
    {                      0x3901, 0x0a},
    {                      0x3731, 0x12},
    {                      0x3600, 0x08},
    {                      0x3601, 0x33},
    {                      0x302d, 0x60},
    {                      0x3620, 0x52},
    {                      0x371b, 0x20},
    {                      0x471c, 0x50},
    {           OV5640_AEC_CTRL13, 0x43},
    {OV5640_AEC_GAIN_CEILING_HIGH, 0x00},
    { OV5640_AEC_GAIN_CEILING_LOW, 0xf8},
    {                      0x3635, 0x13},
    {                      0x3636, 0x03},
    {                      0x3634, 0x40},
    {                      0x3622, 0x01},
    {        OV5640_5060HZ_CTRL01, 0x34},
    {        OV5640_5060HZ_CTRL04, 0x28},
    {        OV5640_5060HZ_CTRL05, 0x98},
    {  OV5640_LIGHTMETER1_TH_HIGH, 0x00},
    {   OV5640_LIGHTMETER1_TH_LOW, 0x00},
    {  OV5640_LIGHTMETER2_TH_HIGH, 0x01},
    {   OV5640_LIGHTMETER2_TH_LOW, 0x2c},
    {   OV5640_SAMPLE_NUMBER_HIGH, 0x9c},
    {    OV5640_SAMPLE_NUMBER_LOW, 0x40},
    {      OV5640_TIMING_TC_REG20, 0x06},
    {      OV5640_TIMING_TC_REG21, 0x00},
    {         OV5640_TIMING_X_INC, 0x31},
    {         OV5640_TIMING_Y_INC, 0x31},
    {       OV5640_TIMING_HS_HIGH, 0x00},
    {        OV5640_TIMING_HS_LOW, 0x00},
    {       OV5640_TIMING_VS_HIGH, 0x00},
    {        OV5640_TIMING_VS_LOW, 0x04},
    {       OV5640_TIMING_HW_HIGH, 0x0a},
    {        OV5640_TIMING_HW_LOW, 0x3f},
    {       OV5640_TIMING_VH_HIGH, 0x07},
    {        OV5640_TIMING_VH_LOW, 0x9b},
    {    OV5640_TIMING_DVPHO_HIGH, 0x03},
    {     OV5640_TIMING_DVPHO_LOW, 0x20},
    {    OV5640_TIMING_DVPVO_HIGH, 0x02},
    {     OV5640_TIMING_DVPVO_LOW, 0x58},
    /* For 800x480 resolution: OV5640_TIMING_HTS=0x790, OV5640_TIMING_VTS=0x440 */
    {      OV5640_TIMING_HTS_HIGH, 0x07},
    {       OV5640_TIMING_HTS_LOW, 0x90},
    {      OV5640_TIMING_VTS_HIGH, 0x04},
    {       OV5640_TIMING_VTS_LOW, 0x40},
    {  OV5640_TIMING_HOFFSET_HIGH, 0x00},
    {   OV5640_TIMING_HOFFSET_LOW, 0x10},
    {  OV5640_TIMING_VOFFSET_HIGH, 0x00},
    {   OV5640_TIMING_VOFFSET_LOW, 0x06},
    {                      0x3618, 0x00},
    {                      0x3612, 0x29},
    {                      0x3708, 0x64},
    {                      0x3709, 0x52},
    {                      0x370c, 0x03},
    {           OV5640_AEC_CTRL02, 0x03},
    {           OV5640_AEC_CTRL03, 0xd8},
    {    OV5640_AEC_B50_STEP_HIGH, 0x01},
    {     OV5640_AEC_B50_STEP_LOW, 0x27},
    {    OV5640_AEC_B60_STEP_HIGH, 0x00},
    {     OV5640_AEC_B60_STEP_LOW, 0xf6},
    {           OV5640_AEC_CTRL0E, 0x03},
    {           OV5640_AEC_CTRL0D, 0x04},
    {    OV5640_AEC_MAX_EXPO_HIGH, 0x03},
    {     OV5640_AEC_MAX_EXPO_LOW, 0xd8},
    {           OV5640_BLC_CTRL01, 0x02},
    {           OV5640_BLC_CTRL04, 0x02},
    {       OV5640_SYSREM_RESET00, 0x00},
    {       OV5640_SYSREM_RESET02, 0x1c},
    {       OV5640_CLOCK_ENABLE00, 0xff},
    {       OV5640_CLOCK_ENABLE02, 0xc3},
    {       OV5640_MIPI_CONTROL00, 0x58},
    {                      0x302e, 0x00},
    {        OV5640_POLARITY_CTRL, 0x22},
    {        OV5640_FORMAT_CTRL00, 0x6F},
    {      OV5640_FORMAT_MUX_CTRL, 0x01},
    {      OV5640_JPG_MODE_SELECT, 0x03},
    {          OV5640_JPEG_CTRL07, 0x04},
    {                      0x440e, 0x00},
    {                      0x460b, 0x35},
    {                      0x460c, 0x23},
    {          OV5640_PCLK_PERIOD, 0x22},
    {                      0x3824, 0x02},
    {        OV5640_ISP_CONTROL00, 0xa7},
    {        OV5640_ISP_CONTROL01, 0xa3},
    {           OV5640_AWB_CTRL00, 0xff},
    {           OV5640_AWB_CTRL01, 0xf2},
    {           OV5640_AWB_CTRL02, 0x00},
    {           OV5640_AWB_CTRL03, 0x14},
    {           OV5640_AWB_CTRL04, 0x25},
    {           OV5640_AWB_CTRL05, 0x24},
    {           OV5640_AWB_CTRL06, 0x09},
    {           OV5640_AWB_CTRL07, 0x09},
    {           OV5640_AWB_CTRL08, 0x09},
    {           OV5640_AWB_CTRL09, 0x75},
    {           OV5640_AWB_CTRL10, 0x54},
    {           OV5640_AWB_CTRL11, 0xe0},
    {           OV5640_AWB_CTRL12, 0xb2},
    {           OV5640_AWB_CTRL13, 0x42},
    {           OV5640_AWB_CTRL14, 0x3d},
    {           OV5640_AWB_CTRL15, 0x56},
    {           OV5640_AWB_CTRL16, 0x46},
    {           OV5640_AWB_CTRL17, 0xf8},
    {           OV5640_AWB_CTRL18, 0x04},
    {           OV5640_AWB_CTRL19, 0x70},
    {           OV5640_AWB_CTRL20, 0xf0},
    {           OV5640_AWB_CTRL21, 0xf0},
    {           OV5640_AWB_CTRL22, 0x03},
    {           OV5640_AWB_CTRL23, 0x01},
    {           OV5640_AWB_CTRL24, 0x04},
    {           OV5640_AWB_CTRL25, 0x12},
    {           OV5640_AWB_CTRL26, 0x04},
    {           OV5640_AWB_CTRL27, 0x00},
    {           OV5640_AWB_CTRL28, 0x06},
    {           OV5640_AWB_CTRL29, 0x82},
    {           OV5640_AWB_CTRL30, 0x38},
    {                 OV5640_CMX1, 0x1e},
    {                 OV5640_CMX2, 0x5b},
    {                 OV5640_CMX3, 0x08},
    {                 OV5640_CMX4, 0x0a},
    {                 OV5640_CMX5, 0x7e},
    {                 OV5640_CMX6, 0x88},
    {                 OV5640_CMX7, 0x7c},
    {                 OV5640_CMX8, 0x6c},
    {                 OV5640_CMX9, 0x10},
    {         OV5640_CMXSIGN_HIGH, 0x01},
    {          OV5640_CMXSIGN_LOW, 0x98},
    {    OV5640_CIP_SHARPENMT_TH1, 0x08},
    {    OV5640_CIP_SHARPENMT_TH2, 0x30},
    {OV5640_CIP_SHARPENMT_OFFSET1, 0x10},
    {OV5640_CIP_SHARPENMT_OFFSET2, 0x00},
    {          OV5640_CIP_DNS_TH1, 0x08},
    {          OV5640_CIP_DNS_TH2, 0x30},
    {      OV5640_CIP_DNS_OFFSET1, 0x08},
    {      OV5640_CIP_DNS_OFFSET2, 0x16},
    {             OV5640_CIP_CTRL, 0x08},
    {    OV5640_CIP_SHARPENTH_TH1, 0x30},
    {    OV5640_CIP_SHARPENTH_TH2, 0x04},
    {OV5640_CIP_SHARPENTH_OFFSET1, 0x06},
    {         OV5640_GAMMA_CTRL00, 0x01},
    {          OV5640_GAMMA_YST00, 0x08},
    {          OV5640_GAMMA_YST01, 0x14},
    {          OV5640_GAMMA_YST02, 0x28},
    {          OV5640_GAMMA_YST03, 0x51},
    {          OV5640_GAMMA_YST04, 0x65},
    {          OV5640_GAMMA_YST05, 0x71},
    {          OV5640_GAMMA_YST06, 0x7d},
    {          OV5640_GAMMA_YST07, 0x87},
    {          OV5640_GAMMA_YST08, 0x91},
    {          OV5640_GAMMA_YST09, 0x9a},
    {          OV5640_GAMMA_YST0A, 0xaa},
    {          OV5640_GAMMA_YST0B, 0xb8},
    {          OV5640_GAMMA_YST0C, 0xcd},
    {          OV5640_GAMMA_YST0D, 0xdd},
    {          OV5640_GAMMA_YST0E, 0xea},
    {          OV5640_GAMMA_YST0F, 0x1d},
    {            OV5640_SDE_CTRL0, 0x02},
    {            OV5640_SDE_CTRL3, 0x40},
    {            OV5640_SDE_CTRL4, 0x10},
    {            OV5640_SDE_CTRL9, 0x10},
    {           OV5640_SDE_CTRL10, 0x00},
    {           OV5640_SDE_CTRL11, 0xf8},
    {              OV5640_GMTRX00, 0x23},
    {              OV5640_GMTRX01, 0x14},
    {              OV5640_GMTRX02, 0x0f},
    {              OV5640_GMTRX03, 0x0f},
    {              OV5640_GMTRX04, 0x12},
    {              OV5640_GMTRX05, 0x26},
    {              OV5640_GMTRX10, 0x0c},
    {              OV5640_GMTRX11, 0x08},
    {              OV5640_GMTRX12, 0x05},
    {              OV5640_GMTRX13, 0x05},
    {              OV5640_GMTRX14, 0x08},
    {              OV5640_GMTRX15, 0x0d},
    {              OV5640_GMTRX20, 0x08},
    {              OV5640_GMTRX21, 0x03},
    {              OV5640_GMTRX22, 0x00},
    {              OV5640_GMTRX23, 0x00},
    {              OV5640_GMTRX24, 0x03},
    {              OV5640_GMTRX25, 0x09},
    {              OV5640_GMTRX30, 0x07},
    {              OV5640_GMTRX31, 0x03},
    {              OV5640_GMTRX32, 0x00},
    {              OV5640_GMTRX33, 0x01},
    {              OV5640_GMTRX34, 0x03},
    {              OV5640_GMTRX35, 0x08},
    {              OV5640_GMTRX40, 0x0d},
    {              OV5640_GMTRX41, 0x08},
    {              OV5640_GMTRX42, 0x05},
    {              OV5640_GMTRX43, 0x06},
    {              OV5640_GMTRX44, 0x08},
    {              OV5640_GMTRX45, 0x0e},
    {              OV5640_GMTRX50, 0x29},
    {              OV5640_GMTRX51, 0x17},
    {              OV5640_GMTRX52, 0x11},
    {              OV5640_GMTRX53, 0x11},
    {              OV5640_GMTRX54, 0x15},
    {              OV5640_GMTRX55, 0x28},
    {            OV5640_BRMATRX00, 0x46},
    {            OV5640_BRMATRX01, 0x26},
    {            OV5640_BRMATRX02, 0x08},
    {            OV5640_BRMATRX03, 0x26},
    {            OV5640_BRMATRX04, 0x64},
    {            OV5640_BRMATRX05, 0x26},
    {            OV5640_BRMATRX06, 0x24},
    {            OV5640_BRMATRX07, 0x22},
    {            OV5640_BRMATRX08, 0x24},
    {            OV5640_BRMATRX09, 0x24},
    {            OV5640_BRMATRX20, 0x06},
    {            OV5640_BRMATRX21, 0x22},
    {            OV5640_BRMATRX22, 0x40},
    {            OV5640_BRMATRX23, 0x42},
    {            OV5640_BRMATRX24, 0x24},
    {            OV5640_BRMATRX30, 0x26},
    {            OV5640_BRMATRX31, 0x24},
    {            OV5640_BRMATRX32, 0x22},
    {            OV5640_BRMATRX33, 0x22},
    {            OV5640_BRMATRX34, 0x26},
    {            OV5640_BRMATRX40, 0x44},
    {            OV5640_BRMATRX41, 0x24},
    {            OV5640_BRMATRX42, 0x26},
    {            OV5640_BRMATRX43, 0x28},
    {            OV5640_BRMATRX44, 0x42},
    {       OV5640_LENC_BR_OFFSET, 0xce},
    {                      0x5025, 0x00},
    {           OV5640_AEC_CTRL0F, 0x30},
    {           OV5640_AEC_CTRL10, 0x28},
    {           OV5640_AEC_CTRL1B, 0x30},
    {           OV5640_AEC_CTRL1E, 0x26},
    {           OV5640_AEC_CTRL11, 0x60},
    {           OV5640_AEC_CTRL1F, 0x14},
    {        OV5640_SYSTEM_CTROL0, 0x02},
};
static const uint16_t sensor_default_regs[][2] = {
    {    SYSTEM_CTROL0, 0x82}, // software reset
    {          REG_DLY,   10}, // delay 10ms
    {    SYSTEM_CTROL0, 0x42}, // power down

    // enable pll
    {           0x3103, 0x13},

    // io direction
    {           0x3017, 0xff},
    {           0x3018, 0xff},

    { DRIVE_CAPABILITY, 0xc3},
    {CLOCK_POL_CONTROL, 0x22},

    {           0x4713, 0x02}, //  jpg mode select

    {   ISP_CONTROL_01, 0x83}, // turn color matrix, awb and SDE

    // sys reset
    {           0x3000, 0x20}, // reset MCU
    {          REG_DLY,   10}, // delay 10ms
    {           0x3002, 0x1c},

    // clock enable
    {           0x3004, 0xff},
    {           0x3006, 0xc3},

    // isp control
    {           0x5000, 0xa7},
    {   ISP_CONTROL_01, 0xa3}, //+scaling?
    {           0x5003, 0x08}, //  special_effect

    // unknown
    {           0x370c, 0x02}, //!!IMPORTANT
    {           0x3634, 0x40}, //!!IMPORTANT

    // AEC/AGC
    {           0x3a02, 0x03},
    {           0x3a03, 0xd8},
    {           0x3a08, 0x01},
    {           0x3a09, 0x27},
    {           0x3a0a, 0x00},
    {           0x3a0b, 0xf6},
    {           0x3a0d, 0x04},
    {           0x3a0e, 0x03},
    {           0x3a0f, 0x30}, //  ae_level
    {           0x3a10, 0x28}, //  ae_level
    {           0x3a11, 0x60}, //  ae_level
    {           0x3a13, 0x43},
    {           0x3a14, 0x03},
    {           0x3a15, 0xd8},
    {           0x3a18, 0x00}, //  gainceiling
    {           0x3a19, 0xf8}, //  gainceiling
    {           0x3a1b, 0x30}, //  ae_level
    {           0x3a1e, 0x26}, //  ae_level
    {           0x3a1f, 0x14}, //  ae_level

    // vcm debug
    {           0x3600, 0x08},
    {           0x3601, 0x33},

    // 50/60Hz
    {           0x3c01, 0xa4},
    {           0x3c04, 0x28},
    {           0x3c05, 0x98},
    {           0x3c06, 0x00},
    {           0x3c07, 0x08},
    {           0x3c08, 0x00},
    {           0x3c09, 0x1c},
    {           0x3c0a, 0x9c},
    {           0x3c0b, 0x40},

    {           0x460c, 0x22}, //  disable jpeg footer

    // BLC
    {           0x4001, 0x02},
    {           0x4004, 0x02},

    // AWB
    {           0x5180, 0xff},
    {           0x5181, 0xf2},
    {           0x5182, 0x00},
    {           0x5183, 0x14},
    {           0x5184, 0x25},
    {           0x5185, 0x24},
    {           0x5186, 0x09},
    {           0x5187, 0x09},
    {           0x5188, 0x09},
    {           0x5189, 0x75},
    {           0x518a, 0x54},
    {           0x518b, 0xe0},
    {           0x518c, 0xb2},
    {           0x518d, 0x42},
    {           0x518e, 0x3d},
    {           0x518f, 0x56},
    {           0x5190, 0x46},
    {           0x5191, 0xf8},
    {           0x5192, 0x04},
    {           0x5193, 0x70},
    {           0x5194, 0xf0},
    {           0x5195, 0xf0},
    {           0x5196, 0x03},
    {           0x5197, 0x01},
    {           0x5198, 0x04},
    {           0x5199, 0x12},
    {           0x519a, 0x04},
    {           0x519b, 0x00},
    {           0x519c, 0x06},
    {           0x519d, 0x82},
    {           0x519e, 0x38},

    // color matrix (Saturation)
    {           0x5381, 0x1e},
    {           0x5382, 0x5b},
    {           0x5383, 0x08},
    {           0x5384, 0x0a},
    {           0x5385, 0x7e},
    {           0x5386, 0x88},
    {           0x5387, 0x7c},
    {           0x5388, 0x6c},
    {           0x5389, 0x10},
    {           0x538a, 0x01},
    {           0x538b, 0x98},

    // CIP control (Sharpness)
    {           0x5300, 0x08}, // CIP sharpen MT threshold 1
    {           0x5301, 0x30}, // CIP sharpen MT threshold 2
    {           0x5302, 0x10}, // CIP sharpen MT offset 1
    {           0x5303, 0x00}, // CIP sharpen MT offset 2
    {           0x5304, 0x08}, // CIP DNS threshold 1
    {           0x5305, 0x30}, // CIP DNS threshold 2
    {           0x5306, 0x08}, // CIP DNS offset 1
    {           0x5307, 0x16}, // CIP DNS offset 2
    {           0x5309, 0x08}, // CIP sharpen TH threshold 1
    {           0x530a, 0x30}, // CIP sharpen TH threshold 2
    {           0x530b, 0x04}, // CIP sharpen TH offset 1
    {           0x530c, 0x06}, // CIP sharpen TH offset 2

    // GAMMA
    {           0x5480, 0x01},
    {           0x5481, 0x00},
    {           0x5482, 0x1e},
    {           0x5483, 0x3b},
    {           0x5484, 0x58},
    {           0x5485, 0x66},
    {           0x5486, 0x71},
    {           0x5487, 0x7d},
    {           0x5488, 0x83},
    {           0x5489, 0x8f},
    {           0x548a, 0x98},
    {           0x548b, 0xa6},
    {           0x548c, 0xb8},
    {           0x548d, 0xca},
    {           0x548e, 0xd7},
    {           0x548f, 0xe3},
    {           0x5490, 0x1d},

    // Special Digital Effects (SDE) (UV adjust)
    {           0x5580, 0x06}, //  enable brightness and contrast
    {           0x5583, 0x40}, //  special_effect
    {           0x5584, 0x10}, //  special_effect
    {           0x5586, 0x20}, //  contrast
    {           0x5587, 0x00}, //  brightness
    {           0x5588, 0x00}, //  brightness
    {           0x5589, 0x10},
    {           0x558a, 0x00},
    {           0x558b, 0xf8},
    {           0x501d, 0x40}, // enable manual offset of contrast

    // power on
    {           0x3008, 0x02},

    // 50Hz
    {           0x3c00, 0x04},

    {          REG_DLY,  300},
    {     REGLIST_TAIL, 0x00}, // tail
};

static const uint16_t sensor_fmt_jpeg[][2] = {
    {  FORMAT_CTRL, 0x00}, // YUV422
    {FORMAT_CTRL00, 0x30}, // YUYV
    {       0x3002, 0x00}, //  0x1c to 0x00 !!!
    {       0x3006, 0xff}, //  0xc3 to 0xff !!!
    {       0x471c, 0x50}, //  0xd0 to 0x50 !!!
    { REGLIST_TAIL, 0x00}, // tail
};

static const uint16_t sensor_fmt_raw[][2] = {
    {  FORMAT_CTRL, 0x03}, // RAW (DPC)
    {FORMAT_CTRL00, 0x00}, // RAW
    { REGLIST_TAIL, 0x00}
};

static const uint16_t sensor_fmt_grayscale[][2] = {
    {  FORMAT_CTRL, 0x00}, // YUV422
    {FORMAT_CTRL00, 0x10}, // Y8
    { REGLIST_TAIL, 0x00}
};

static const uint16_t sensor_fmt_yuv422[][2] = {
    {  FORMAT_CTRL, 0x00}, // YUV422
    {FORMAT_CTRL00, 0x30}, // YUYV
    { REGLIST_TAIL, 0x00}
};

static const uint16_t sensor_fmt_rgb565[][2] = {
    {  FORMAT_CTRL, 0x01}, // RGB
    {FORMAT_CTRL00, 0x6F}, // RGB565 (BGR)
    { REGLIST_TAIL, 0x00}
};

static const uint8_t sensor_saturation_levels[9][11] = {
    {0x1d, 0x60, 0x03, 0x07, 0x48, 0x4f, 0x4b, 0x40, 0x0b, 0x01, 0x98}, //-4
    {0x1d, 0x60, 0x03, 0x08, 0x54, 0x5c, 0x58, 0x4b, 0x0d, 0x01, 0x98}, //-3
    {0x1d, 0x60, 0x03, 0x0a, 0x60, 0x6a, 0x64, 0x56, 0x0e, 0x01, 0x98}, //-2
    {0x1d, 0x60, 0x03, 0x0b, 0x6c, 0x77, 0x70, 0x60, 0x10, 0x01, 0x98}, //-1
    {0x1d, 0x60, 0x03, 0x0c, 0x78, 0x84, 0x7d, 0x6b, 0x12, 0x01, 0x98}, //  0
    {0x1d, 0x60, 0x03, 0x0d, 0x84, 0x91, 0x8a, 0x76, 0x14, 0x01, 0x98}, //+1
    {0x1d, 0x60, 0x03, 0x0e, 0x90, 0x9e, 0x96, 0x80, 0x16, 0x01, 0x98}, //+2
    {0x1d, 0x60, 0x03, 0x10, 0x9c, 0xac, 0xa2, 0x8b, 0x17, 0x01, 0x98}, //+3
    {0x1d, 0x60, 0x03, 0x11, 0xa8, 0xb9, 0xaf, 0x96, 0x19, 0x01, 0x98}, //+4
};

static const uint8_t sensor_special_effects[7][4] = {
    {0x06, 0x40, 0x2c, 0x08}, //  Normal
    {0x46, 0x40, 0x28, 0x08}, //  Negative
    {0x1e, 0x80, 0x80, 0x08}, //  Grayscale
    {0x1e, 0x80, 0xc0, 0x08}, //  Red Tint
    {0x1e, 0x60, 0x60, 0x08}, //  Green Tint
    {0x1e, 0xa0, 0x40, 0x08}, //  Blue Tint
    {0x1e, 0x40, 0xa0, 0x08}, //  Sepia
};

static const uint16_t sensor_regs_gamma0[][2] = {
    {0x5480, 0x01},
    {0x5481, 0x08},
    {0x5482, 0x14},
    {0x5483, 0x28},
    {0x5484, 0x51},
    {0x5485, 0x65},
    {0x5486, 0x71},
    {0x5487, 0x7d},
    {0x5488, 0x87},
    {0x5489, 0x91},
    {0x548a, 0x9a},
    {0x548b, 0xaa},
    {0x548c, 0xb8},
    {0x548d, 0xcd},
    {0x548e, 0xdd},
    {0x548f, 0xea},
    {0x5490, 0x1d}
};

static const uint16_t sensor_regs_gamma1[][2] = {
    {0x5480,  0x1},
    {0x5481,  0x0},
    {0x5482, 0x1e},
    {0x5483, 0x3b},
    {0x5484, 0x58},
    {0x5485, 0x66},
    {0x5486, 0x71},
    {0x5487, 0x7d},
    {0x5488, 0x83},
    {0x5489, 0x8f},
    {0x548a, 0x98},
    {0x548b, 0xa6},
    {0x548c, 0xb8},
    {0x548d, 0xca},
    {0x548e, 0xd7},
    {0x548f, 0xe3},
    {0x5490, 0x1d}
};

static const uint16_t sensor_regs_awb0[][2] = {
    {0x5180, 0xff},
    {0x5181, 0xf2},
    {0x5182, 0x00},
    {0x5183, 0x14},
    {0x5184, 0x25},
    {0x5185, 0x24},
    {0x5186, 0x09},
    {0x5187, 0x09},
    {0x5188, 0x09},
    {0x5189, 0x75},
    {0x518a, 0x54},
    {0x518b, 0xe0},
    {0x518c, 0xb2},
    {0x518d, 0x42},
    {0x518e, 0x3d},
    {0x518f, 0x56},
    {0x5190, 0x46},
    {0x5191, 0xf8},
    {0x5192, 0x04},
    {0x5193, 0x70},
    {0x5194, 0xf0},
    {0x5195, 0xf0},
    {0x5196, 0x03},
    {0x5197, 0x01},
    {0x5198, 0x04},
    {0x5199, 0x12},
    {0x519a, 0x04},
    {0x519b, 0x00},
    {0x519c, 0x06},
    {0x519d, 0x82},
    {0x519e, 0x38}
};

#endif
