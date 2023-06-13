#include <stdio.h>
#include "connect.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "mdns.h"

static const char *TAG = "WIFI";
TaskHandle_t taskHandle;

// task
void OnConnected( void *para ) {
  while (true){
    vTaskDelay( 5000 / portTICK_PERIOD_MS );
  }
}

void start_mdns_service(){
  uint8_t mac[6];
  char HOST[11];

  mdns_init();
  // name your own device my-esp32.local
  esp_read_mac( mac, ESP_MAC_WIFI_STA ); // read the wifi MAC address
  unsigned int node_id = (( mac[4] << 8 ) | mac[5] ) & 0x7FFF;
  sprintf( HOST, "%s_%04X", "esp32", node_id );
  mdns_hostname_set( HOST ); 
  mdns_instance_name_set("EFE_course");
  ESP_LOGI( TAG, "hostname: %s.local", HOST );
}

void app_main(void){
  ESP_ERROR_CHECK(nvs_flash_init());

  wifi_init();
  // specify SSID and Password, 10 sec timeout
  ESP_ERROR_CHECK( wifi_connect_sta( CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, 10000 ));
  start_mdns_service();
  // you get there when we are connected
  xTaskCreate( OnConnected, "handel comms", 1024 * 5, NULL, 5, &taskHandle );

}
