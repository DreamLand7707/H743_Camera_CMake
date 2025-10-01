#include "prj_header.hpp"
#include "jpeg_utils.h"
#include "file_explorer.hpp"

#include <ranges>

// extern Variable Definition
void *SDRAM_GRAM1;
void *SDRAM_GRAM2;
// void             *JPEG_ENCODE_DEST;
// void             *JEPG_YCbCr;
// void             *JPEG_ENCODE_SOURCE;

uint8_t           mkfs_buffer[4096] __attribute__((section(".fatfs_buffer")));
uint32_t          sdcard_link_driver = 0;
uint32_t          sdcard_disk_init   = 0;
uint32_t          sdcard_is_mounted  = 0;
uint32_t          sdcard_initialized = 0;

SemaphoreHandle_t sema_flash_screen_routine_start;
SemaphoreHandle_t sema_camera_routine_start;
SemaphoreHandle_t sema_swap_buffer_handle;

// traceString       trace_analyzer_channel1;
// traceString       trace_analyzer_channel2;
// traceString       trace_analyzer_channel3;
// traceString       trace_analyzer_channel4;


// static variable Definition
namespace
{
    TaskHandle_t      take_screenshot;
    SemaphoreHandle_t mutex_gram_read;
    // SemaphoreHandle_t sema_take_screenshot;

    JPEG_ConfTypeDef jpeg_conf;
    uint8_t         *curr_encode_ptr;
    uint8_t         *target_encode_ptr;
    uint8_t         *curr_dest_ptr;
    FIL              jpeg1;

    void            *curr_screen_buffer;

} // namespace

// extern Variable Decl
LV_FONT_DECLARE(Camera_1_bit)

// MARCO
#define MY_CAMERA_SYMBOL "\xEF\x80\xB0"
#define sd_enable        ((sdcard_disk_init) & (sdcard_is_mounted) & (sdcard_link_driver))

// type decl
enum class scr_mess;

// static function decl
static void    lvgl_initialize_port1();
static void    lvgl_initialize_port2();
static void    sdcard_initialize();
static int     routine(int argc, char **argv);
static void    initial_before_routine();
extern "C" int _getentropy(void *buffer, size_t length);

// static void    hdma2dCompleteCallback(DMA2D_HandleTypeDef *hdma2d);
static void insbtn_call(lv_event_t *event);
static void popbtn_call(lv_event_t *event);
static void ins_card_call();
static void pop_card_call();
static void lvgl_create_main_interface();
static void change_to_camera(lv_event_t *event);
static void click_one_file(lv_event_t *e, const char *path);

enum class scr_mess {
    INFO = 0,
    WARN = 1,
    ERRO = 2,
    ALL  = 3
};


/*
        TRUE TASK
*/

// task1
void initial_task_routine(void const *argument) {
    initial_before_routine();
    JPEG_InitColorTables();
    routine(0, nullptr);
}

/*
        Internal Routine
*/

static void initial_before_routine() {
    mutex_gram_read = xSemaphoreCreateMutex();
    // sema_take_screenshot = xSemaphoreCreateBinary();
    // sema_update_local    = xSemaphoreCreateBinary();

    SDRAM_GRAM1 = sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));
    SDRAM_GRAM2 = sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));

    HAL_LTDC_SetAddress(&hltdc, (uint32_t)SDRAM_GRAM1, LTDC_LAYER_1);

    // hdma2d.XferCpltCallback     = hdma2dCompleteCallback;
    // jpeg_conf.ChromaSubsampling = JPEG_444_SUBSAMPLING;
    // jpeg_conf.ColorSpace        = JPEG_YCBCR_COLORSPACE;
    // jpeg_conf.ImageQuality      = 100;
    // jpeg_conf.ImageHeight       = 272;
    // jpeg_conf.ImageWidth        = 480;
    // HAL_JPEG_ConfigEncoding(&hjpeg, &jpeg_conf);
}

