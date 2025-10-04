
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
            dest[pos] = '/';
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
    for (size_t i = n - 1; i >= 0; i--) {
        if (file_name[i] == '.') {
            b = i;
            break;
        }
    }
    strncpy(extension, file_name + b, n - b);
}
