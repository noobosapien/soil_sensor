#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nvs_flash.h>

// Inner function to open the NVS 
static nvs_handle_t storage_open(nvs_open_mode_t mode){
    esp_err_t err;
    nvs_handle_t handle;

    err = nvs_open("storage", mode, &handle);

    if(err != 0){ // NVS not initialized
        nvs_flash_init();
        err = nvs_open("storage", mode, &handle);
        printf("err1: %d\n", err);
    }

    return handle;
}

// Read from the NVS
void storage_read_string(char* name, char* def, char*dest, int len){
    nvs_handle_t handle = storage_open(NVS_READONLY);
    strncpy(dest, def, len);
    size_t length = len;
    nvs_get_str(handle, name, dest, &length);
    nvs_close(handle);

    printf("Read %s = %s\n", name, dest);
}

// Write to NVS
void storage_write_string(char* name, char* val){
    nvs_handle_t handle = storage_open(NVS_READWRITE);
    nvs_set_str(handle, name, val);
    nvs_commit(handle);
    nvs_close(handle);

    printf("Written %s = %s\n", name, val);
}


