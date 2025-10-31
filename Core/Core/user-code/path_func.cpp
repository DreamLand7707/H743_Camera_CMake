
#include "path_func.hpp"
#include <cctype>
#include <vector>
#include <string_view>

bool path_is_absolute(const char *path) {
    size_t n = strlen(path);
    if (n < 3)
        return false;
    if (isalnum(path[0]) && path[1] == ':' && path[2] == '/')
        return true;
    return false;
}

int path_norm(const char *path, char *dest, char *disk_root, char *file_name) {
    size_t n = strlen(path);
    if (!n)
        return 0;

    enum state {
        idle,
        disk,
        normal,
        curr_dir,
        parent_dir,
        unknown
    };

    std::vector<std::pair<std::string_view, bool>> elements;
    size_t                                         pos, pos_bef = 0;
    //
    bool  have_disk        = false;
    bool  have_parent_head = false;
    bool  have_parent      = false;
    bool  have_file_name   = false;
    state curr_state       = idle;
    for (pos = 0;; pos++) {
        if (path[pos] == '/' || path[pos] == 0) {
            if (!pos)
                return -1;
            switch (curr_state) {
            case parent_dir:
                if (elements.empty() || elements.back().second)
                    elements.emplace_back("..", true);
                else if (!have_disk || elements.size() > 1)
                    elements.pop_back();
                else
                    return -1;
                break;
            case normal:
            case disk:
                elements.emplace_back(std::string_view {path + pos_bef, path + pos}, false);
                break;
            default:
                break;
            }
            if (path[pos] == 0)
                break;
            pos_bef    = pos + 1;
            curr_state = idle;
        }
        else if (path[pos] == '.') {
            switch (curr_state) {
            case idle:
                curr_state = curr_dir;
                break;
            case curr_dir:
                curr_state = parent_dir;
                break;
            case parent_dir:
            case disk:
                return -1;
            default:
                curr_state  = normal;
                have_parent = true;
            }
        }
        else if (path[pos] == ':')
            if (curr_state == normal) {
                have_disk   = true;
                have_parent = false;
                curr_state  = disk;
            }
            else
                return -1;
        else if (isprint(path[pos]) && curr_state != disk) {
            curr_state  = normal;
            have_parent = true;
        }
        else
            return -1;
    }
    if (dest) {
        pos = 0;
        for (auto &x : elements) {
            x.first.copy(dest + pos, x.first.size());
            pos += x.first.size();
            dest[pos]     = '/';
            dest[pos + 1] = '\0';
            pos++;
        }
        if (!have_disk || elements.size() > 1)
            dest[pos - 1] = '\0';
    }

    if (disk_root) {
        if (have_disk) {
            elements[0].first.copy(disk_root, elements[0].first.size());
            disk_root[elements[0].first.size()] = '\0';
        }
    }

    if (!have_disk || elements.size() > 1) {
        have_file_name = true;
        if (file_name) {
            elements.back().first.copy(disk_root, elements.back().first.size());
            file_name[elements.back().first.size()] = '\0';
        }
    }

    return (have_disk ? (PATH_IS_ABSOLUTE) : 0) |
           (have_parent_head ? (PATH_IS_NOT_COMPLETE) : 0) |
           (have_parent ? (PATH_HAVE_PARENT) : 0) |
           (have_file_name ? (PATH_HAVE_FILE_NAME) : 0);
}

int path_concat(char *lhs, const char *rhs, char *dest) {
    size_t n   = strlen(lhs);
    lhs[n]     = '/';
    lhs[n + 1] = '\0';
    strcat(lhs, rhs);
    int ret = path_norm(lhs, dest);
    lhs[n]  = '\0';
    return ret;
}

int path_get_disk_root(const char *lhs, char *disk_root) {
    return (path_norm(lhs, disk_root) & PATH_IS_ABSOLUTE) ? 0 : -1;
}

int path_get_file_name(const char *lhs, char *file_name) {
    return (path_norm(lhs, nullptr, file_name) & PATH_HAVE_FILE_NAME) ? 0 : -1;
}

void path_get_file_extension(const char *file_name, char *extension) {
    size_t n = strlen(file_name);
    size_t b = n;
    for (size_t i = n; i > 0; i--) {
        if (file_name[i - 1] == '.') {
            b = i - 1;
            break;
        }
    }
    strncpy(extension, file_name + b, n - b);
}

const char *path_get_file_name_static(const char *file_name) {
    size_t n = strlen(file_name);
    size_t b = n;
    for (size_t i = n; i > 0; i--) {
        if (file_name[i - 1] == '/') {
            b = i - 1;
            break;
        }
    }
    return file_name + b + 1;
}

