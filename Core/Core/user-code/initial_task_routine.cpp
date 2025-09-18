#include "prj_header.hpp"
#include "jpeg_utils.h"

#include "SEGGER_RTT.h"
#include <ranges>

// extern Variable Definition
void *					SDRAM_GRAM1;
void *					SDRAM_GRAM2;
void *					JPEG_ENCODE_DEST;
void *					JEPG_YCbCr;
void *					JPEG_ENCODE_SOURCE;

uint8_t 				mkfs_buffer[4096] 				__attribute__((section(".fatfs_buffer")));
int 					sdcard_link_driver 							= 0;
int 					sdcard_disk_init 							= 0;
int 					sdcard_is_mounted 							= 0;
int 					sdcard_initialized 							= 0;

SemaphoreHandle_t 		sema_flash_screen_routine_start;
SemaphoreHandle_t 		sema_camera_routine_start;
SemaphoreHandle_t 		sema_timer_handle;

SemaphoreHandle_t 		sema_33ms_flash_screen;
TimerHandle_t 			timer_33ms_flash_screen;


// static variable Definition
namespace {
lv_obj_t *	myBtn;
lv_obj_t *	label_btn;
lv_obj_t *	myLabel;
lv_obj_t *	shot_btn;
lv_obj_t *	shot_btn_label;
lv_obj_t * 	pop_sd;
lv_obj_t *	ins_sd;
lv_obj_t * 	pop_sd_label;
lv_obj_t *	ins_sd_label;

lv_obj_t *	spangroup_messages;
lv_style_t  spangroup_style;
lv_span_t * span;
lv_style_t  spaninfo_style;
lv_style_t  spanwarning_style;
lv_style_t  spanerror_style;


TaskHandle_t 		render_sync_daemon;
TaskHandle_t 		take_screenshot;
JPEG_ConfTypeDef 	jpeg_conf;
SemaphoreHandle_t 	mutex_gram_read;
SemaphoreHandle_t 	sema_take_screenshot;
SemaphoreHandle_t 	sema_update_local;
char 				buff_str[40];
int 				count = 0, i = 0;
uint8_t *			curr_encode_ptr;
uint8_t *			target_encode_ptr;
uint8_t *			curr_dest_ptr;
FIL 				jpeg1;

SemaphoreHandle_t   mutex_internal_lvgl;

void *				curr_screen_buffer;

}

// extern Variable Decl
LV_FONT_DECLARE(Camera_1_bit)

// MARCO
#define MY_CAMERA_SYMBOL "\xEF\x80\xB0"
#define mprint 		 	 screen_print_message
#define mclear 		 	 screen_clear_message
#define mprintf 		 screen_printf_message
#define sd_enable        ((sdcard_disk_init) & (sdcard_is_mounted) & (sdcard_link_driver))

// type decl
enum class scr_mess;

// static function decl
static void lvgl_initialize_port1					();
static void lvgl_initialize_port2					();
static void sdcard_initialize						();
static void MYSCB_CleanInvalidateDCache_by_AddrRange(uint32_t *pData_begin, uint32_t *pData_end);
static void timer_500ms_internal_callback			(TimerHandle_t xTimer);
static void render_sync_daemon_task					(void *params);
static void take_screenshot_task					(void *params);
static void hdma2dCompleteCallback					(DMA2D_HandleTypeDef *hdma2d);
static void screen_print_message					(scr_mess message_type, const char *message);
static void screen_printf_message					(scr_mess message_type, const char *f_message, ...);
static void screen_clear_message					(scr_mess message_type);

// extern function But Link to Library
extern "C" int 	_getentropy(void *buffer, size_t length);

enum class scr_mess {
	INFO = 0,
	WARN = 1,
	ERRO = 2,
	ALL  = 3
};

