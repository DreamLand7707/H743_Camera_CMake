
#include "path_func.hpp"
#include <cctype>

bool path_is_absolute(const char *path) {
    size_t n = strlen(path);
    if (n < 3)
        return false;
    if (isalnum(path[0]) && path[1] == ':' && path[2] == '/')
        return true;
    return false;
}

int path_norm(char *path) {
    size_t n = strlen(path);
    if (!n)
        return 0;

    int enum_state = 0;
    /*
        0: Normal Path, concat as is
        1: Disk Number, must be first element
        2: Current Path, delete it
        3: Parent Path, move behind
        4: Unknown Element, return false
    */
     

}

int path_concat(char *lhs, const char *rhs) {
}
