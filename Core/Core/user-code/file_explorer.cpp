
#include "file_explorer.hpp"

#include "core/lv_obj_class_private.h"
#include "core/lv_obj_private.h"

#define P_BUF_IDX user_data->buffer_idx[0]
#define C_BUF_IDX user_data->buffer_idx[1]
#define N_BUF_IDX user_data->buffer_idx[2]

#define P_BUF_BEG user_data->buffer_idx_begin[P_BUF_IDX]
#define C_BUF_BEG user_data->buffer_idx_begin[C_BUF_IDX]
#define N_BUF_BEG user_data->buffer_idx_begin[N_BUF_IDX]

#define P_BUF_LEN user_data->buffer_length[P_BUF_IDX]
#define C_BUF_LEN user_data->buffer_length[C_BUF_IDX]
#define N_BUF_LEN user_data->buffer_length[N_BUF_IDX]

#define P_BUF     user_data->info_buffer[P_BUF_IDX]
#define C_BUF     user_data->info_buffer[C_BUF_IDX]
#define N_BUF     user_data->info_buffer[N_BUF_IDX]

struct file_explorer_styles {
    lv_style_t file_explorer_style;
    lv_style_t container_style;
    struct {
        lv_style_t default_style;
        lv_style_t press_style;
    } item_styles;
};

struct file_explorer_data {
    file_explorer_styles      style;
    lv_style_transition_dsc_t transition;
    lv_style_prop_t          *transition_props;
    lv_obj_t                 *container;    // the container: item's parent
    lv_obj_t                **total_items;  // items; elements = item count
    FILINFO                 **info_buffer;  // file info to show
    char                     *current_path; // path; detect the folder
    int32_t                   buffer_idx_begin[3];
    int32_t                   buffer_length[3];
    int32_t                   buffer_idx[3];
    int32_t                   total_count;   // current path's file count
    int32_t                   vis_count;     // how many items can be seen now
    int32_t                   item_count;    // how many items is used now
    int32_t                   item_height;   // the item height
    int32_t                   virt_curr_idx; // logic curr_idx
    int32_t                   first_idx;     // the first item you can see
    int32_t                   last_idx;      // the last item you can see
    bool                      reload_pending;
    bool                      should_reload;
};

static void file_explorer_init_style(lv_obj_t *fe_obj);
static void file_explorer_use_style(lv_obj_t *fe_obj);
static void file_explorer_relocate_items(lv_obj_t *fe_obj, int32_t from_pos);
static void file_explorer_rename_items(lv_obj_t *fe_obj);
static void file_explorer_callback(lv_event_t *e);
static void file_explorer_item_callback(lv_event_t *e);
static void file_explorer_use_style_items(lv_obj_t *fe_obj);
static bool file_explorer_opendir(lv_obj_t *fe_obj);
static bool file_explorer_load_next_page(lv_obj_t *fe_obj);
static bool file_explorer_load_prev_page(lv_obj_t *fe_obj);
static bool file_explorer_switch_to_next_page(lv_obj_t *fe_obj);
static bool file_explorer_switch_to_prev_page(lv_obj_t *fe_obj);

