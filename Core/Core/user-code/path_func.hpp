
#pragma once

#include "prj_header.hpp"

bool path_is_absolute(const char *path);
int  path_norm(const char *path, char *dest, char *disk_root = nullptr, char *file_name = nullptr);
int  path_concat(char *lhs, const char *rhs, char *dest);
int  path_get_disk_root(const char *lhs, char *disk_root);
int  path_get_file_name(const char *lhs, char *file_name);
void path_get_file_extension(const char *file_name, char *extension);

#define PATH_IS_ABSOLUTE     1
#define PATH_IS_NOT_COMPLETE 2
#define PATH_HAVE_PARENT     4
#define PATH_HAVE_FILE_NAME  8
