#include "prj_header.hpp"
#include "jpeg_utils.h"
#include "file_explorer.hpp"

// extern Variable Definition
void             *SDRAM_GRAM1;
void             *SDRAM_GRAM2;

uint8_t           mkfs_buffer[4096] __attribute__((section(".fatfs_buffer")));
uint32_t          sdcard_link_driver = 0;
uint32_t          sdcard_is_mounted  = 0;
uint32_t          sdcard_initialized = 0;

SemaphoreHandle_t sema_flash_screen_routine_start;
SemaphoreHandle_t sema_camera_routine_start;
SemaphoreHandle_t sema_swap_buffer_handle;

// traceString       trace_analyzer_channel1;
// traceString       trace_analyzer_channel2;
// traceString       trace_analyzer_channel3;
// traceString       trace_analyzer_channel4;

uint8_t segger_data_upload[64 * 1024] __attribute((section(".dtcmicm")));

// static variable Definition
namespace
{
    void *curr_screen_buffer;

} // namespace

// extern Variable Decl
LV_FONT_DECLARE(Camera_1_bit)

// MARCO
#define MY_CAMERA_SYMBOL "\xEF\x80\xB0"
#define sd_enable        ((sdcard_is_mounted) & (sdcard_link_driver))

// static function decl
static void    lvgl_initialize_port1();
static void    lvgl_initialize_port2();
static void    sdcard_initialize();
static int     routine(int argc, char **argv);
static void    initial_before_routine();
extern "C" int _getentropy(void *buffer, size_t length);

