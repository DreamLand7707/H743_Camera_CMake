#include "prj_header.hpp"
#include "jpeg_utils.h"

#include "SEGGER_RTT.h"
#include <ranges>

// extern Variable Definition
void             *SDRAM_GRAM1;
void             *SDRAM_GRAM2;
void             *JPEG_ENCODE_DEST;
void             *JEPG_YCbCr;
void             *JPEG_ENCODE_SOURCE;

uint8_t           mkfs_buffer[4096] __attribute__((section(".fatfs_buffer")));
uint32_t          sdcard_link_driver = 0;
uint32_t          sdcard_disk_init   = 0;
uint32_t          sdcard_is_mounted  = 0;
uint32_t          sdcard_initialized = 0;

SemaphoreHandle_t sema_flash_screen_routine_start;
SemaphoreHandle_t sema_camera_routine_start;
SemaphoreHandle_t sema_timer_handle;

SemaphoreHandle_t sema_33ms_flash_screen;
TimerHandle_t     timer_33ms_flash_screen;


// static variable Definition
namespace
{
    lv_obj_t         *myBtn;
    lv_obj_t         *label_btn;
    lv_obj_t         *myLabel;
    lv_obj_t         *shot_btn;
    lv_obj_t         *shot_btn_label;
    lv_obj_t         *pop_sd;
    lv_obj_t         *ins_sd;
    lv_obj_t         *pop_sd_label;
    lv_obj_t         *ins_sd_label;

    TaskHandle_t      render_sync_daemon;
    TaskHandle_t      take_screenshot;
    JPEG_ConfTypeDef  jpeg_conf;
    SemaphoreHandle_t mutex_gram_read;
    SemaphoreHandle_t sema_take_screenshot;
    SemaphoreHandle_t sema_update_local;
    char              buff_str[40];
    int               count = 0, i = 0;
    uint8_t          *curr_encode_ptr;
    uint8_t          *target_encode_ptr;
    uint8_t          *curr_dest_ptr;
    FIL               jpeg1;

    SemaphoreHandle_t mutex_internal_lvgl;

    void             *curr_screen_buffer;

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
static void    MYSCB_CleanInvalidateDCache_by_AddrRange(const uint32_t *pData_begin, const uint32_t *pData_end);
static void    timer_500ms_internal_callback(TimerHandle_t xTimer);
static void    render_sync_daemon_task(void *params);
static int     routine(int argc, char **argv);
static void    initial_before_routine();
extern "C" int _getentropy(void *buffer, size_t length);

static void    take_screenshot_task(void *params);
static void    hdma2dCompleteCallback(DMA2D_HandleTypeDef *hdma2d);
static void    press_call(lv_event_t *event);
static void    release_call(lv_event_t *event);
static void    shot_call(lv_event_t *event);
static void    insbtn_call(lv_event_t *event);
static void    popbtn_call(lv_event_t *event);


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

// task2
static void render_sync_daemon_task(void *params) {
    while (1) {
        xSemaphoreTake(sema_timer_handle, portMAX_DELAY);
        xSemaphoreTake(mutex_gram_read, portMAX_DELAY);
        lv_display_flush_ready(rgb_screen_disp);
        //		ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        xSemaphoreGive(mutex_gram_read);
    }
}

/*
        Internal Routine
*/

static void initial_before_routine() {
    mutex_gram_read      = xSemaphoreCreateMutex();
    sema_take_screenshot = xSemaphoreCreateBinary();
    sema_update_local    = xSemaphoreCreateBinary();
    mutex_internal_lvgl  = xSemaphoreCreateMutex();

    SDRAM_GRAM1          = sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));
    SDRAM_GRAM2          = sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));
    JPEG_ENCODE_SOURCE   = sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));
    JPEG_ENCODE_DEST     = sdram_Malloc(256 * 1024);
    JEPG_YCbCr           = sdram_Malloc(256 * 1024);

    HAL_LTDC_SetAddress(&hltdc, (uint32_t)SDRAM_GRAM1, LTDC_LAYER_1);

    hdma2d.XferCpltCallback     = hdma2dCompleteCallback;
    jpeg_conf.ChromaSubsampling = JPEG_444_SUBSAMPLING;
    jpeg_conf.ColorSpace        = JPEG_YCBCR_COLORSPACE;
    jpeg_conf.ImageQuality      = 100;
    jpeg_conf.ImageHeight       = 272;
    jpeg_conf.ImageWidth        = 480;
    HAL_JPEG_ConfigEncoding(&hjpeg, &jpeg_conf);
}

