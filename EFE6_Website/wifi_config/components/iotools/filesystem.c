#include <stdio.h>
#include <string.h>
#include <sdkconfig.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_vfs.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc_periph.h"
#include "soc/rtc_i2c_reg.h"
#include "driver/rtc_io.h"
#include "esp_log.h"
#include "errno.h"
#include "cJSON.h"
#include "settings.h"
#include "filesystem.h"
#include "extflash.h"

#include "esp_littlefs.h"

#define TAG "FS"


// FILE SYSTEM SFIFFS, LittleFS and FATFS
// must contain / at the end for directory
void
list_files( const char* basedir )  { 
    struct dirent *de;  // Pointer for directory entry 
    struct stat entry_stat;

    ESP_LOGI( TAG, "opening %s", basedir );
    // opendir() returns a pointer of DIR type.  
    DIR *dr = opendir( basedir ); 
  
    if (dr == NULL){  // opendir returns NULL if couldn't open directory 
        ESP_LOGE(TAG, "Could not open current directory" ); 
    } 
  
    // Refer http://pubs.opengroup.org/onlinepubs/7990989775/xsh/readdir.html 
    // for readdir() 
    while((de = readdir(dr)) != NULL) {
        char entrypath[ 2*CONFIG_LITTLEFS_OBJ_NAME_LEN ];

        if( de->d_name[0] == '/' ){
            sprintf( entrypath, "%s%s", basedir, &de->d_name[1] ); // remove beginning '/'
        } else {
            sprintf( entrypath, "%s%s", basedir, de->d_name );
        }
        // create a full path from the entry
        snprintf( entrypath, sizeof(entrypath), "%s/%s", basedir, de->d_name );
        //ESP_LOGI( TAG, "%s", entrypath ); 
        if( stat( entrypath, &entry_stat ) != 0 ) {
            ESP_LOGE( TAG, "Failed to stat %s, errno %d", entrypath, errno );
            // somehow not able to open, ignore
            continue;
        }
        if( S_ISDIR( entry_stat.st_mode )) {
            if (strcmp(".", de->d_name) == 0 || strcmp("..", de->d_name) == 0) {
                continue;
            }
            // recursive
            list_files( entrypath );
        } else {        
            ESP_LOGI( TAG, "Found %s (%ld bytes)", de->d_name, entry_stat.st_size );
        }

    }
    closedir(dr);     
} 


esp_err_t 
filesystem_init( bool reset ) {
    esp_err_t ret;

    // 1: Main file system
    ESP_LOGI(TAG, "Initializing LittelFS");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "",
        .partition_label = "image",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    if(( ret = esp_vfs_littlefs_register( &conf ))!= ESP_OK) {
        if (ret == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount or format LittleFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
                ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
            ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    // debug list all files
    // careful, this is a recursive function, requires BIG stack space in CONFIG_ESP_MAIN_TASK_STACK_SIZE
    ESP_LOGI(TAG, "files:"); list_files( "/" );        

    // 2: mount SDCARD file system
    if( sdcard_init() != ESP_OK ){
        ESP_LOGE( TAG, "no SD card available");
    } 
    return ESP_OK;
}