static void    insbtn_call(lv_event_t *event);
static void    popbtn_call(lv_event_t *event);
static void    ins_card_call();
static void    pop_card_call();
static void    lvgl_create_main_interface();
static void    change_to_camera(lv_event_t *event);
static void    click_one_file(lv_event_t *e, const char *path);
static void    click_file_init();
static void    jpeg_rgb_exchange_init();


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

    vTaskPrioritySet(Initial_TaskHandle, 3);
    // trace_analyzer_channel1 = xTraceRegisterString("User_Channel_1");
    // trace_analyzer_channel2 = xTraceRegisterString("User_Channel_2");
    // trace_analyzer_channel3 = xTraceRegisterString("User_Channel_3");
    // trace_analyzer_channel4 = xTraceRegisterString("User_Channel_4");

    SEGGER_RTT_ConfigUpBuffer(1, "DataLog", segger_data_upload, sizeof(segger_data_upload),
                              SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    jpeg_rgb_exchange_init();
    lvgl_create_main_interface();
    click_file_init();

    while (1) {
        lv_timer_handler();
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

    lv_obj_add_event_cb(shot_btn, change_to_camera, LV_EVENT_PRESSED, nullptr);
    file_explorer_set_callback(file_explorer_obj, click_one_file);
    lv_obj_add_event_cb(ins_sd, insbtn_call, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(pop_sd, popbtn_call, LV_EVENT_PRESSED, nullptr);
    if (sdcard_is_mounted)
        ins_card_call();
    else
        pop_card_call();
}

static void lvgl_initialize_port1() {
    touch_sence_init();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
}

static void lvgl_initialize_port2() {
    if (sdcard_is_mounted)
        lv_port_fs_init();
}


static void sdcard_initialize() {
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
    print_sdcard_info();
    sdcard_is_mounted = 1;

    f_open(&SDFile, "0:/a.txt", (uint32_t)FA_WRITE | (uint32_t)FA_CREATE_ALWAYS);
    const char *str    = "Hello world!";
    UINT        expect = strlen(str);
    UINT        get    = 0;
    f_write(&SDFile, str, expect, &get);
    f_close(&SDFile);
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
    (void)HAL_SD_DeInit(&hsd1);
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
}

uint8_t *file_exchange_buffer         = nullptr;
uint8_t *jpeg_after_buffer            = nullptr; // YCbCr
uint8_t *jpeg_before_buffer_rgb       = nullptr; // YCbCr -> RGB
uint8_t *full_screen_pict_show_buffer = nullptr;
uint8_t *widget_pict_show_buffer      = nullptr;

namespace
{

    JPEG_ConfTypeDef jpeg_decode_conf = {};
    // use sd card as storage of all rgb
    FIL               jpeg_decode_target_file;
    bool              jpeg_decode_target_file_should_close = false;
    bool              jpeg_decode_should_abort             = false;

    SemaphoreHandle_t jpeg_decode_task_interrupt_mutex     = nullptr;
    QueueHandle_t     jpeg_decode_ctrl_task_queue          = nullptr;
    TaskHandle_t      jpeg_decode_task_handle              = nullptr;
    TaskHandle_t      jpeg_decode_ctrl_task_handle         = nullptr;
    TaskHandle_t      picture_show_task_handle             = nullptr;
    QueueHandle_t     jpeg_decode_input_data_req           = nullptr;
    QueueHandle_t     jpeg_decode_output_buffer_req        = nullptr;
    QueueSetHandle_t  jpeg_decode_input_output_queueset    = nullptr;
    SemaphoreHandle_t jpeg_decode_complete                 = nullptr;
    QueueHandle_t     picture_show_loading                 = nullptr;
    TimeOut_t         lastest_decode_time                  = {};

    struct jpeg_output_buffer_req {
        uint8_t *data_out;
        uint32_t length;
    };

    struct jpeg_decode_command {
        std::string *path;
        TimeOut_t    decode_time;
    };

    struct pict_show_command {
        int       type;
        TimeOut_t show_time;
    };

} // namespace

static void jpeg_decode_task(void *args);
static void jpeg_decode_ctrl_task(void *args);
static void picture_show_task(void *arg);

static void click_file_init() {
    jpeg_decode_task_interrupt_mutex  = xSemaphoreCreateMutex();
    jpeg_decode_ctrl_task_queue       = xQueueCreate(1, sizeof(jpeg_decode_command));
    jpeg_decode_input_data_req        = xQueueCreate(1, sizeof(uint32_t));
    jpeg_decode_output_buffer_req     = xQueueCreate(1, sizeof(jpeg_output_buffer_req));
    picture_show_loading              = xQueueCreate(1, sizeof(pict_show_command));
    jpeg_decode_complete              = xSemaphoreCreateBinary();
    jpeg_decode_input_output_queueset = xQueueCreateSet(3);
    xQueueAddToSet(jpeg_decode_input_data_req, jpeg_decode_input_output_queueset);
    xQueueAddToSet(jpeg_decode_output_buffer_req, jpeg_decode_input_output_queueset);
    xQueueAddToSet(jpeg_decode_complete, jpeg_decode_input_output_queueset);

    vQueueAddToRegistry((QueueHandle_t)jpeg_decode_task_interrupt_mutex, "jpeg_decode_task_interrupt_mutex");
    vQueueAddToRegistry((QueueHandle_t)jpeg_decode_ctrl_task_queue, "jpeg_decode_ctrl_task_queue");
    vQueueAddToRegistry((QueueHandle_t)jpeg_decode_input_data_req, "jpeg_decode_input_data_req");
    vQueueAddToRegistry((QueueHandle_t)jpeg_decode_output_buffer_req, "jpeg_decode_output_buffer_req");
    vQueueAddToRegistry((QueueHandle_t)picture_show_loading, "picture_show_loading");
    vQueueAddToRegistry((QueueHandle_t)jpeg_decode_complete, "jpeg_decode_complete");
    vQueueAddToRegistry((QueueHandle_t)jpeg_decode_input_output_queueset, "jpeg_decode_input_output_queueset");

    xTaskCreate(jpeg_decode_ctrl_task, "JPEG Decode Ctrl Task", 2048, nullptr, 4, &jpeg_decode_ctrl_task_handle);
    xTaskCreate(picture_show_task, "Picture Show Task", 1024, nullptr, 4, &picture_show_task_handle);
}

static void jpeg_rgb_exchange_init() {
    file_exchange_buffer         = (uint8_t *)pvPortMalloc(64 * 1024);        // 64KB
    jpeg_after_buffer            = (uint8_t *)pvPortMalloc(64 * 1024);        // 64KB
    jpeg_before_buffer_rgb       = (uint8_t *)sdram_Malloc(16 * 1024 * 1024); // 16MB
    full_screen_pict_show_buffer = (uint8_t *)sdram_Malloc(3 * 272 * 480);    // 382.5KB
    widget_pict_show_buffer      = (uint8_t *)sdram_Malloc(3 * 272 * 480);    // 382.5KB

    // jpeg_decode_target_file      = (FIL *)sdram_Malloc(sizeof(FIL));

    HeapStats_t stats;
    char        buffer[128] = {};

    sdram_GetHeapStats(&stats);
    sprintf(buffer, "SDRAM All      Avail: %f KBytes\n", float(stats.xAvailableHeapSpaceInBytes) / 1024.0f);
    jprintf(0, "%s", buffer);
    sprintf(buffer, "SDRAM Largest  Avail: %f KBytes\n", float(stats.xSizeOfLargestFreeBlockInBytes) / 1024.0f);
    jprintf(0, "%s", buffer);
    sprintf(buffer, "SDRAM Smallest Avail: %f KBytes\n", float(stats.xSizeOfSmallestFreeBlockInBytes) / 1024.0f);
    jprintf(0, "%s", buffer);
    sprintf(buffer, "SDRAM Blocks   Avail: %u Blocks\n", stats.xNumberOfFreeBlocks);
    jprintf(0, "%s", buffer);

    vPortGetHeapStats(&stats);
    sprintf(buffer, "Internal SRAM All      Avail: %f KBytes\n", float(stats.xAvailableHeapSpaceInBytes) / 1024.0f);
    jprintf(0, "%s", buffer);
    sprintf(buffer, "Internal SRAM Largest  Avail: %f KBytes\n", float(stats.xSizeOfLargestFreeBlockInBytes) / 1024.0f);
    jprintf(0, "%s", buffer);
    sprintf(buffer, "Internal SRAM Smallest Avail: %f KBytes\n", float(stats.xSizeOfSmallestFreeBlockInBytes) / 1024.0f);
    jprintf(0, "%s", buffer);
    sprintf(buffer, "Internal SRAM Blocks   Avail: %u Blocks\n", stats.xNumberOfFreeBlocks);
    jprintf(0, "%s", buffer);
}

static void jpeg_decode_task(void *args) {

    xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);
    auto *jpeg_file_path = (jpeg_decode_command *)(args);
    do {
        FRESULT f_res = FR_OK;

        //
        jpeg_decode_target_file_should_close = false;
        f_res                                = f_open(&jpeg_decode_target_file, jpeg_file_path->path->c_str(), FA_READ);
        if (f_res != FR_OK) {
            break;
        }
        jpeg_decode_target_file_should_close = true;

        UINT          real_read_num;
        FSIZE_t       jpeg_decode_target_file_size = f_size(&jpeg_decode_target_file);
        QueueHandle_t the_queue                    = nullptr;
        //
        f_res = f_read(&jpeg_decode_target_file, file_exchange_buffer, 65536, &real_read_num);
        if (f_res != FR_OK) {
            break;
        }
        jpeg_decode_target_file_size -= real_read_num;

        real_read_num = ((real_read_num / 32) + !!(real_read_num % 32)) * 32;
        MYSCB_CleanInvalidateDCache_by_AddrRange(file_exchange_buffer, (void *)((uintptr_t)file_exchange_buffer + 65536));
        HAL_JPEG_Decode_DMA(&hjpeg, (uint8_t *)file_exchange_buffer, real_read_num,
                            (uint8_t *)jpeg_after_buffer, 65536);
        jpeg_decode_should_abort                           = true;

        uint32_t                         decoded_data      = 0;
        jpeg_output_buffer_req           data_ready_decode = {};
        JPEG_YCbCrToRGB_Convert_Function decode_function   = nullptr;
        uint32_t                         nbMcu             = 0;
        uint32_t                         data_consume      = 0;
        uint32_t                         curr_mcu          = 0;
        uint32_t                         data_valid        = 0;

        while (true) {
            xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
            {
                the_queue = xQueueSelectFromSet(jpeg_decode_input_output_queueset, portMAX_DELAY);
            }
            xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);
            if (the_queue == jpeg_decode_input_data_req) {
                xQueueReceive(the_queue, &decoded_data, 0);
                if (jpeg_decode_target_file_size > 65536) {
                    f_res = f_read(&jpeg_decode_target_file, file_exchange_buffer, 65536, &real_read_num);
                }
                else {
                    f_res = f_read(&jpeg_decode_target_file, file_exchange_buffer, jpeg_decode_target_file_size, &real_read_num);
                }
                if (f_res != FR_OK)
                    goto error;
                jpeg_decode_target_file_size -= real_read_num;
                real_read_num = ((real_read_num / 32) + !!(real_read_num % 32)) * 32;
                taskENTER_CRITICAL();
                {
                    HAL_JPEG_ConfigInputBuffer(&hjpeg, file_exchange_buffer, real_read_num);
                    MYSCB_CleanDCache_by_AddrRange(file_exchange_buffer, (void *)((uintptr_t)file_exchange_buffer + 65536));
                    HAL_JPEG_Resume(&hjpeg, JPEG_PAUSE_RESUME_INPUT);
                }
                taskEXIT_CRITICAL();
            }
            else if (the_queue == jpeg_decode_output_buffer_req) {
                xQueueReceive(the_queue, &data_ready_decode, 0);
                if (!decode_function)
                    JPEG_GetDecodeColorConvertFunc(&jpeg_decode_conf, &decode_function, &nbMcu);
                xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
                {
                    data_valid += data_ready_decode.length;
                    MYSCB_InvalidateDCache_by_AddrRange(jpeg_after_buffer, (void *)((uintptr_t)jpeg_after_buffer + data_valid));
                    uint32_t use_mcu = decode_function(jpeg_after_buffer, jpeg_before_buffer_rgb, curr_mcu, data_valid, &data_consume);
                    curr_mcu += use_mcu;
                    uint32_t data_used = 0;
                    if (jpeg_decode_conf.ChromaSubsampling == JPEG_420_SUBSAMPLING) {
                        data_used = use_mcu * 384;
                    }
                    else if (jpeg_decode_conf.ChromaSubsampling == JPEG_422_SUBSAMPLING) {
                        data_used = use_mcu * 256;
                    }
                    else if (jpeg_decode_conf.ChromaSubsampling == JPEG_444_SUBSAMPLING) {
                        data_used = use_mcu * 192;
                    }
                    data_valid -= data_used;
                    memcpy(jpeg_after_buffer, jpeg_after_buffer + data_used, data_valid);
                    MYSCB_CleanDCache_by_AddrRange(jpeg_after_buffer, (void *)((uintptr_t)jpeg_after_buffer + data_valid));
                }
                xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);
                taskENTER_CRITICAL();
                {
                    HAL_JPEG_ConfigOutputBuffer(&hjpeg, jpeg_after_buffer + data_valid, 65536 - data_valid);
                    HAL_JPEG_Resume(&hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
                }
                taskEXIT_CRITICAL();
            }
            else if (the_queue == jpeg_decode_complete) {
                jpeg_decode_should_abort = false;

                vTaskSuspendAll();
                {
                    if (jpeg_file_path->decode_time.xOverflowCount > lastest_decode_time.xOverflowCount ||
                        jpeg_file_path->decode_time.xTimeOnEntering >= lastest_decode_time.xTimeOnEntering) {
                        pict_show_command cm {0, jpeg_file_path->decode_time};
                        xQueueOverwrite(picture_show_loading, &cm);
                    }
                }
                xTaskResumeAll();

                break;
            }
        }

    error:
        break;
    }
    while (false);

    { // JPEG Abort
        if (jpeg_decode_should_abort) {
            HAL_JPEG_Abort(&hjpeg);
            jpeg_decode_should_abort = false;
        }
    }
    { // Close File
        if (jpeg_decode_target_file_should_close) {
            f_close(&jpeg_decode_target_file);
            jpeg_decode_target_file_should_close = false;
        }
    }

    xQueueReset(jpeg_decode_input_data_req);
    xQueueReset(jpeg_decode_output_buffer_req);
    xQueueReset(jpeg_decode_complete);
    xQueueReset(jpeg_decode_input_output_queueset);
    xSemaphoreGive(jpeg_decode_task_interrupt_mutex);

    while (true) {
        vTaskSuspend(xTaskGetCurrentTaskHandle()); // Suspend itself
    }
}

