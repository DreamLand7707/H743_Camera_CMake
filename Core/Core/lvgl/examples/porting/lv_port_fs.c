/**
 * @file lv_port_fs_templ.c
 *
 */

/*Copy this file as "lv_port_fs.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_fs.h"
#include "fatfs.h"
#include "main.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fs_init(void);

static void* fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
//static lv_fs_res_t fs_size(lv_fs_drv_t *drv, void *file_p, uint32_t *size_p);
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);

static void* fs_dir_open(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn, uint32_t fn_len);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p);
static bool fs_ready(lv_fs_drv_t *drv);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_fs_init(void) {
    /*----------------------------------------------------
     * Initialize your storage device and File System
     * -------------------------------------------------*/
    fs_init();

    /*---------------------------------------------------
     * Register the file system interface in LVGL
     *--------------------------------------------------*/

    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    /*Set up fields...*/
    fs_drv.letter = 'S';
    fs_drv.ready_cb = fs_ready;
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;

    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static FRESULT file_system_res;
/*Initialize your Storage device and File system.*/
static void fs_init(void) {
    /*E.g. for FatFS initialize the SD card and FatFS itself*/
    if (!sdcard_is_mounted) {
        for (int i = 0; i < 10; i++) {
            file_system_res = f_mount(&SDFatFS, SDPath, 1);
            if (file_system_res != FR_OK) continue;
            else break;
        }
        if (file_system_res == FR_NO_FILESYSTEM) {
            jprintf(0, "IN LVGL: The card don't have filesystem, Now Format it\n");
            file_system_res = f_mkfs(SDPath, FM_FAT32, 4096, (void*) mkfs_buffer, 4096);
            if (file_system_res == FR_OK) {
                jprintf(0, "IN LVGL: Format success\n");
                file_system_res = f_mount(&SDFatFS, SDPath, 1);
                jprintf(0, "IN LVGL: Mount Message: %d\n", file_system_res);
            }
            else jprintf(0, "IN LVGL: Format Failed! Message: %d\n", file_system_res);
        }
        else if (file_system_res != FR_OK) {
            jprintf(0, "IN LVGL: Mount Failed! Message: %d\n", file_system_res);
        }
        sdcard_is_mounted  = 1;
    }
    /*You code here*/
}

static bool fs_ready(lv_fs_drv_t *drv) {
    return (file_system_res == FR_OK);
}

/**
 * Open a file
 * @param drv       pointer to a driver where this function belongs
 * @param path      path to the file beginning with the driver letter (e.g. S:/folder/file.txt)
 * @param mode      read: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @return          a file descriptor or NULL on error
 */

static char complete_path[256];

static void* fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) {
    void *f = NULL;
    FRESULT f_res;
    FIL *file = (FIL*) lv_malloc(sizeof(FIL));
    strcpy(complete_path, "0:/");
    strcat(complete_path, path);

    if (mode == LV_FS_MODE_WR) {
        f_res = f_open(file, complete_path, FA_WRITE);
        if (f_res == FR_OK) f = file;
    }
    else if (mode == LV_FS_MODE_RD) {
        f_res = f_open(file, complete_path, FA_READ);
        if (f_res == FR_OK) f = file;
    }
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) {
        f_res = f_open(file, complete_path, FA_WRITE | FA_READ);
        if (f_res != FR_OK) f = file;
    }

    return f;
}

/**
 * Close an opened file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable. (opened with fs_open)
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p) {
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;

    /*Add your code here*/
    FIL *file = (FIL*) file_p;
    FRESULT f_res;
    f_res = f_close(file);
    lv_free(file_p);
    if (f_res != FR_OK) res = LV_FS_RES_FS_ERR;

    else res = LV_FS_RES_OK;

    return res;
}

/**
 * Read data from an opened file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable.
 * @param buf       pointer to a memory block where to store the read data
 * @param btr       number of Bytes To Read
 * @param br        the real number of read bytes (Byte Read)
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br) {
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;
    FIL *file = (FIL*) file_p;
    FRESULT f_res;
    f_res = f_read(file, buf, (UINT) btr, (UINT*) br);
    if (f_res != FR_OK) res = LV_FS_RES_FS_ERR;

    else res = LV_FS_RES_OK;

    /*Add your code here*/

    return res;
}