/////////////////////
// Button Callback //
/////////////////////

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

	mclear(scr_mess::ALL);
	mprint(scr_mess::INFO, "Try To Insert...\n");

	if (!sdcard_link_driver && FATFS_LinkDriver(&SD_Driver, SDPath) != 0) {
		mprint(scr_mess::ERRO, "Can't Link Driver!\n");
		return;
	}
	sdcard_link_driver = 1;
	if (!sdcard_disk_init && SD_Driver.disk_initialize(0) != RES_OK) {
		HAL_SD_DeInit(&hsd1);
		mprint(scr_mess::ERRO, "Can't Initialize Disk Card!\n");
		return;
	}
	sdcard_disk_init = 1;
	if (!sdcard_is_mounted && f_mount(&SDFatFS, SDPath, 1) != FR_OK) {
		mprint(scr_mess::ERRO, "Can't Mount SD Card!\n");
		return;
	}
	sdcard_is_mounted = 1;
	mprint(scr_mess::INFO, "Ins Success!\n");

}
void popbtn_call(lv_event_t *event) {

	mclear(scr_mess::ALL);
	mprint(scr_mess::INFO, "Try To Pop...\n");

	if (sdcard_is_mounted && f_mount(nullptr, SDPath, 1) != FR_OK) {
		mprint(scr_mess::ERRO, "Can't UnMount SD Card!\n");
		return;
	}
	sdcard_is_mounted = 0;
	if (sdcard_disk_init && HAL_SD_DeInit(&hsd1) != HAL_OK) {
		mprint(scr_mess::ERRO, "Can't DeInit SD Port!\n");
		return;
	}
	sdcard_disk_init = 0;
	if (sdcard_link_driver && FATFS_UnLinkDriver(SDPath) != 0) {
		mprint(scr_mess::ERRO, "Can't Unlink Driver!\n");
		return;
	}
	sdcard_link_driver = 0;
	mprint(scr_mess::INFO, "Pop Success!\n");
}

/*************************************
//////////////////
// TRUE ROUTINE //
//////////////////
*************************************/

static int routine(int argc, char **argv) {
	lvgl_initialize_port1();
	sdcard_initialize();
	lvgl_initialize_port2();

	xSemaphoreGive(sema_flash_screen_routine_start);
	xSemaphoreGive(sema_camera_routine_start);

	osThreadSetPriority(Initial_TaskHandle, osPriorityNormal);

	vQueueAddToRegistry((QueueHandle_t) sema_update_local, "sema_update_local");
	vQueueAddToRegistry((QueueHandle_t) mutex_gram_read, "mutex_gram_read");
	vQueueAddToRegistry((QueueHandle_t) sema_take_screenshot, "sema_take_screenshot");

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
			if (count % 2) lv_label_set_text(myLabel, buff_str);
			else lv_label_set_text(label_btn, buff_str);
		}
		lv_timer_handler();
		xSemaphoreGive(mutex_internal_lvgl);
		vTaskDelay(1);
	}
}

/////////////////
// static task //
/////////////////

static void timer_500ms_internal_callback(TimerHandle_t xTimer) {
	sprintf(buff_str, "%d", i);
	i++;
	count++;
	i = i % 10;
	xSemaphoreGive(sema_update_local);
}

