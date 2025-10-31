
#pragma once

#include "prj_header.hpp"
#include "path_func.hpp"

typedef void (*file_explorer_openfile_callback)(lv_event_t *e, const char *path, bool change_dir, FILINFO *file_info);

lv_obj_t *file_explorer_create(lv_obj_t *parent, int32_t item_height, const char *start_path);
void      file_explorer_set_callback(lv_obj_t *fe_obj, file_explorer_openfile_callback callback);
void      file_explorer_reload_force(lv_obj_t *fe_obj);
void      file_explorer_media_invalid(lv_obj_t *fe_obj);
void      file_explorer_media_valid(lv_obj_t *fe_obj);
void      file_explorer_open_dir(lv_obj_t *fe_obj, const char *path);