lv_obj_t   *file_explorer_create(lv_obj_t *parent, int32_t item_height, const char *start_path) {
    lv_obj_t *fe_obj = lv_obj_create(parent);
    lv_obj_t *cont   = lv_obj_create(fe_obj);
    // user_data
    auto *user_data             = (file_explorer_data *)pvPortMalloc(sizeof(file_explorer_data));
    user_data->container        = cont;
    user_data->item_height      = item_height;
    user_data->total_count      = 0;
    user_data->vis_count        = 0;
    user_data->item_count       = 0;
    user_data->first_idx        = 0;
    user_data->last_idx         = 0;
    user_data->virt_curr_idx    = 0;
    user_data->total_items      = nullptr;
    user_data->transition_props = (lv_style_prop_t *)pvPortMalloc(4 * sizeof(lv_style_prop_t));
    user_data->current_path     = (char *)pvPortMalloc(512);
    user_data->info_buffer      = (FILINFO **)pvPortMalloc(sizeof(FILINFO *) * 3);
    user_data->reload_pending   = false;
    for (int i = 0; i < 3; i++) {
        user_data->info_buffer[i]   = (FILINFO *)pvPortMalloc(sizeof(FILINFO) * 32);
        user_data->buffer_length[i] = 0;
    }
    lv_strcpy(user_data->current_path, start_path);

    lv_obj_set_user_data(fe_obj, user_data);

    file_explorer_init_style(fe_obj);
    file_explorer_use_style(fe_obj);

    lv_obj_update_layout(fe_obj);
    int32_t parent_height  = lv_obj_get_height(fe_obj);
    int32_t item_count     = (parent_height / item_height) + 1 + !!(parent_height % item_height);
    user_data->item_count  = item_count;

    user_data->total_items = (lv_obj_t **)pvPortMalloc(sizeof(lv_obj_t *) * item_count);
    for (int i = 0; i < item_count; i++) {
        lv_obj_t *item = lv_obj_create(cont);
        lv_label_create(item);
        user_data->total_items[i] = item;
    }

    P_BUF_IDX = 0; // 0 -> before
    C_BUF_IDX = 1; // 1 -> current
    N_BUF_IDX = 2; // 2 -> next
    P_BUF_BEG = -1;
    C_BUF_BEG = 0;
    N_BUF_BEG = 32;
    file_explorer_opendir(fe_obj);

    file_explorer_use_style_items(fe_obj);
    file_explorer_relocate_items(fe_obj, 0);
    file_explorer_rename_items(fe_obj);

    for (int i = 0; i < item_count; i++)
        lv_obj_add_event_cb(user_data->total_items[i], file_explorer_item_callback, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(fe_obj, file_explorer_callback, LV_EVENT_ALL, nullptr);

    return fe_obj;
}

static bool file_explorer_opendir(lv_obj_t *fe_obj) {
    auto   *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));

    int32_t count     = 0;
    DIR     current_dir;
    FILINFO file_info;
    FRESULT fres = f_opendir(&current_dir, user_data->current_path);
    if (fres != FR_OK) {
        f_closedir(&current_dir);
        jprintf(0, "Can't Open Directory %s\n", user_data->current_path);
        return false;
    }

    // reset all length
    P_BUF_LEN = 0;
    C_BUF_LEN = 0;
    N_BUF_LEN = 0;

    while (true) {
        fres = f_readdir(&current_dir, &file_info);
        if (fres != FR_OK || (file_info.fname[0] == 0)) {
            break;
        }
        count++;
        if (C_BUF_LEN < 32) {
            lv_memcpy(&(C_BUF[C_BUF_LEN]), &file_info, sizeof(FILINFO));
            C_BUF_LEN++;
        }
        else if (N_BUF_LEN < 32) {
            lv_memcpy(&(N_BUF[N_BUF_LEN]), &file_info, sizeof(FILINFO));
            N_BUF_LEN++;
        }
    }
    f_closedir(&current_dir);

    user_data->total_count = count;
    if (count > (user_data->item_count - 1))
        user_data->vis_count = (user_data->item_count - 1);
    else
        user_data->vis_count = count;

    user_data->last_idx = user_data->vis_count - 1;

    return true;
}

static bool file_explorer_load_next_page(lv_obj_t *fe_obj) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    if (N_BUF_BEG < 0)
        return false;
    DIR     curr_dir;
    FRESULT fres;
    FILINFO file_info;

    fres = f_opendir(&curr_dir, user_data->current_path);
    if (fres != FR_OK) {
        f_closedir(&curr_dir);
        return false;
    }

    for (int32_t i = 0; i < N_BUF_BEG; i++) {
        fres = f_readdir(&curr_dir, &file_info);
        if (fres != FR_OK) {
            f_closedir(&curr_dir);
            return false;
        }
    }

    while (true) {
        if (N_BUF_LEN < 32) {
            fres = f_readdir(&curr_dir, &file_info);
            if (fres != FR_OK) {
                f_closedir(&curr_dir);
                return false;
            }
            lv_memcpy(&(N_BUF[N_BUF_LEN]), &file_info, sizeof(FILINFO));
            N_BUF_LEN++;
        }
        else {
            f_closedir(&curr_dir);
            break;
        }
    }

    return true;
}