/**
 * Write into a file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable
 * @param buf       pointer to a buffer with the bytes to write
 * @param btw       Bytes To Write
 * @param bw        the number of real written bytes (Bytes Written). NULL if unused.
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw) {
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;

    FIL *file = (FIL*) file_p;
    FRESULT f_res;
    f_res = f_write(file, buf, (UINT) btw, (UINT*) bw);
    if (f_res != FR_OK) res = LV_FS_RES_FS_ERR;

    else res = LV_FS_RES_OK;

    return res;
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable. (opened with fs_open )
 * @param pos       the new position of read write pointer
 * @param whence    tells from where to interpret the `pos`. See @lv_fs_whence_t
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence) {
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;

    /*Add your code here*/
    FIL *file = (FIL*) file_p;
    FRESULT f_res;
    if (whence == LV_FS_SEEK_SET) f_res = f_lseek(file, (FSIZE_t) pos);
    else if (whence == LV_FS_SEEK_CUR) f_res = f_lseek(file, f_tell(file) + pos);
    else if (whence == LV_FS_SEEK_END) f_res = f_lseek(file, f_size(file) - pos);

    if (f_res != FR_OK) res = LV_FS_RES_FS_ERR;
    else res = LV_FS_RES_OK;

    return res;
}
//static lv_fs_res_t fs_size(lv_fs_drv_t *drv, void *file_p, uint32_t *size_p) {
//	lv_fs_res_t res = LV_FS_RES_OK;
//	FIL *file = (FIL*) file_p;
//	*size_p = (uint32_t) f_size(file);
//	return res;
//}
/**
 * Give the position of the read write pointer
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable
 * @param pos_p     pointer to store the result
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p) {
    lv_fs_res_t res = LV_FS_RES_OK;
    FIL *file = (FIL*) file_p;
    *pos_p = (uint32_t) f_tell(file);
    return res;
}

/**
 * Initialize a 'lv_fs_dir_t' variable for directory reading
 * @param drv       pointer to a driver where this function belongs
 * @param path      path to a directory
 * @return          pointer to the directory read descriptor or NULL on error
 */
static void* fs_dir_open(lv_fs_drv_t *drv, const char *path) {
    void *dir = NULL;
    FRESULT f_res;
    /*Add your code here*/
    DIR *direct = (DIR*) lv_malloc(sizeof(DIR));
    strcpy(complete_path, "0:/");
    strcat(complete_path, path);
    f_res = f_opendir(direct, complete_path);
    if (f_res == FR_OK) dir = direct;

    return dir;
}

/**
 * Read the next filename form a directory.
 * The name of the directories will begin with '/'
 * @param drv       pointer to a driver where this function belongs
 * @param rddir_p   pointer to an initialized 'lv_fs_dir_t' variable
 * @param fn        pointer to a buffer to store the filename
 * @param fn_len    length of the buffer to store the filename
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn, uint32_t fn_len) {
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;
    DIR *direct = (DIR*) rddir_p;
    FILINFO f_info;
    FRESULT f_res;
    f_res = f_readdir(direct, &f_info);
    strncpy(fn, f_info.fname, fn_len);
    if (f_res != FR_OK) res = LV_FS_RES_FS_ERR;

    else res = LV_FS_RES_OK;

    /*Add your code here*/

    return res;
}

/**
 * Close the directory reading
 * @param drv       pointer to a driver where this function belongs
 * @param rddir_p   pointer to an initialized 'lv_fs_dir_t' variable
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p) {
    lv_fs_res_t res = LV_FS_RES_NOT_IMP;
    DIR *direct = (DIR*) rddir_p;
    FRESULT f_res;
    f_res = f_closedir(direct);
    lv_free(rddir_p);
    if (f_res != FR_OK) res = LV_FS_RES_FS_ERR;

    else res = LV_FS_RES_OK;

    /*Add your code here*/

    return res;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
