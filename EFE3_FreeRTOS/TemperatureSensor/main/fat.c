#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"


static const char *TAG = "FAT : ";
static const char *BASE_PATH = "/store";

void write_file(char *path, char *content);
void read_file(char *path, char *buffer, int len);

wl_handle_t mount_fat(void){
    wl_handle_t wl_handle;

    esp_vfs_fat_mount_config_t esp_vfs_fat_mount_config = {
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_vfs_fat_spiflash_mount_rw_wl(BASE_PATH, "storage", &esp_vfs_fat_mount_config, &wl_handle);

    return wl_handle;
    
}

void unmount_fat(wl_handle_t wl_handle){
    esp_vfs_fat_spiflash_unmount_rw_wl(BASE_PATH, wl_handle);
}

void read_file(char *path, char *buffer, int len){
    ESP_LOGI(TAG, "reading file %s", path);
    FILE *file = fopen(path, "r");
    while( fgets(buffer, len, file) != NULL ){
        ESP_LOGI(TAG, "file contains: %s", buffer);
    }
    fclose(file);
    
}

void write_file(char *path, char *content)
{
    ESP_LOGI(TAG, "Writing \"%s\" to %s", content, path);
    FILE *file = fopen(path, "a");
    fputs(content, file);
    fclose(file);
}