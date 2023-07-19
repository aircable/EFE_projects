#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "esp_http_client.h"
#include "connect.h"
#include "esp_log.h"
#include "esp_check.h"

const static char *TAG = "WHATSAPP";
const static char *CONFIG_WIFI_SSID = "UbeeAD6D-2.4G";
const static char *CONFIG_WIFI_PASSWORD = "6F38E6AD6D";

void send_whatsapp_message(void);
void wifi_init(void);
esp_err_t wifi_connect_sta(const char *ssid, const char *pass, int timeout);

void app_main(void){
    nvs_flash_init();
    wifi_init();
  // specify SSID and Password, 10 sec timeout
    ESP_ERROR_CHECK( wifi_connect_sta( CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD, 10000 ));

    send_whatsapp_message();
}


void send_whatsapp_message( void ){
    char callmebot_url[] = 
        "https://api.callmebot.com/whatsapp.php?phone=%s&text=%s&apikey=%s";
    char URL[strlen(callmebot_url)];

    //sprintf( URL, callmebot_url, "1234567890", 
    //         url_encode("this is a test"), "xxx" );

    sprintf( URL, callmebot_url, "+11234567890", 
             "TwinkleTwinkle", "xxx" );

    esp_http_client_config_t config = {
        .url = URL,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init( &config );
    esp_err_t err = esp_http_client_perform( client );
    
}