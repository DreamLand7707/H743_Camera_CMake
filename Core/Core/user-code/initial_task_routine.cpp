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
    // TaskHandle_t      take_screenshot;
    // SemaphoreHandle_t sema_take_screenshot;

    // JPEG_ConfTypeDef jpeg_conf;
    // uint8_t         *curr_encode_ptr;
    // uint8_t         *target_encode_ptr;
    // uint8_t         *curr_dest_ptr;
    // FIL              jpeg1;

    SemaphoreHandle_t mutex_gram_read;
    void             *curr_screen_buffer;

} // namespace

// extern Variable Decl
LV_FONT_DECLARE(Camera_1_bit)

// MARCO
#define MY_CAMERA_SYMBOL "\xEF\x80\xB0"
#define sd_enable        ((sdcard_is_mounted) & (sdcard_link_driver))

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
static void click_file_init();
static void jpeg_rgb_exchange_init();

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

    jpeg_rgb_exchange_init();

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
    click_file_init();

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
    if (sdcard_is_mounted)
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

uint8_t *file_exchange_buffer   = nullptr;
uint8_t *jpeg_after_buffer      = nullptr; // YCbCr
uint8_t *jpeg_before_buffer_rgb = nullptr; // YCbCr -> RGB
namespace
{

    JPEG_ConfTypeDef jpeg_decode_conf = {};
    // use sd card as storage of all rgb

    // FIL               jpeg_decode_temp_file;
    // bool              jpeg_decode_temp_file_should_close = false;
    FIL               jpeg_decode_target_file;
    bool              jpeg_decode_target_file_should_close = false;
    bool              jpeg_decode_should_abort             = false;

    SemaphoreHandle_t jpeg_decode_task_interrupt_mutex     = nullptr;
    QueueHandle_t     jpeg_decode_ctrl_task_queue          = nullptr;
    TaskHandle_t      jpeg_decode_task_handle              = nullptr;
    TaskHandle_t      jpeg_decode_ctrl_task_handle         = nullptr;
    QueueHandle_t     jpeg_decode_input_data_req           = nullptr;
    QueueHandle_t     jpeg_decode_output_buffer_req        = nullptr;
    QueueSetHandle_t  jpeg_decode_input_output_queueset    = nullptr;
    SemaphoreHandle_t jpeg_decode_complete                 = nullptr;

    struct jpeg_output_buffer_req {
        uint8_t *data_out;
        uint32_t length;
    };

} // namespace

static void jpeg_decode_task(void *args);
static void jpeg_decode_ctrl_task(void *args);

static void click_file_init() {
    jpeg_decode_task_interrupt_mutex  = xSemaphoreCreateMutex();
    jpeg_decode_ctrl_task_queue       = xQueueCreate(1, sizeof(std::string *));
    jpeg_decode_input_data_req        = xQueueCreate(1, sizeof(uint32_t));
    jpeg_decode_output_buffer_req     = xQueueCreate(1, sizeof(jpeg_output_buffer_req));
    jpeg_decode_complete              = xSemaphoreCreateBinary();
    jpeg_decode_input_output_queueset = xQueueCreateSet(3);
    xQueueAddToSet(jpeg_decode_input_data_req, jpeg_decode_input_output_queueset);
    xQueueAddToSet(jpeg_decode_output_buffer_req, jpeg_decode_input_output_queueset);
    xQueueAddToSet(jpeg_decode_complete, jpeg_decode_input_output_queueset);
    xTaskCreate(jpeg_decode_ctrl_task, "JPEG Decode Ctrl Task", 2048, nullptr, 3, &jpeg_decode_ctrl_task_handle);
}

static void jpeg_rgb_exchange_init() {
    file_exchange_buffer   = (uint8_t *)pvPortMalloc(64 * 1024);
    jpeg_after_buffer      = (uint8_t *)pvPortMalloc(64 * 1024);        
    jpeg_before_buffer_rgb = (uint8_t *)sdram_Malloc(15 * 1024 * 1024); // 15MB
}

