#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_random.h"

#include "sdkconfig.h"
#include "settings.h"

static const char *TAG =    "SETUP";
#define STORAGE_NAMESPACE   "storage"
#define STORED_BLOB         "settings"

// we can write to NVS every 5 minutes
#define FIVE_MINUTES    (60*5)

// menuconfig set program version to 16 bit HEX string
#define SETTINGS_MAGIC_NUMBER (CONFIG_APP_PROJECT_VER)

// default values, keep in deep sleep
settings_t RTC_DATA_ATTR settings;

// semaphone writing into settings
static SemaphoreHandle_t settings_semaphore = NULL;

#pragma GCC diagnostic ignored "-Wcast-qual"



static void settings_default( void ){
    uint8_t mac[6];

    settings.magic_number = (int)strtol( SETTINGS_MAGIC_NUMBER, NULL, 16);

    settings.current_time = 0LL; // boot set to zero
    settings.saved_time = 0LL; // last time settings saved

    strcpy( settings.time_zone, "PST8PDT" ); //"America/Los_Angeles" );
    strcpy( settings.ssid, CONFIG_WIFI_SSID );
    strcpy( settings.password, CONFIG_WIFI_PASSWORD ); 


    //  product ID, version and unique
    esp_read_mac( mac, ESP_MAC_WIFI_STA ); // read the wifi MAC address
    // mesh node ID
    settings.node_id = (( mac[4] << 8 ) | mac[5] ) & 0x7FFF;
    // name of device for Wifi and Bluetooth
    sprintf( settings.ap_ssid, "%s_%04X", "ESP32S3", settings.node_id );
}


esp_err_t settings_write(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    struct timeval now; // 4 bytes

    // stops here if not given before
    xSemaphoreTake( settings_semaphore, 0 );

    gettimeofday(&now, NULL);
    settings.saved_time = now.tv_sec;

    ESP_ERROR_CHECK( nvs_open( STORAGE_NAMESPACE, NVS_READWRITE, &my_handle ));
    err = nvs_set_blob( my_handle, STORED_BLOB, &settings, sizeof( settings_t ) );
    if( err ){
        ESP_LOGE( TAG, "creating %d NVS %s", sizeof( settings_t ), esp_err_to_name( err ));
    }
    // done writing
    xSemaphoreGive( settings_semaphore );

    return( err );
}



static bool
settings_read(void) {
    bool was_read = false;
    esp_err_t err = ESP_OK;
    nvs_handle_t my_handle;
    size_t get_size = sizeof( settings ); // must be initialized

    ESP_ERROR_CHECK( nvs_open( STORAGE_NAMESPACE, NVS_READWRITE, &my_handle ));
    err = nvs_get_blob(my_handle, STORED_BLOB, &settings, &get_size );
    if( err ){
        ESP_LOGE( TAG, "reading %d NVS %s", get_size, esp_err_to_name( err ));
    }

    // getting data somehow failed
    if(( settings.magic_number != SETTINGS_MAGIC_NUMBER )||( get_size != sizeof( settings_t ) )||( err )){
        ESP_LOGI( TAG, "create default settings entries, magic 0x%x", settings.magic_number );
        settings_default();
        settings_print();
        err = settings_write();
        was_read = false;
    } else {
        ESP_LOGI( TAG, "read magic 0x%x, size %d, requested %d", settings.magic_number, get_size, sizeof( settings_t ) );
        settings_print();
        was_read = true; // successful read
    }
    nvs_close(my_handle);

    return was_read;
}


void
settings_factory_reset( void ){

    settings_default();
    settings_write();
    vTaskDelay( 1000 / portTICK_PERIOD_MS );

    esp_restart();
    // does not return
}



//
void settings_print( void ){
    uint8_t mac[6];
    esp_app_desc_t running_app_info;
    const esp_partition_t *running = esp_ota_get_running_partition();;
    // get firmware version
    esp_ota_get_partition_description(running, &running_app_info);

    ESP_LOGI( TAG, "%s: %s version %s, date %s", TAG, running_app_info.project_name, running_app_info.version, running_app_info.date );
    //  product ID, version and unique
    ESP_LOGI( TAG, "  Device Name: \"%s\"", settings.ap_ssid);

    if( strlen( settings.ssid ) > 3 ){
        ESP_LOGI( TAG, "  SSID \"%s\", Password \"%s\"", settings.ssid, settings.password );
    }

    ESP_LOGI( TAG, "  Current time %lld, ", settings.current_time );
    ESP_LOGI( TAG, "timezone: %s", settings.time_zone );

}



// init writes defaults the first time and reads the stored values
// coming from deep sleep, don't read from NVS, it's still in RTC memory
// returns false if settings got initialized
// returns true if settings are read from NVS correctly
bool settings_init( bool reset )
{
    esp_err_t err;
    bool was_read = false;

    // semaphores don't survive deep sleep
    settings_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive( settings_semaphore );

    // volatiles are still preserved after deep sleep

    err = nvs_flash_init();
    if( err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
        printf("New NVS initialization\n");
        // new flash entry
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
        // open and write default values
        settings_default();
        settings_write();
        was_read = false;
    } else {
        if( reset ) {
            ESP_LOGI(TAG, "reading settings from NVS");
            // set default values, if required
            was_read = settings_read();  
        } else {
             //printf("settings still in RTC memory\n");           
        }      
    }

    return was_read;
}


// we can write every 5 minutes for a 120 year life of the FLASH
esp_err_t settings_update( void ){
    struct timeval now; // 4 bytes
    esp_err_t err = ESP_OK;

    settings.loop_counter++;

    gettimeofday(&now, NULL);
    settings.current_time = now.tv_sec;

    // write settings every 5 minutes
    if(( now.tv_sec - settings.saved_time ) > FIVE_MINUTES ){
        err = settings_write();
        //ESP_LOGI( stderr, "%s: Wrote settings to NVM", TAG );
        //settings_print();
    }
    return err;
}