static int routine(int argc, char **argv) {
    lvgl_initialize_port1();
    sdcard_initialize();
    lvgl_initialize_port2();

    xSemaphoreGive(sema_flash_screen_routine_start);
    xSemaphoreGive(sema_camera_routine_start);

    osThreadSetPriority(Initial_TaskHandle, osPriorityNormal);

    vQueueAddToRegistry((QueueHandle_t)sema_update_local, "sema_update_local");
    vQueueAddToRegistry((QueueHandle_t)mutex_gram_read, "mutex_gram_read");
    vQueueAddToRegistry((QueueHandle_t)sema_take_screenshot, "sema_take_screenshot");

    xTaskCreate(render_sync_daemon_task, "thread_render_sync_daemon", 512, nullptr, 5, &render_sync_daemon);
    xTaskCreate(take_screenshot_task, "thread_take_screen_shot", 512, nullptr, 5, &take_screenshot);

    TimerHandle_t timer_500ms_internal;
    timer_500ms_internal = xTimerCreate("timer_500ms_internal", pdMS_TO_TICKS(500), pdTRUE, nullptr, &timer_500ms_internal_callback);
    xTimerStart(timer_500ms_internal, portMAX_DELAY);

    lv_obj_add_event_cb(myBtn, press_call, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(myBtn, release_call, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(shot_btn, shot_call, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(ins_sd, insbtn_call, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(pop_sd, popbtn_call, LV_EVENT_PRESSED, nullptr);

    while (1) {
        xSemaphoreTake(mutex_internal_lvgl, portMAX_DELAY);
        if (pdTRUE == xSemaphoreTake(sema_update_local, 0)) {
            if (count % 2)
                lv_label_set_text(myLabel, buff_str);
            else
                lv_label_set_text(label_btn, buff_str);
        }
        lv_timer_handler();
        xSemaphoreGive(mutex_internal_lvgl);
        vTaskDelay(1);
    }
}

static void lvgl_initialize_port1() {
    lcd_touch_initialize();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    myBtn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(myBtn, 120, 50);
    lv_obj_align(myBtn, LV_ALIGN_CENTER, 170, 0);
    lv_obj_set_style_bg_color(myBtn, lv_color_make(255, 0, 0), 0);

    label_btn = lv_label_create(myBtn);
    lv_obj_align(label_btn, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label_btn, "--button--\nclick me!");

    myLabel = lv_label_create(lv_scr_act());
    lv_label_set_text(myLabel, "Hello Kitty!");
    lv_obj_align(myLabel, LV_ALIGN_CENTER, 100, -50);

    shot_btn = lv_btn_create(lv_scr_act());
    lv_obj_align(shot_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_size(shot_btn, 40, 20);
    lv_obj_set_style_bg_color(shot_btn, lv_color_make(0x00, 0x77, 0x77), 0);

    shot_btn_label = lv_label_create(shot_btn);
    lv_obj_align(shot_btn_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(shot_btn_label, &Camera_1_bit, 0);
    lv_label_set_text(shot_btn_label, MY_CAMERA_SYMBOL);
    lv_obj_set_style_text_color(shot_btn_label, lv_color_make(0xff, 0xff, 0xff), 0);

    pop_sd = lv_button_create(lv_scr_act());
    lv_obj_align(pop_sd, LV_ALIGN_RIGHT_MID, -20, 60);
    lv_obj_set_size(pop_sd, 60, 40);
    lv_obj_set_style_bg_color(pop_sd, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);

    ins_sd = lv_button_create(lv_scr_act());
    lv_obj_align(ins_sd, LV_ALIGN_RIGHT_MID, -20, -60);
    lv_obj_set_size(ins_sd, 60, 40);
    lv_obj_set_style_bg_color(ins_sd, lv_palette_main(LV_PALETTE_LIGHT_BLUE), 0);

    pop_sd_label = lv_label_create(pop_sd);
    lv_label_set_text(pop_sd_label, "pop");
    lv_obj_align(pop_sd_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(pop_sd_label, lv_color_make(0x00, 0x00, 0x00), 0);

    ins_sd_label = lv_label_create(ins_sd);
    lv_label_set_text(ins_sd_label, "ins");
    lv_obj_align(ins_sd_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(ins_sd_label, lv_color_make(0x00, 0x00, 0x00), 0);
}

static void sdcard_initialize() {
    printf("STATUS: %d\n", SD_Driver.disk_initialize(0));
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
    f_mount(nullptr, SDPath, 1);

    sdcard_is_mounted = 0;
}

static void lvgl_initialize_port2() {
    lv_port_fs_init();
}

// Tool function
static void MYSCB_CleanInvalidateDCache_by_AddrRange(const uint32_t *pData_begin, const uint32_t *pData_end) {
    uint32_t address_start = (uint32_t)pData_begin;
    uint32_t address_end   = (uint32_t)pData_end + 31;
    address_start &= 0xffffffe0;
    address_end &= 0xffffffe0;
    int32_t real_size = int32_t(address_end - address_start);
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)address_start, real_size);
}

// Tool function
int rgba_equal(BGR *a, BGR *b) {
    return (a->B == b->B) && (a->G == b->G) && (a->R == b->R);
}

// ISR Callback
void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc) {
    BaseType_t woke = pdFALSE;
    xSemaphoreGiveFromISR(sema_timer_handle, &woke);
    portYIELD_FROM_ISR(woke);
}

// timer callback
static void timer_500ms_internal_callback(TimerHandle_t xTimer) {
    sprintf(buff_str, "%d", i);
    i++;
    count++;
    i = i % 10;
    xSemaphoreGive(sema_update_local);
}

// timer callback
void timer_33ms_flash_screen_callback(TimerHandle_t xTimer) {
    xSemaphoreGive(sema_33ms_flash_screen);
}

// BSP FATFS Callback
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks) {
    uint8_t  sd_state      = MSD_OK;
    uint32_t address_start = (uint32_t)pData;
    uint32_t address_end   = address_start + NumOfBlocks * 512;
    MYSCB_CleanInvalidateDCache_by_AddrRange((uint32_t *)address_start, (uint32_t *)address_end);
    if (HAL_SD_ReadBlocks_DMA(&hsd1, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK) {
        sd_state = MSD_ERROR;
    }

    return sd_state;
}

// BSP FATFS Callback
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks) {
    uint8_t  sd_state      = MSD_OK;
    uint32_t address_start = (uint32_t)pData;
    uint32_t address_end   = address_start + NumOfBlocks * 512;
    MYSCB_CleanInvalidateDCache_by_AddrRange((uint32_t *)address_start, (uint32_t *)address_end);
    if (HAL_SD_WriteBlocks_DMA(&hsd1, (uint8_t *)pData, WriteAddr, NumOfBlocks) != HAL_OK) {
        sd_state = MSD_ERROR;
    }

    return sd_state;
}

// lvgl callback
void swapBuffer(void *passbuf) {
    HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t)passbuf, LTDC_LAYER_1);
    HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);
    curr_screen_buffer = passbuf;
    printf("swapBuffer\n");
}

// std::cout callback
int _getentropy(void *buffer, size_t length) {
    return 0;
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


////////////////////////////////////////////////////////////////////////
// Take Screen Shot!!!
///////////////////////////////////////////////////////////////////////

static void take_screenshot_task(void *params) {
    uint32_t notification_value = 0;
    while (1) {
        xSemaphoreTake(sema_take_screenshot, portMAX_DELAY);
        xSemaphoreTake(mutex_gram_read, portMAX_DELAY);
        HAL_DMA2D_Start_IT(&hdma2d, (uint32_t)curr_screen_buffer, (uint32_t)JPEG_ENCODE_SOURCE, 480, 272);
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
        conv_func((uint8_t *)JPEG_ENCODE_SOURCE, (uint8_t *)JEPG_YCbCr, 0, sizeof(SDRAM_SCREEN_BUFFER), &byte_consume);

        curr_encode_ptr   = (uint8_t *)JEPG_YCbCr;
        target_encode_ptr = (uint8_t *)JEPG_YCbCr + byte_consume;
        curr_dest_ptr     = (uint8_t *)JPEG_ENCODE_DEST;
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
        UINT brw;
        if (f_write(&jpeg1, (const uint8_t *)JPEG_ENCODE_DEST, (UINT)(curr_dest_ptr - (uint8_t *)JPEG_ENCODE_DEST), &brw) != FR_OK) {
            continue;
        }
        if (f_close(&jpeg1) != FR_OK) {
            continue;
        }
    }
}

void press_call(lv_event_t *event) {
    lv_color_t color = lv_color_make(0xff, 0x00, 0x00);
    lv_obj_set_style_text_color(myLabel, color, 0);
}
void release_call(lv_event_t *event) {
    lv_color_t color = lv_color_make(0x00, 0x00, 0x00);
    lv_obj_set_style_text_color(myLabel, color, 0);
}
void shot_call(lv_event_t *event) {
    xSemaphoreGive(sema_take_screenshot);
}
void insbtn_call(lv_event_t *event) {

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
}
void popbtn_call(lv_event_t *event) {

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
}

static void hdma2dCompleteCallback(DMA2D_HandleTypeDef *hdma2d) {
    BaseType_t woken;
    xTaskNotifyFromISR(take_screenshot, 0x01, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}

void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {
    BaseType_t woken;
    xTaskNotifyFromISR(take_screenshot, 0x02, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}

void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData) {
    uint32_t length_this_time;
    curr_encode_ptr += NbDecodedData;
    if (curr_encode_ptr < target_encode_ptr) {
        length_this_time = uint32_t(target_encode_ptr - curr_encode_ptr);
        if (length_this_time > 65536)
            length_this_time = 65536;
    }
    else {
        length_this_time = 0;
    }
    HAL_JPEG_ConfigInputBuffer(hjpeg, curr_encode_ptr, length_this_time);
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength) {
    curr_dest_ptr += OutDataLength;
    HAL_JPEG_ConfigOutputBuffer(hjpeg, curr_dest_ptr, 65536);
}