static void render_sync_daemon_task(void *params) {
	while (1) {
		xSemaphoreTake(sema_timer_handle, portMAX_DELAY);
		xSemaphoreTake(mutex_gram_read, portMAX_DELAY);
		lv_display_flush_ready(rgb_screen_disp);
//		ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
		xSemaphoreGive(mutex_gram_read);
	}
}
static void take_screenshot_task(void *params) {
	uint32_t notification_value = 0;
	while (1) {
		xSemaphoreTake(sema_take_screenshot, portMAX_DELAY);
		xSemaphoreTake(mutex_gram_read, portMAX_DELAY);
		mclear(scr_mess::ALL);
		mprint(scr_mess::INFO, "Begin: Take screenshot!\n");
		HAL_DMA2D_Start_IT(&hdma2d, (uint32_t) curr_screen_buffer, (uint32_t) JPEG_ENCODE_SOURCE, 480, 272);
		while (1) {
			xTaskNotifyWait(0, 0x01, &notification_value, portMAX_DELAY);
			if (notification_value & 0x01) {
				xTaskNotify(xTaskGetCurrentTaskHandle(), 0, eNoAction);
				break;
			}
		}
		xSemaphoreGive(mutex_gram_read);

		vTaskPrioritySet(xTaskGetCurrentTaskHandle(), uxTaskPriorityGet(Initial_TaskHandle));
		JPEG_RGBToYCbCr_Convert_Function conv_func;
		uint32_t nbMcu;
		JPEG_GetEncodeColorConvertFunc(&jpeg_conf, &conv_func, &nbMcu);

		uint32_t byte_consume, nb_Mcu_truth;
		nb_Mcu_truth = conv_func((uint8_t*) JPEG_ENCODE_SOURCE, (uint8_t*) JEPG_YCbCr, 0, sizeof(SDRAM_SCREEN_BUFFER), &byte_consume);

		curr_encode_ptr = (uint8_t*) JEPG_YCbCr;
		target_encode_ptr = (uint8_t*) JEPG_YCbCr + byte_consume;
		curr_dest_ptr = (uint8_t*) JPEG_ENCODE_DEST;
		HAL_JPEG_Encode_DMA(&hjpeg, curr_encode_ptr, 65536, curr_dest_ptr, 65536);
		while (1) {
			xTaskNotifyWait(0, 0x02, &notification_value, portMAX_DELAY);
			if (notification_value & 0x02) {
				xTaskNotify(xTaskGetCurrentTaskHandle(), 0, eNoAction);
				break;
			}
		}
		mprint(scr_mess::INFO, "Middle: Success Take screenshot!\n");
		if (!sd_enable) {
			mprint(scr_mess::ERRO, "ERROR: SD Card is Not enabled\n");
			continue;
		}
		if (f_open(&jpeg1, "0:/a.jpeg", FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
			mprint(scr_mess::ERRO, "ERROR: Can't Open File\n");
			continue;
		}
		UINT brw;
		if (f_write(&jpeg1, (const uint8_t*) JPEG_ENCODE_DEST, (UINT) (curr_dest_ptr - (uint8_t*) JPEG_ENCODE_DEST), &brw) != FR_OK) {
			mprint(scr_mess::ERRO, "ERROR: Can't Write to File\n");
			continue;
		}
		if (f_close(&jpeg1) != FR_OK) {
			mprint(scr_mess::ERRO, "ERROR: Can't Close File\n");
			continue;
		}
		mprint(scr_mess::INFO, "Final: Storage ScreenShot Success!\n");
	}
}

/////////////////
// extern task //
/////////////////

void timer_33ms_flash_screen_callback(TimerHandle_t xTimer) {
	xSemaphoreGive(sema_33ms_flash_screen);
}


////////////////
// Initialize //
////////////////

// JPEG & FreeRTOS

static void initial_before_routine() {
	mutex_gram_read 		= xSemaphoreCreateMutex();
	sema_take_screenshot 	= xSemaphoreCreateBinary();
	sema_update_local 		= xSemaphoreCreateBinary();
	mutex_internal_lvgl		= xSemaphoreCreateMutex();

	SDRAM_GRAM1 		= sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));
	SDRAM_GRAM2 		= sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));
	JPEG_ENCODE_SOURCE 	= sdram_Malloc(sizeof(SDRAM_SCREEN_BUFFER));
	JPEG_ENCODE_DEST   	= sdram_Malloc(256 * 1024);
	JEPG_YCbCr			= sdram_Malloc(256 * 1024);

	HAL_LTDC_SetAddress(&hltdc, (uint32_t) SDRAM_GRAM1, LTDC_LAYER_1);

	hdma2d.XferCpltCallback = hdma2dCompleteCallback;
	jpeg_conf.ChromaSubsampling = JPEG_444_SUBSAMPLING;
	jpeg_conf.ColorSpace = JPEG_YCBCR_COLORSPACE;
	jpeg_conf.ImageQuality = 100;
	jpeg_conf.ImageHeight = 272;
	jpeg_conf.ImageWidth = 480;
	HAL_JPEG_ConfigEncoding(&hjpeg, &jpeg_conf);
}