static int routine(int argc, char **argv) {
    lvgl_initialize_port1();
    sdcard_initialize();
    lvgl_initialize_port2();

    xSemaphoreGive(sema_flash_screen_routine_start);
    xSemaphoreGive(sema_camera_routine_start);

    vTaskPrioritySet(Initial_TaskHandle, 4);

    vQueueAddToRegistry((QueueHandle_t)mutex_gram_read, "mutex_gram_read");
    // vQueueAddToRegistry((QueueHandle_t)sema_take_screenshot, "sema_take_screenshot");

    // trace_analyzer_channel1 = xTraceRegisterString("User_Channel_1");
    // trace_analyzer_channel2 = xTraceRegisterString("User_Channel_2");
    // trace_analyzer_channel3 = xTraceRegisterString("User_Channel_3");
    // trace_analyzer_channel4 = xTraceRegisterString("User_Channel_4");

    // xTaskCreate(take_screenshot_task, "thread_take_screen_shot", 512, nullptr, 5, &take_screenshot);

    lvgl_create_main_interface();

    while (1) {
        // xTracePrint(trace_analyzer_channel2, "=Main Thread= Begin lv_timer_handler");
        lv_timer_handler();
        // xTracePrint(trace_analyzer_channel2, "=Main Thread= End   lv_timer_handler");
        vTaskDelay(1);
    }
}

namespace
{
    lv_obj_t *screen_container;

    lv_obj_t *file_explorer_obj;
    lv_obj_t *right_container;

    lv_obj_t *image_container;
    lv_obj_t *button_container;

    lv_obj_t *image;
    lv_obj_t *image_indicator;
    lv_obj_t *image_indicator_label;

    lv_obj_t *shot_btn;
    lv_obj_t *pop_sd;
    lv_obj_t *ins_sd;
    lv_obj_t *shot_btn_label;
    lv_obj_t *pop_sd_label;
    lv_obj_t *ins_sd_label;
} // namespace

