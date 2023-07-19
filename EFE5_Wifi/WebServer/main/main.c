#include <stdio.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "connect.h"
#include "web.h"
#include "interface.h"


const static char *TAG = "WEB_SERVER";
const static char *CONFIG_WIFI_SSID = "UbeeAD6D-2.4G";
const static char *CONFIG_WIFI_PASSWORD = "6F38E6AD6D";


void app_main(void)
{
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
    //ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    wifi_init();
    // specify SSID and Password, 10 sec timeout
    ESP_ERROR_CHECK( wifi_connect_sta( CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, 10000 ));

    // GPIO initialization
    //gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    //ESP_LOGI(TAG, "LED Control Web Server is running ... ...\n");
    setup_server();
}