void initial_task_routine(void const *argument) {
	initial_before_routine();
	JPEG_InitColorTables();
	routine(0, nullptr);
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

	spangroup_messages = lv_spangroup_create(lv_screen_active());
	lv_style_init(&spangroup_style);
	lv_style_init(&spaninfo_style);
	lv_style_init(&spanwarning_style);
	lv_style_init(&spanerror_style);

	lv_obj_align(spangroup_messages, LV_ALIGN_BOTTOM_LEFT, 0, 0);
	lv_style_set_border_color(&spangroup_style, lv_color_make(128, 128, 255));
	lv_style_set_border_width(&spangroup_style, 4);
	lv_style_set_pad_all(&spangroup_style, 2);
	lv_style_set_size(&spangroup_style, 240, 136);

	lv_obj_add_style(spangroup_messages, &spangroup_style, 0);
	lv_spangroup_set_mode(spangroup_messages, LV_SPAN_MODE_FIXED);

	lv_style_set_text_color(&spaninfo_style, lv_color_make(0x00, 0x00, 0x00));
	lv_style_set_text_color(&spanwarning_style, lv_palette_darken(LV_PALETTE_YELLOW, 4));
	lv_style_set_text_color(&spanerror_style, lv_palette_darken(LV_PALETTE_RED, 3));

	lv_style_set_text_decor(&spanwarning_style, LV_TEXT_DECOR_UNDERLINE);
	lv_style_set_text_decor(&spanerror_style, LV_TEXT_DECOR_UNDERLINE);

	lv_spangroup_new_span(spangroup_messages);
	lv_spangroup_new_span(spangroup_messages);
	lv_spangroup_new_span(spangroup_messages);
}

static void lvgl_initialize_port2() {
	lv_port_fs_init();
}

static void sdcard_initialize() {
	printf("STATUS: %d\n", SD_Driver.disk_initialize(0));
	sdcard_disk_init = 1;
	print_sdcard_info();

	FRESULT file_system_res;
	file_system_res = f_mount(&SDFatFS, SDPath, 1);
	if (file_system_res == FR_NO_FILESYSTEM) {
		mprint(scr_mess::WARN, "The card don't have filesystem, Now Format it\n");
		file_system_res = f_mkfs(SDPath, FM_FAT32, 4096, (void*) mkfs_buffer, 4096);
		if (file_system_res == FR_OK) {
			mprint(scr_mess::INFO, "Format success\n");
			file_system_res = f_mount(&SDFatFS, SDPath, 1);
			mprintf((file_system_res == FR_OK) ? scr_mess::INFO : scr_mess::ERRO,
					"Mount Message: %d\n", file_system_res);
		}
		else
			mprintf(scr_mess::ERRO, "Format Failed! Message: %d\n", file_system_res);
	}
	else if (file_system_res != FR_OK) {
		mprintf(scr_mess::ERRO, "Format Failed! Message: %d\n", file_system_res);
	}
	sdcard_is_mounted  = 1;

	f_open(&SDFile, "0:/a.txt", FA_WRITE | FA_CREATE_ALWAYS);
	const char *str = "Hello world!";
	UINT expect = strlen(str);
	UINT get = 0;
	f_write(&SDFile, str, expect, &get);
	f_close(&SDFile);
	f_mount(nullptr, SDPath, 1);

	sdcard_is_mounted = 0;
}

////////////////////////////
// Interrupt ISR CallBack //
////////////////////////////

// JPEG

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
		if (length_this_time > 65536) length_this_time = 65536;
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

// DMA 2D

static void hdma2dCompleteCallback(DMA2D_HandleTypeDef *hdma2d) {
	BaseType_t woken;
	xTaskNotifyFromISR(take_screenshot, 0x01, eSetBits, &woken);
	portYIELD_FROM_ISR(woken);
}

