#ifndef STORAGE_H
#define STORAGE_H

#include <nvs_flash.h>

nvs_handle_t storage_open(nvs_open_mode_t mode);
void storage_read_string(char* name, char* def, char*dest, int len);
void storage_write_string(char* name, char* val);

#endif