static bool file_explorer_load_prev_page(lv_obj_t *fe_obj) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    if (P_BUF_BEG < 0)
        return false;
    DIR     curr_dir;
    FRESULT fres;
    FILINFO file_info;

    fres = f_opendir(&curr_dir, user_data->current_path);
    if (fres != FR_OK) {
        f_closedir(&curr_dir);
        return false;
    }

    for (int32_t i = 0; i < P_BUF_BEG; i++) {
        fres = f_readdir(&curr_dir, &file_info);
        if (fres != FR_OK) {
            f_closedir(&curr_dir);
            return false;
        }
    }

    while (true) {
        if (P_BUF_LEN < 32) {
            fres = f_readdir(&curr_dir, &file_info);
            if (fres != FR_OK) {
                f_closedir(&curr_dir);
                return false;
            }
            lv_memcpy(&(P_BUF[P_BUF_LEN]), &file_info, sizeof(FILINFO));
            P_BUF_LEN++;
        }
        else {
            f_closedir(&curr_dir);
            break;
        }
    }

    return true;
}

static bool file_explorer_switch_to_next_page(lv_obj_t *fe_obj) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    if (N_BUF_BEG < -1)
        return false;
    //
    int32_t temp = P_BUF_IDX;
    P_BUF_IDX    = C_BUF_IDX;
    C_BUF_IDX    = N_BUF_IDX;
    N_BUF_IDX    = temp;
    //
    P_BUF_BEG = C_BUF_BEG;
    C_BUF_BEG = N_BUF_BEG;
    N_BUF_BEG += 32;
    if (N_BUF_BEG >= user_data->total_count) {
        N_BUF_BEG = -1;
    }
    //
    user_data->buffer_length[N_BUF_IDX] = 0;
    user_data->virt_curr_idx -= 32;
    return true;
}

static bool file_explorer_switch_to_prev_page(lv_obj_t *fe_obj) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    if (P_BUF_IDX < -1)
        return false;
    //
    int32_t temp = N_BUF_IDX;
    N_BUF_IDX    = C_BUF_IDX;
    C_BUF_IDX    = P_BUF_IDX;
    P_BUF_IDX    = temp;
    //
    N_BUF_IDX = C_BUF_BEG;
    C_BUF_BEG = P_BUF_IDX;
    P_BUF_IDX -= 32;
    if (P_BUF_IDX < 0) {
        P_BUF_IDX = -1;
    }
    //
    user_data->buffer_length[P_BUF_IDX] = 0;
    user_data->virt_curr_idx += 32;
    return true;
}