// LTDC

void HAL_LTDC_ReloadEventCallback(LTDC_HandleTypeDef *hltdc) {
	BaseType_t woke = pdFALSE;
	xSemaphoreGiveFromISR(sema_timer_handle, &woke);
	portYIELD_FROM_ISR(woke);
}

///////////////////////////
// library port Callback //
///////////////////////////

// FATFS

uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks) {
	uint8_t sd_state = MSD_OK;
	uint32_t address_start = (uint32_t) pData;
	uint32_t address_end = address_start + NumOfBlocks * 512;
	MYSCB_CleanInvalidateDCache_by_AddrRange((uint32_t*) address_start, (uint32_t*) address_end);
	if (HAL_SD_ReadBlocks_DMA(&hsd1, (uint8_t*) pData, ReadAddr, NumOfBlocks) != HAL_OK) {
		sd_state = MSD_ERROR;
	}

	return sd_state;
}

uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *pData, uint32_t WriteAddr, uint32_t NumOfBlocks) {
	uint8_t sd_state = MSD_OK;
	uint32_t address_start = (uint32_t) pData;
	uint32_t address_end = address_start + NumOfBlocks * 512;
	MYSCB_CleanInvalidateDCache_by_AddrRange((uint32_t*) address_start, (uint32_t*) address_end);
	if (HAL_SD_WriteBlocks_DMA(&hsd1, (uint8_t*) pData, WriteAddr, NumOfBlocks) != HAL_OK) {
		sd_state = MSD_ERROR;
	}

	return sd_state;
}

// LVGL

void swapBuffer(void *passbuf) {
	HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t) passbuf, LTDC_LAYER_1);
	HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);
	curr_screen_buffer = passbuf;
	printf("swapBuffer\n");
}

// STD::COUT

int _getentropy(void *buffer, size_t length) {
	return 0;
}

//////////////////////
// Private Function //
//////////////////////

// FLUSH DCACHE

static void MYSCB_CleanInvalidateDCache_by_AddrRange(uint32_t *pData_begin, uint32_t *pData_end) {
	uint32_t address_start = (uint32_t) pData_begin;
	uint32_t address_end = (uint32_t) pData_end + 31;
	address_start &= 0xffffffe0;
	address_end &= 0xffffffe0;
	int32_t real_size = int32_t(address_end - address_start);
	SCB_CleanInvalidateDCache_by_Addr((uint32_t*) address_start, real_size);
}

static void screen_print_message(scr_mess message_type, const char *message) {
	if (xTaskGetCurrentTaskHandle() != Initial_TaskHandle) {
		xSemaphoreTake(mutex_internal_lvgl, portMAX_DELAY);
	}
	switch (message_type) {
	case scr_mess::INFO : {
		span = lv_spangroup_get_child(spangroup_messages, 0);
		lv_span_set_text(span, message);
		*lv_span_get_style(span) = spaninfo_style;
		break;
	}
	case scr_mess::WARN : {
		span = lv_spangroup_get_child(spangroup_messages, 1);
		lv_span_set_text(span, message);
		*lv_span_get_style(span) = spanwarning_style;
		break;
	}
	case scr_mess::ERRO : {
		span = lv_spangroup_get_child(spangroup_messages, 2);
		lv_span_set_text(span, message);
		*lv_span_get_style(span) = spanerror_style;
		break;
	}
	default: {
		break;
	}
	}
	lv_spangroup_refr_mode(spangroup_messages);
	if (xTaskGetCurrentTaskHandle() != Initial_TaskHandle) {
		xSemaphoreGive(mutex_internal_lvgl);
	}
}

static void screen_printf_message(scr_mess message_type, const char *f_message, ...) {
	static char message_buffer[512];
	va_list vl;
	va_start(vl, f_message);
	vsprintf(message_buffer, f_message, vl);
	screen_print_message(message_type, message_buffer);
	va_end(vl);
}