static void jpeg_decode_task(void *args) {

    xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);
    auto *jpeg_file_path = (std::string *)(args);
    do {
        FRESULT f_res = FR_OK;

        // DIR     x_dir;
        // f_res = f_opendir(&x_dir, "0:/.temp");
        // if (f_res == FR_OK) {
        //     f_closedir(&x_dir);
        // }
        // else {
        //     f_res = f_mkdir("0:/.temp");
        //     if (f_res != FR_OK) {
        //         break;
        //     }
        // }
        // xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
        // xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);

        // jpeg_decode_temp_file_should_close = false;
        // //
        // f_res = f_open(&jpeg_decode_temp_file, "0:/.temp/jpeg_pict_temp", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
        // if (f_res != FR_OK)
        //     break;
        // jpeg_decode_temp_file_should_close   = true;

        vTaskSuspendAll();
        {
            jpeg_decode_target_file_should_close = false;
        }
        xTaskResumeAll();
        //
        f_res = f_open(&jpeg_decode_target_file, jpeg_file_path->c_str(), FA_READ);
        if (f_res != FR_OK)
            break;
        vTaskSuspendAll();
        {
            jpeg_decode_target_file_should_close = true;
        }
        xTaskResumeAll();

        UINT          real_read_num;
        FSIZE_t       jpeg_decode_target_file_size = f_size(&jpeg_decode_target_file);
        QueueHandle_t the_queue                    = nullptr;
        //
        f_res = f_read(&jpeg_decode_target_file, file_exchange_buffer, 65536, &real_read_num);
        if (f_res != FR_OK)
            break;
        jpeg_decode_target_file_size -= real_read_num;

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
                HAL_JPEG_ConfigInputBuffer(&hjpeg, file_exchange_buffer, real_read_num);
                HAL_JPEG_Resume(&hjpeg, JPEG_PAUSE_RESUME_INPUT);
            }
            else if (the_queue == jpeg_decode_output_buffer_req) {
                xQueueReceive(the_queue, &data_ready_decode, 0);
                if (!decode_function)
                    JPEG_GetDecodeColorConvertFunc(&jpeg_decode_conf, &decode_function, &nbMcu);
                xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
                {
                    data_valid += data_ready_decode.length;
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
                    memcpy(jpeg_after_buffer, jpeg_after_buffer + data_used, data_valid - data_used);
                    HAL_JPEG_ConfigOutputBuffer(&hjpeg, jpeg_after_buffer + data_valid - data_used, 65536 - (data_valid - data_used));
                }
                xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);
                HAL_JPEG_Resume(&hjpeg, JPEG_PAUSE_RESUME_OUTPUT);
            }
            else if (the_queue == jpeg_decode_complete) {
                vTaskSuspendAll();
                {
                    jpeg_decode_should_abort = false;
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
            vTaskSuspendAll();
            {
                jpeg_decode_should_abort = false;
            }
            xTaskResumeAll();
        }
    }

    // if (jpeg_decode_temp_file_should_close) {
    //     f_close(&jpeg_decode_temp_file);
    //     jpeg_decode_temp_file_should_close = false;
    // }
    { // Close File
        if (jpeg_decode_target_file_should_close) {
            f_close(&jpeg_decode_target_file);
            vTaskSuspendAll();
            {
                jpeg_decode_target_file_should_close = false;
            }
            xTaskResumeAll();
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
    std::string *message     = nullptr;
    std::string *old_message = nullptr;
    while (true) {
        xQueuePeek(jpeg_decode_ctrl_task_queue, &message, portMAX_DELAY);
        if (old_message && message && *old_message == *message) {
            xQueueReceive(jpeg_decode_ctrl_task_queue, &message, portMAX_DELAY);
            continue;
        }

        xSemaphoreTake(jpeg_decode_task_interrupt_mutex, portMAX_DELAY);
        xQueueReceive(jpeg_decode_ctrl_task_queue, &message, portMAX_DELAY);

        if (old_message && message && *old_message == *message) {
            xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
            continue;
        }

        if (jpeg_decode_task_handle) {
            vTaskDelete(jpeg_decode_task_handle);
            jpeg_decode_task_handle = nullptr;
            delete old_message;
            // if (jpeg_decode_temp_file_should_close) {
            //     f_close(&jpeg_decode_temp_file);
            //     jpeg_decode_temp_file_should_close = false;
            // }
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
            old_message = nullptr;
        }
        if (message) {
            old_message                          = message;
            jpeg_decode_target_file_should_close = false;
            xTaskCreate(jpeg_decode_task, "JPEG Decode Task", 2048, old_message, 3, &jpeg_decode_task_handle);
        }
        xSemaphoreGive(jpeg_decode_task_interrupt_mutex);
    }
}

static void click_one_file(lv_event_t *e, const char *path) {
    auto        *the_file_path = new std::string(path);
    std::string *peek_message  = nullptr;
    vTaskSuspendAll();
    {
        if (xQueuePeek(jpeg_decode_ctrl_task_queue, peek_message, 0) == pdPASS) {
            delete peek_message;
        }
        xQueueOverwrite(jpeg_decode_ctrl_task_queue, &the_file_path);
    }
    xTaskResumeAll();
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