static void file_explorer_init_style(lv_obj_t *fe_obj) {
    auto *user_data                = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    auto &fe_style                 = user_data->style;

    user_data->transition_props[0] = LV_STYLE_BG_COLOR;
    user_data->transition_props[1] = LV_STYLE_BG_OPA;
    user_data->transition_props[2] = 0;

    lv_style_init(&fe_style.file_explorer_style);
    lv_style_init(&fe_style.container_style);
    lv_style_init(&fe_style.item_styles.default_style);
    lv_style_init(&fe_style.item_styles.press_style);

    lv_style_set_size(&fe_style.file_explorer_style, lv_pct(40), lv_pct(100));
    lv_style_set_flex_flow(&fe_style.file_explorer_style, LV_FLEX_FLOW_COLUMN);
    lv_style_set_pad_all(&fe_style.file_explorer_style, 0);
    lv_style_set_border_color(&fe_style.file_explorer_style, lv_color_hex(0xA0A0A0));

    lv_style_set_size(&fe_style.item_styles.default_style, lv_pct(100), user_data->item_height);
    lv_style_set_bg_color(&fe_style.item_styles.default_style, lv_color_white());
    lv_style_set_radius(&fe_style.item_styles.default_style, 0);
    lv_style_set_border_width(&fe_style.item_styles.default_style, 1);
    lv_style_set_border_side(&fe_style.item_styles.default_style, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_color(&fe_style.item_styles.default_style, lv_color_hex(0xE0E0E0));
    lv_style_set_pad_all(&fe_style.item_styles.default_style, 10);

    lv_style_set_bg_color(&fe_style.item_styles.press_style, lv_color_hex(0x999999));

    lv_style_transition_dsc_init(&user_data->transition, user_data->transition_props, lv_anim_path_ease_out, 200, 0, nullptr);
    lv_style_set_transition(&fe_style.item_styles.default_style, &user_data->transition);
    lv_style_set_transition(&fe_style.item_styles.press_style, &user_data->transition);
}

static void file_explorer_use_style(lv_obj_t *fe_obj) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    auto &fe_style  = user_data->style;
    lv_obj_remove_style_all(user_data->container);
    lv_obj_clear_flag(user_data->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_margin_hor(user_data->container, 15,
                                (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

    lv_obj_add_style(fe_obj, &fe_style.file_explorer_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
}

static void file_explorer_use_style_items(lv_obj_t *fe_obj) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    auto &fe_style  = user_data->style;

    lv_style_set_size(&fe_style.container_style, lv_pct(100), user_data->total_count * user_data->item_height);
    lv_obj_add_style(user_data->container, &fe_style.container_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);

    for (int i = 0; i < user_data->item_count; i++) {
        lv_obj_add_style(user_data->total_items[i], &fe_style.item_styles.default_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_DEFAULT);
        lv_obj_add_style(user_data->total_items[i], &fe_style.item_styles.press_style, (lv_style_selector_t)LV_PART_MAIN | (lv_style_selector_t)LV_STATE_PRESSED);
    }
}

static void file_explorer_relocate_items(lv_obj_t *fe_obj, int32_t from_pos) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    for (int i = 0; i < user_data->item_count; i++) {
        lv_obj_set_pos(user_data->total_items[i], 0, from_pos + (user_data->item_height * i));
    }
}

static void file_explorer_rename_items(lv_obj_t *fe_obj) {
    auto *user_data = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    for (int i = 0; i < user_data->item_count; i++) {
        lv_obj_t *item_label = lv_obj_get_child(user_data->total_items[i], 0);
        if (i < user_data->vis_count) {
            int n = user_data->virt_curr_idx + i;
            if (n < 0) {
                lv_label_set_text_static(item_label, P_BUF[32 + n].fname);
            }
            else if (n > 31) {
                lv_label_set_text_static(item_label, N_BUF[n - 32].fname);
            }
            else {
                lv_label_set_text_static(item_label, C_BUF[n].fname);
            }
        }
        else {
            lv_obj_add_flag(user_data->total_items[i], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(item_label, "");
        }
    }
}

static void file_explorer_callback(lv_event_t *e) {
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t       *fe_obj     = lv_event_get_current_target_obj(e);
    auto           *user_data  = static_cast<file_explorer_data *>(lv_obj_get_user_data(fe_obj));
    switch (event_code) {
    case LV_EVENT_DRAW_POST: {
        if (user_data->reload_pending) {
            // reload the current directory
            if (file_explorer_opendir(fe_obj)) {
                // reload
                user_data->reload_pending = false;
                user_data->virt_curr_idx  = 0;
                user_data->first_idx      = 0;
                lv_obj_scroll_to_y(fe_obj, 0, LV_ANIM_OFF);
                file_explorer_relocate_items(fe_obj, 0);
                file_explorer_rename_items(fe_obj);
            }
            else {
                jprintf(0, "Can't Reload the directory: %s\n", user_data->current_path);
            }
        }
        break;
    }
    case LV_EVENT_SCROLL: {
        lv_coord_t fe_scroll_top = lv_obj_get_scroll_top(fe_obj);
        lv_coord_t fe_scroll_btm = lv_obj_get_scroll_bottom(fe_obj);

        int32_t    height        = lv_obj_get_height(fe_obj);
        int32_t    new_first_idx = fe_scroll_top / user_data->item_height;
        int32_t    new_last_idx =
            ((fe_scroll_top + height) / user_data->item_height) +
            !!((fe_scroll_top + height) % user_data->item_height);

        if (new_last_idx > user_data->total_count) {
            new_last_idx = user_data->total_count;
        }

        if (new_first_idx != user_data->first_idx &&
            new_first_idx >= 0 && fe_scroll_btm >= 0 &&
            new_last_idx != user_data->last_idx) {
            user_data->virt_curr_idx += (new_first_idx - user_data->first_idx);
            user_data->first_idx = new_first_idx;
            user_data->last_idx  = new_last_idx;
            file_explorer_relocate_items(fe_obj, user_data->first_idx * user_data->item_height);
            file_explorer_rename_items(fe_obj);

            int n = user_data->virt_curr_idx + user_data->vis_count;
            int m = user_data->virt_curr_idx;

            if (m < -15) {
                file_explorer_switch_to_prev_page(fe_obj);
                file_explorer_load_prev_page(fe_obj);
            }
            else if (n > 15) {
                file_explorer_switch_to_next_page(fe_obj);
                file_explorer_load_next_page(fe_obj);
            }
        }

        break;
    }
    default: {
    }
    }
}

static void file_explorer_item_callback(lv_event_t *e) {
    jprintf(0, ANSI_COLOR_FG_BRIGHT_GREEN "lv_event_clicked\n");
}