static void jpeg_decode_ctrl_task(void *args) {
    jpeg_decode_command        message {};
    jpeg_decode_command        old_message {};
    static jpeg_decode_command s_message;
    while (true) {
        xQueuePeek(jpeg_decode_ctrl_task_queue, &message, portMAX_DELAY);
        // if (old_message.path && message.path && *old_message.path == *message.path) {
        //     xQueueReceive(jpeg_decode_ctrl_task_queue, &message, portMAX_DELAY);
        //     continue;
        // }

        xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);
        xQueueReceive(jpeg_decode_ctrl_task_queue, &message, portMAX_DELAY);

        // if (old_message.path && message.path && *old_message.path == *message.path) {
        //     xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
        //     continue;
        // }

        if (jpeg_decode_task_handle) {
            vTaskDelete(jpeg_decode_task_handle);
            jpeg_decode_task_handle = nullptr;
            delete old_message.path;

            if (jpeg_decode_target_file_should_close) {
                f_close(&jpeg_decode_target_file);
                jpeg_decode_target_file_should_close = false;
            }
            if (jpeg_decode_should_abort) {
                HAL_JPEG_Abort(&hjpeg);
                jpeg_decode_should_abort = false;
            }
            xQueueReset(jpeg_decode_input_data_req);
            xQueueReset(jpeg_decode_output_buffer_req);
            xQueueReset(jpeg_decode_complete);
            xQueueReset(jpeg_decode_input_output_queueset);
            old_message.path = nullptr;
        }
        if (message.path) {
            old_message                          = message;
            s_message                            = old_message;
            jpeg_decode_target_file_should_close = false;
            lastest_decode_time                  = message.decode_time;
            pict_show_command cm                 = {1, message.decode_time};
            xQueueOverwrite(picture_show_loading, &cm);
            xTaskCreate(jpeg_decode_task, "JPEG Decode Task", 2048, &s_message, 3, &jpeg_decode_task_handle);
        }
        xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
    }
}