FRESULT create_directory_recursive(const char *path) {
    FRESULT res;
    char    temp_path[256];
    char   *p = NULL;
    size_t  len;
    int     start_pos = 0;

    // 复制路径到临时缓冲区
    snprintf(temp_path, sizeof(temp_path), "%s", path);
    len = strlen(temp_path);

    // 移除末尾的斜杠
    if (len > 0 && temp_path[len - 1] == '/') {
        temp_path[len - 1] = 0;
        len--;
    }

    // 检查是否有驱动器号（如 "0:" 或 "0:/"）
    if (len >= 2 && temp_path[1] == ':') {
        start_pos = 2; // 跳过驱动器号 "0:"
        if (len > 2 && temp_path[2] == '/') {
            start_pos = 3; // 跳过 "0:/"
        }
    }

    // 如果路径只是驱动器号，直接返回
    if (start_pos >= len) {
        return FR_OK;
    }

    // 从驱动器号后开始遍历路径
    for (p = temp_path + start_pos; *p; p++) {
        if (*p == '/') {
            *p = 0; // 临时截断字符串

            // 检查当前路径是否存在
            FILINFO fno;
            res = f_stat(temp_path, &fno);

            if (res == FR_NO_FILE || res == FR_NO_PATH) {
                // 不存在则创建
                res = f_mkdir(temp_path);
                if (res != FR_OK && res != FR_EXIST) {
                    return res; // 创建失败
                }
            }
            else if (res == FR_OK) {
                // 存在，检查是否为目录
                if (!(fno.fattrib & AM_DIR)) {
                    return FR_DENIED; // 存在同名文件
                }
            }
            else {
                return res; // 其他错误
            }

            *p = '/'; // 恢复斜杠
        }
    }

    // 创建最后一级目录
    res = f_mkdir(temp_path);
    if (res == FR_EXIST) {
        res = FR_OK; // 已存在视为成功
    }

    return res;
}

FRESULT get_directory_path(const char *path, char *dir_path, size_t dir_path_size) {
    FRESULT res;
    FILINFO fno;
    size_t  len;

    if (path == NULL || dir_path == NULL || dir_path_size == 0) {
        return FR_INVALID_PARAMETER;
    }

    len = strlen(path);
    if (len == 0 || len >= dir_path_size) {
        return FR_INVALID_PARAMETER;
    }

    // 先复制路径
    strcpy(dir_path, path);

    // 移除末尾的斜杠（但保留根目录的斜杠）
    while (len > 0 && dir_path[len - 1] == '/') {
        // 保留 "0:/" 或 "/" 这样的根目录斜杠
        if (len == 1) {
            // 只剩一个 "/"
            break;
        }
        if (len == 3 && dir_path[1] == ':') {
            // "0:/" 格式
            break;
        }
        dir_path[len - 1] = 0;
        len--;
    }

    // 检查路径是否存在
    res = f_stat(dir_path, &fno);
    if (res != FR_OK) {
        return res; // 路径不存在或其他错误
    }

    // 如果是文件夹，直接返回
    if (fno.fattrib & AM_DIR) {
        return FR_OK;
    }

    // 如果是文件，找到最后一个斜杠，返回其父文件夹
    char *last_slash = strrchr(dir_path, '/');

    if (last_slash != NULL) {
        // 找到斜杠
        size_t slash_pos = last_slash - dir_path;

        if (slash_pos == 0) {
            // 根目录的文件，如 "/file.txt"
            dir_path[1] = 0; // 返回 "/"
        }
        else if (slash_pos == 2 && dir_path[1] == ':') {
            // 驱动器根目录的文件，如 "0:/file.txt"
            dir_path[3] = 0; // 返回 "0:/"
        }
        else {
            // 普通情况，截断到最后一个斜杠之前
            *last_slash = 0;
        }
    }
    else {
        // 没有斜杠
        if (len >= 2 && dir_path[1] == ':') {
            // 有驱动器号但没有斜杠，如 "0:file.txt"
            dir_path[2] = 0; // 返回 "0:"
        }
        else {
            // 当前目录的文件，返回 "."
            strcpy(dir_path, ".");
        }
    }

    return FR_OK;
}

int is_directory_empty(const char *path) {
    DIR     dir;
    FILINFO fno;
    FRESULT res;

    // 检查是否为根目录
    size_t len = strlen(path);

    // 识别根目录的几种形式: "0:", "0:/", "/"
    if ((len == 2 && path[1] == ':') ||
        (len == 3 && path[1] == ':' && path[2] == '/') ||
        (len == 1 && path[0] == '/')) {
        return 0;
    }

    // 打开目录
    res = f_opendir(&dir, path);
    if (res != FR_OK) {
        return -1; // 无法打开目录
    }

    // 读取目录内容
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK) {
            f_closedir(&dir);
            return -1; // 读取错误
        }

        // 到达目录末尾
        if (fno.fname[0] == 0) {
            f_closedir(&dir);
            // 根目录为空是正常情况
            return 1; // 空目录
        }

        // 跳过 "." 和 ".." 条目
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0) {
            continue;
        }

        // 找到了有效文件或子目录
        f_closedir(&dir);
        return 0; // 非空目录
    }
}


