#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#define TAG "NVS"

nvs_handle handle;


esp_err_t 
read_data( char* to_read, void* into ){
  esp_err_t result = nvs_get_i32( handle, to_read, into ); // nvs_get_blob
  switch( result ){
  case ESP_ERR_NVS_NOT_FOUND:
  case ESP_ERR_NOT_FOUND:
    ESP_LOGE(TAG, "Value not set yet");
    break;
  case ESP_OK:
    ESP_LOGI(TAG, "Value read ok");
    break;
  default:
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!\n", esp_err_to_name(result));
    break;
  }
  return result;
}

void nvs()
{

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(nvs_open("store", NVS_READWRITE, &handle));

  int32_t val = 0;
  read_data( "val", &val );
  val++;
  ESP_ERROR_CHECK(nvs_set_i32(handle, "val", val));
  ESP_ERROR_CHECK(nvs_commit(handle));

  read_data( "val", &val );
  ESP_LOGI( TAG, "read %ld", val );
  nvs_close(handle);

  vTaskDelay(1000 / portTICK_PERIOD_MS);
}