static void picture_show_task(void *arg) {
    static lv_image_dsc_t img_dsc;
    img_dsc.header.magic      = LV_IMAGE_HEADER_MAGIC;
    img_dsc.header.cf         = LV_COLOR_FORMAT_RGB888;
    img_dsc.header.flags      = 0;
    img_dsc.header.reserved_2 = 0;
    pict_show_command cm {};
    while (true) {
        xQueuePeek(picture_show_loading, &cm, portMAX_DELAY);

        lv_lock();
        xQueueReceive(picture_show_loading, &cm, 0);
        if (cm.type == 1) {
            lv_image_set_src(image, LV_SYMBOL_LOOP "\nLoading...");
        }
        else if (cm.type == 0) {
            img_dsc.header.stride = 0;
            img_dsc.header.w      = jpeg_decode_conf.ImageWidth;
            img_dsc.header.h      = jpeg_decode_conf.ImageHeight;
            img_dsc.data          = jpeg_before_buffer_rgb;
            img_dsc.data_size     = img_dsc.header.w * img_dsc.header.h * 3;
            lv_image_set_src(image, &img_dsc);
            uint32_t x_scale = 256ull * lv_obj_get_width(image) / img_dsc.header.w;
            uint32_t y_scale = 256ull * lv_obj_get_height(image) / img_dsc.header.h;
            lv_image_set_scale(image, std::min(x_scale, y_scale));
        }
        lv_unlock();
    }
}