static void lvgl_create_main_interface() {
    screen_container = lv_obj_create(lv_scr_act());
    lv_obj_set_size(screen_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_layout(screen_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(screen_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(screen_container, 0, LV_STATE_DEFAULT);

    file_explorer_obj = file_explorer_create(screen_container, 40, "0:/");
    lv_obj_set_flex_grow(file_explorer_obj, 0);

    right_container = lv_obj_create(screen_container);
    lv_obj_set_style_height(right_container, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_flex_grow(right_container, 1);
    lv_obj_set_layout(right_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(right_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(right_container, 0, LV_STATE_DEFAULT);

    image_container = lv_obj_create(right_container);
    lv_obj_set_style_width(image_container, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_flex_grow(image_container, 4);
    lv_obj_set_layout(image_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(image_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(image_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(image_container, 0, LV_STATE_DEFAULT);

    button_container = lv_obj_create(right_container);
    lv_obj_set_style_width(button_container, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_flex_grow(button_container, 1);
    lv_obj_set_layout(button_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(button_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_border_width(button_container, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(button_container, 5, LV_STATE_DEFAULT);

    shot_btn = lv_btn_create(button_container);
    lv_obj_set_size(shot_btn, lv_pct(20), lv_pct(100));
    lv_obj_set_style_bg_color(shot_btn, lv_color_make(0x00, 0x77, 0x77), 0);
    lv_obj_set_flex_grow(shot_btn, 1);

    pop_sd = lv_button_create(button_container);
    lv_obj_set_size(pop_sd, lv_pct(20), lv_pct(100));
    lv_obj_set_style_bg_color(pop_sd, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    lv_obj_set_flex_grow(pop_sd, 2);

    ins_sd = lv_button_create(button_container);
    lv_obj_set_size(ins_sd, lv_pct(20), lv_pct(100));
    lv_obj_set_style_bg_color(ins_sd, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);
    lv_obj_set_flex_grow(ins_sd, 2);

    shot_btn_label = lv_label_create(shot_btn);
    lv_obj_align(shot_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(shot_btn_label, &Camera_1_bit, 0);
    lv_label_set_text(shot_btn_label, MY_CAMERA_SYMBOL);
    lv_obj_set_style_text_color(shot_btn_label, lv_color_make(0xff, 0xff, 0xff), 0);

    pop_sd_label = lv_label_create(pop_sd);
    lv_obj_align(pop_sd_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(pop_sd_label, lv_color_make(0x00, 0x00, 0x00), 0);

    ins_sd_label = lv_label_create(ins_sd);
    lv_obj_align(ins_sd_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(ins_sd_label, lv_color_make(0x00, 0x00, 0x00), 0);

    image_indicator = lv_obj_create(image_container);
    lv_obj_set_flex_grow(image_indicator, 1);
    lv_obj_set_style_width(image_indicator, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(image_indicator, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(image_indicator, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_hor(image_indicator, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_margin_all(image_indicator, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_margin_right(image_indicator, 10, LV_STATE_DEFAULT);

    image_indicator_label = lv_label_create(image_indicator);
    lv_obj_set_style_align(image_indicator_label, LV_ALIGN_LEFT_MID, LV_STATE_DEFAULT);

    image = lv_image_create(image_container);
    lv_obj_set_flex_grow(image, 4);
    lv_obj_set_style_width(image, LV_PCT(100), LV_STATE_DEFAULT);
    lv_obj_set_style_margin_right(image, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(image, 1, LV_STATE_DEFAULT);

    lv_obj_add_event_cb(shot_btn, change_to_camera, LV_EVENT_PRESSED, nullptr);
    file_explorer_set_callback(file_explorer_obj, click_one_file);
    lv_obj_add_event_cb(ins_sd, insbtn_call, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(pop_sd, popbtn_call, LV_EVENT_PRESSED, nullptr);
    if (sdcard_disk_init)
        ins_card_call();
    else
        pop_card_call();
}

static void lvgl_initialize_port1() {
    lcd_touch_initialize();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
}

static void lvgl_initialize_port2() {
    if (sdcard_disk_init)
        lv_port_fs_init();
}


static void sdcard_initialize() {
    DSTATUS ret = SD_Driver.disk_initialize(0);
    printf("STATUS: %d\n", ret);

    if (ret != RES_OK)
        return;
    sdcard_disk_init = 1;
    print_sdcard_info();

    FRESULT file_system_res;
    file_system_res = f_mount(&SDFatFS, SDPath, 1);
    if (file_system_res == FR_NO_FILESYSTEM) {
        file_system_res = f_mkfs(SDPath, FM_FAT32, 4096, (void *)mkfs_buffer, 4096);
        if (file_system_res == FR_OK) {
            file_system_res = f_mount(&SDFatFS, SDPath, 1);
        }
    }
    else if (file_system_res != FR_OK) {
    }
    sdcard_is_mounted = 1;

    f_open(&SDFile, "0:/a.txt", (uint32_t)FA_WRITE | (uint32_t)FA_CREATE_ALWAYS);
    const char *str    = "Hello world!";
    UINT        expect = strlen(str);
    UINT        get    = 0;
    f_write(&SDFile, str, expect, &get);
    f_close(&SDFile);
    // f_mount(nullptr, SDPath, 1);
    // sdcard_is_mounted = 0;
}

void print_sdcard_info(void) {
    uint64_t               CardCap; // SD卡容量
    HAL_SD_CardCIDTypeDef  SDCard_CID;
    HAL_SD_CardInfoTypeDef SDCardInfo;

    HAL_SD_GetCardCID(&hsd1, &SDCard_CID);                                              // 获取CID
    HAL_SD_GetCardInfo(&hsd1, &SDCardInfo);                                             // 获取SD卡信息
    CardCap = (uint64_t)(SDCardInfo.LogBlockNbr) * (uint64_t)(SDCardInfo.LogBlockSize); // 计算SD卡容量
    switch (SDCardInfo.CardType) {
    case CARD_SDSC: {
        if (SDCardInfo.CardVersion == CARD_V1_X)
            jprintf(0, "Card Type:SDSC V1\r\n");
        else if (SDCardInfo.CardVersion == CARD_V2_X)
            jprintf(0, "Card Type:SDSC V2\r\n");
        break;
    }
    case CARD_SDHC_SDXC: {
        jprintf(0, "Card Type:SDHC\r\n");
        break;
    }
    default:
        break;
    }

    jprintf(0, "Card ManufacturerID: %d \r\n", SDCard_CID.ManufacturerID);            // 制造商ID
    jprintf(0, "CardVersion:         %lu \r\n", (uint32_t)(SDCardInfo.CardVersion));  // 卡版本号
    jprintf(0, "Class:               %lu \r\n", (uint32_t)(SDCardInfo.Class));        //
    jprintf(0, "Card RCA(RelCardAdd):%lu \r\n", SDCardInfo.RelCardAdd);               // 卡相对地址
    jprintf(0, "Card BlockNbr:       %lu \r\n", SDCardInfo.BlockNbr);                 // 块数量
    jprintf(0, "Card BlockSize:      %lu \r\n", SDCardInfo.BlockSize);                // 块大小
    jprintf(0, "LogBlockNbr:         %lu \r\n", (uint32_t)(SDCardInfo.LogBlockNbr));  // 逻辑块数量
    jprintf(0, "LogBlockSize:        %lu \r\n", (uint32_t)(SDCardInfo.LogBlockSize)); // 逻辑块大小
    jprintf(0, "Card Capacity:       %lu MB\r\n", (uint32_t)(CardCap >> 20u));        // 卡容量
}

// ISR Callback
void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc) {
    BaseType_t woke = pdFALSE;
    xSemaphoreGiveFromISR(sema_swap_buffer_handle, &woke);
    portYIELD_FROM_ISR(woke);
}

// lvgl callback
void swapBuffer(void *passbuf, lv_display_t *disp) {
    // xTracePrint(trace_analyzer_channel1, "=SwapBuffer= Begin Swap");
    HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t)passbuf, LTDC_LAYER_1);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);
    curr_screen_buffer = passbuf;
    xSemaphoreTake(sema_swap_buffer_handle, portMAX_DELAY);
    lv_display_flush_ready(disp);
    // xTracePrint(trace_analyzer_channel1, "=SwapBuffer= End   Swap");
    // jprintf(0, "swapBuffer\n");
}

// std::cout callback
int _getentropy(void *buffer, size_t length) {
    return 0;
}

static void insbtn_call(lv_event_t *event) {
    if (!sdcard_link_driver && FATFS_LinkDriver(&SD_Driver, SDPath) != 0) {
        return;
    }
    sdcard_link_driver = 1;
    if (!sdcard_disk_init && SD_Driver.disk_initialize(0) != RES_OK) {
        HAL_SD_DeInit(&hsd1);
        return;
    }
    sdcard_disk_init = 1;
    if (!sdcard_is_mounted && f_mount(&SDFatFS, SDPath, 1) != FR_OK) {
        return;
    }
    sdcard_is_mounted = 1;
    file_explorer_media_valid(file_explorer_obj);
    ins_card_call();
}

static void popbtn_call(lv_event_t *event) {
    if (sdcard_is_mounted && f_mount(nullptr, SDPath, 1) != FR_OK) {
        return;
    }
    sdcard_is_mounted = 0;
    if (sdcard_disk_init && HAL_SD_DeInit(&hsd1) != HAL_OK) {
        return;
    }
    sdcard_disk_init = 0;
    if (sdcard_link_driver && FATFS_UnLinkDriver(SDPath) != 0) {
        return;
    }
    sdcard_link_driver = 0;
    file_explorer_media_invalid(file_explorer_obj);
    pop_card_call();
}

static void ins_card_call() {
    lv_obj_add_state(ins_sd, LV_STATE_CHECKED);
    lv_label_set_text_static(ins_sd_label, "SD Card\nOK");
    lv_label_set_text_static(pop_sd_label, "SD Card\nClick To Pop");
    lv_obj_remove_state(pop_sd, LV_STATE_CHECKED);
}

static void pop_card_call() {
    lv_obj_add_state(pop_sd, LV_STATE_CHECKED);
    lv_label_set_text_static(ins_sd_label, "SD Card\nClick To Ins");
    lv_label_set_text_static(pop_sd_label, "SD Card\nRemoved");
    lv_obj_remove_state(ins_sd, LV_STATE_CHECKED);
}

static void change_to_camera(lv_event_t *event) {
    // xSemaphoreGive(sema_take_screenshot);
}

namespace
{
    void            *fe_file_read_buffer      = nullptr;

    void            *jpeg_decode_after_buffer = nullptr; // YCbCr
    void            *jpeg_decode_rgb_buffer   = nullptr; // YCbCr -> RGB
    JPEG_ConfTypeDef jpeg_decode_conf;
    // use sd card as storage of all rgb
    void  *pict_to_show = nullptr; // RGB ----down sample----> RGB --> LVGL
    size_t pict_size[2] = {0, 0};

} // namespace

static void click_one_file(lv_event_t *e, const char *path) {
    DIR     x_dir;
    FRESULT f_res = f_opendir(&x_dir, "0:/.temp");
    if (f_res == FR_OK) {
        f_closedir(&x_dir);
    }
    else {
        f_res = f_mkdir("0:/.temp");
        if (f_res != FR_OK) {
            return;
        }
    }

    FIL temp_file;
    f_res = f_open(&temp_file, "0:/.temp/jpeg_pict_temp", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);

    if (f_res != FR_OK)
        return;
}

void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo) {}
void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {}
void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {}

void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData) {
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength) {
}

////////////////////////////////////////////////////////////////////////
// Take Screen Shot!!!
///////////////////////////////////////////////////////////////////////

static void take_screenshot_task(void *params) {
    uint32_t notification_value = 0;
    while (1) {
        // xSemaphoreTake(sema_take_screenshot, portMAX_DELAY);
        xSemaphoreTake(mutex_gram_read, portMAX_DELAY);
        // HAL_DMA2D_Start_IT(&hdma2d, (uint32_t)curr_screen_buffer, (uint32_t)JPEG_ENCODE_SOURCE, 480, 272);
        while (1) {
            xTaskNotifyWait(0, 0x01, &notification_value, portMAX_DELAY);
            if (notification_value & 0x01u) {
                xTaskNotify(xTaskGetCurrentTaskHandle(), 0, eNoAction);
                break;
            }
        }
        xSemaphoreGive(mutex_gram_read);

        vTaskPrioritySet(xTaskGetCurrentTaskHandle(), uxTaskPriorityGet(Initial_TaskHandle));
        JPEG_RGBToYCbCr_Convert_Function conv_func;
        uint32_t                         nbMcu;
        JPEG_GetEncodeColorConvertFunc(&jpeg_conf, &conv_func, &nbMcu);

        uint32_t byte_consume;
        // conv_func((uint8_t *)JPEG_ENCODE_SOURCE, (uint8_t *)JEPG_YCbCr, 0, sizeof(SDRAM_SCREEN_BUFFER), &byte_consume);

        // curr_encode_ptr   = (uint8_t *)JEPG_YCbCr;
        // target_encode_ptr = (uint8_t *)JEPG_YCbCr + byte_consume;
        // curr_dest_ptr     = (uint8_t *)JPEG_ENCODE_DEST;
        HAL_JPEG_Encode_DMA(&hjpeg, curr_encode_ptr, 65536, curr_dest_ptr, 65536);
        while (1) {
            xTaskNotifyWait(0, 0x02, &notification_value, portMAX_DELAY);
            if (notification_value & 0x02u) {
                xTaskNotify(xTaskGetCurrentTaskHandle(), 0, eNoAction);
                break;
            }
        }
        if (!sd_enable) {
            continue;
        }
        if (f_open(&jpeg1, "0:/a.jpeg", (uint32_t)FA_WRITE | (uint32_t)FA_CREATE_ALWAYS) != FR_OK) {
            continue;
        }
        // UINT brw;
        // if (f_write(&jpeg1, (const uint8_t *)JPEG_ENCODE_DEST, (UINT)(curr_dest_ptr - (uint8_t *)JPEG_ENCODE_DEST), &brw) != FR_OK) {
        // continue;
        // }
        if (f_close(&jpeg1) != FR_OK) {
            continue;
        }
    }
}


// static void hdma2dCompleteCallback(DMA2D_HandleTypeDef *hdma2d) {
//     BaseType_t woken;
//     xTaskNotifyFromISR(take_screenshot, 0x01, eSetBits, &woken);
//     portYIELD_FROM_ISR(woken);
// }

// void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {
//     BaseType_t woken;
//     xTaskNotifyFromISR(take_screenshot, 0x02, eSetBits, &woken);
//     portYIELD_FROM_ISR(woken);
// }