static void screen_clear_message(scr_mess message_type) {
	if (xTaskGetCurrentTaskHandle() != Initial_TaskHandle) {
		xSemaphoreTake(mutex_internal_lvgl, portMAX_DELAY);
	}
	switch (message_type) {
	case scr_mess::INFO: {
		span = lv_spangroup_get_child(spangroup_messages, 0);
		lv_span_set_text(span, "");
		*lv_span_get_style(span) = spaninfo_style;
		break;
	}
	case scr_mess::WARN: {
		span = lv_spangroup_get_child(spangroup_messages, 1);
		lv_span_set_text(span, "");
		*lv_span_get_style(span) = spanwarning_style;
		break;
	}
	case scr_mess::ERRO: {
		span = lv_spangroup_get_child(spangroup_messages, 2);
		lv_span_set_text(span, "");
		*lv_span_get_style(span) = spanerror_style;
		break;
	}
	case scr_mess::ALL: {
		span = lv_spangroup_get_child(spangroup_messages, 0);
		lv_span_set_text(span, "");
		span = lv_spangroup_get_child(spangroup_messages, 1);
		lv_span_set_text(span, "");
		span = lv_spangroup_get_child(spangroup_messages, 2);
		lv_span_set_text(span, "");
	}
	}
	lv_spangroup_refr_mode(spangroup_messages);
	if (xTaskGetCurrentTaskHandle() != Initial_TaskHandle) {
		xSemaphoreGive(mutex_internal_lvgl);
	}
}

/////////////////////
// Extern Function //
/////////////////////

void print_sdcard_info(void) {
	uint64_t CardCap;      	//SD卡容量
	HAL_SD_CardCIDTypeDef SDCard_CID;
	HAL_SD_CardInfoTypeDef SDCardInfo;

	HAL_SD_GetCardCID(&hsd1, &SDCard_CID);	//获取CID
	HAL_SD_GetCardInfo(&hsd1, &SDCardInfo);                    //获取SD卡信息
	CardCap = (uint64_t) (SDCardInfo.LogBlockNbr) * (uint64_t) (SDCardInfo.LogBlockSize);	//计算SD卡容量
	switch (SDCardInfo.CardType) {
	case CARD_SDSC: {
		if (SDCardInfo.CardVersion == CARD_V1_X) jprintf(0, "Card Type:SDSC V1\r\n");
		else if (SDCardInfo.CardVersion == CARD_V2_X) jprintf(0, "Card Type:SDSC V2\r\n");
		break;
	}
	case CARD_SDHC_SDXC: {
		jprintf(0, "Card Type:SDHC\r\n");
		break;
	}
	default:
		break;
	}

	jprintf(0, "Card ManufacturerID: %d \r\n", SDCard_CID.ManufacturerID);	//制造商ID
	jprintf(0, "CardVersion:         %lu \r\n", (uint32_t) (SDCardInfo.CardVersion));		//卡版本号
	jprintf(0, "Class:               %lu \r\n", (uint32_t) (SDCardInfo.Class));	//
	jprintf(0, "Card RCA(RelCardAdd):%lu \r\n", SDCardInfo.RelCardAdd);		//卡相对地址
	jprintf(0, "Card BlockNbr:       %lu \r\n", SDCardInfo.BlockNbr);		//块数量
	jprintf(0, "Card BlockSize:      %lu \r\n", SDCardInfo.BlockSize);		//块大小
	jprintf(0, "LogBlockNbr:         %lu \r\n", (uint32_t) (SDCardInfo.LogBlockNbr));		//逻辑块数量
	jprintf(0, "LogBlockSize:        %lu \r\n", (uint32_t) (SDCardInfo.LogBlockSize));		//逻辑块大小
	jprintf(0, "Card Capacity:       %lu MB\r\n", (uint32_t) (CardCap >> 20));		//卡容量
}

int rgba_equal(BGR *a, BGR *b) {
	return (a->B == b->B) && (a->G == b->G) && (a->R == b->R);
}