static void click_one_file(lv_event_t *e, const char *path) {
    char                extension[8]  = {};
    auto               *the_file_path = new std::string(path);
    jpeg_decode_command peek_message  = {};
    jpeg_decode_command send_command  = {};
    TimeOut_t           new_time_flag {};
    path_get_file_extension(path, extension);
    if (!strcmp(extension, ".jpeg") || !strcmp(extension, ".jpg") ||
        !strcmp(extension, ".JPEG") || !strcmp(extension, ".JPG")) {
        vTaskSuspendAll();
        {
            if (xQueuePeek(jpeg_decode_ctrl_task_queue, &peek_message, 0) == pdPASS) {
                delete peek_message.path;
            }

            vTaskSetTimeOutState(&new_time_flag);

            send_command.path        = the_file_path;
            send_command.decode_time = new_time_flag;
            xQueueOverwrite(jpeg_decode_ctrl_task_queue, &send_command);
        }
        xTaskResumeAll();
    }
}

void HAL_JPEG_InfoReadyCallback(JPEG_HandleTypeDef *hjpeg, JPEG_ConfTypeDef *pInfo) {
    memcpy(&jpeg_decode_conf, pInfo, sizeof(JPEG_ConfTypeDef));
}

void HAL_JPEG_EncodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {}

void HAL_JPEG_DecodeCpltCallback(JPEG_HandleTypeDef *hjpeg) {
    BaseType_t should_yield = pdFALSE;
    xSemaphoreGiveFromISR(jpeg_decode_complete, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void HAL_JPEG_GetDataCallback(JPEG_HandleTypeDef *hjpeg, uint32_t NbDecodedData) {
    BaseType_t should_yield = pdFALSE;
    HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_INPUT);
    xQueueSendFromISR(jpeg_decode_input_data_req, &NbDecodedData, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}

void HAL_JPEG_DataReadyCallback(JPEG_HandleTypeDef *hjpeg, uint8_t *pDataOut, uint32_t OutDataLength) {
    BaseType_t should_yield = pdFALSE;
    HAL_JPEG_Pause(hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
    jpeg_output_buffer_req temp {pDataOut, OutDataLength};
    xQueueSendFromISR(jpeg_decode_output_buffer_req, &temp, &should_yield);
    portYIELD_FROM_ISR(should_yield);
}
