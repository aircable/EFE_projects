#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sdkconfig.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "connect.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"

#include "mdns.h"
#include "server.h"
#include "filesystem.h"
#include "toggleLed.h"

static const char *TAG = "WIFI";
TaskHandle_t taskHandle;


#define INDEX_HTML_PATH "/index.html"
char index_html[4096];

static void initi_web_page_buffer(void){

    memset((void *)index_html, 0, sizeof(index_html));
    struct stat st;
    if (stat(INDEX_HTML_PATH, &st)){
        ESP_LOGE(TAG, "index.html not found");
        return;
    }
    FILE *fp = fopen(INDEX_HTML_PATH, "r");
    if(fread( index_html, st.st_size, 1, fp ) == 0){
        ESP_LOGE(TAG, "fread failed");
    }
    fclose(fp);
}

// task
void OnConnected( void *para ) {

  ESP_LOGI(TAG, "ESP32 ESP-IDF WebSocket Web Server is running ... ...\n");
  setup_websocket_server();

  while (true){
    vTaskDelay( 5000 / portTICK_PERIOD_MS );
  }
  vTaskDelete( NULL );
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

  // init hardware
	init_led();  
  // file system
  if( filesystem_init( true )){
      ESP_LOGE( TAG, "error file system init" );
  }
  // read single page into buffer
  initi_web_page_buffer();
  
  wifi_init();
  // specify SSID and Password, 10 sec timeout
  ESP_ERROR_CHECK( wifi_connect_sta( CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, 10000 ));
  start_mdns_service();
  // you get there when we are connected
  xTaskCreate( OnConnected, "handel comms", 1024 * 5, NULL, 5, &taskHandle );

}