/**
 * 规范化路径（移除末尾多余斜杠）
 */
static void normalize_path(const char *input, char *output, size_t output_size) {
    size_t len = strlen(input);

    if (len >= output_size) {
        output[0] = 0;
        return;
    }

    strcpy(output, input);

    // 移除末尾的斜杠（保留根目录的斜杠）
    while (len > 0 && output[len - 1] == '/') {
        // 保留 "/" 或 "0:/" 格式的根目录斜杠
        if (len == 1) {
            break; // 只剩 "/"
        }
        if (len == 3 && output[1] == ':') {
            break; // "0:/" 格式
        }
        output[len - 1] = 0;
        len--;
    }
}

/**
 * 检查是否为根目录
 */
static int is_root_directory(const char *path) {
    size_t len = strlen(path);

    // "/"
    if (len == 1 && path[0] == '/') {
        return 1;
    }

    // "0:" 或 "0:/"
    if (len >= 2 && path[1] == ':') {
        if (len == 2) {
            return 1; // "0:"
        }
        if (len == 3 && path[2] == '/') {
            return 1; // "0:/"
        }
    }

    return 0;
}

/**
 * 递归删除文件夹及其所有内容
 * @param path 要删除的文件夹路径
 * @return FR_OK 成功，FR_DENIED 如果是根目录，其他值表示失败
 */
FRESULT delete_directory_recursive(const char *path) {
    FRESULT     res;
    DIR         dir;
    FILINFO     fno;
    static char sub_path[512];
    static char normalized_path[256];

    if (path == NULL || strlen(path) == 0) {
        return FR_INVALID_PARAMETER;
    }

    // 规范化路径
    normalize_path(path, normalized_path, sizeof(normalized_path));

    // 禁止删除根目录
    if (is_root_directory(normalized_path)) {
        return FR_DENIED; // 不允许删除根目录
    }

    // 检查路径是否存在
    res = f_stat(normalized_path, &fno);
    if (res != FR_OK) {
        return res; // 路径不存在或其他错误
    }

    // 检查是否为目录
    if (!(fno.fattrib & AM_DIR)) {
        return FR_DENIED; // 不是目录
    }

    // 打开目录
    res = f_opendir(&dir, normalized_path);
    if (res != FR_OK) {
        return res;
    }

    // 遍历目录中的所有项
    while (1) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
            break; // 错误或到达目录末尾
        }

        // 跳过 "." 和 ".."
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0) {
            continue;
        }

        // 构建完整路径
        snprintf(sub_path, sizeof(sub_path), "%s/%s", normalized_path, fno.fname);

        if (fno.fattrib & AM_DIR) {
            // 是文件夹，递归删除
            res = delete_directory_recursive(sub_path);
            if (res != FR_OK) {
                f_closedir(&dir);
                return res;
            }
        }
        else {
            // 是文件，检查是否只读
            if (fno.fattrib & AM_RDO) {
                // 移除只读属性
                f_chmod(sub_path, 0, AM_RDO);
            }

            // 删除文件
            res = f_unlink(sub_path);
            if (res != FR_OK) {
                f_closedir(&dir);
                return res;
            }
        }
    }

    // 关闭目录
    f_closedir(&dir);

    // 删除空目录本身
    res = f_unlink(normalized_path);
    return res;
}

/**
 * 删除空文件夹
 * @param path 要删除的文件夹路径
 * @return FR_OK 成功，FR_DENIED 如果非空或是根目录
 */
FRESULT delete_empty_directory(const char *path) {
    FRESULT res;
    FILINFO fno;
    char    normalized_path[256];

    if (path == NULL || strlen(path) == 0) {
        return FR_INVALID_PARAMETER;
    }

    // 规范化路径
    normalize_path(path, normalized_path, sizeof(normalized_path));

    // 禁止删除根目录
    if (is_root_directory(normalized_path)) {
        return FR_DENIED;
    }

    // 检查路径是否存在
    res = f_stat(normalized_path, &fno);
    if (res != FR_OK) {
        return res;
    }

    // 检查是否为目录
    if (!(fno.fattrib & AM_DIR)) {
        return FR_DENIED;
    }

    // 检查是否为空
    int is_empty = is_directory_empty(normalized_path);
    if (is_empty != 1) {
        return FR_DENIED; // 非空或检查失败
    }

    // 删除空目录
    return f_unlink(normalized_path);
}

/**
 * 安全删除文件夹（带选项）
 * @param path 要删除的文件夹路径
 * @param recursive 是否递归删除内容（1=递归，0=仅删除空文件夹）
 * @return FR_OK 成功，其他值表示失败
 */
FRESULT delete_directory(const char *path, int recursive) {
    if (recursive) {
        return delete_directory_recursive(path);
    }
    else {
        return delete_empty_directory(path);
    }
}