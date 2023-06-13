#include <stdio.h>
#include "connect.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "esp_mac.h"
#include "mdns.h"

static const char *TAG = "SMS";
TaskHandle_t taskHandle;


char api_key[] = "3601822";
char whatsapp_num[] = "14085200502";
char whatsapp_messgae[] = "WhatsApp Message Test!";

char *url_encode(const char *str)
{
    static const char *hex = "0123456789abcdef";
    static char encoded[1024];
    char *p = encoded;
    while (*str)
    {
        if (isalnum(*str) || *str == '-' || *str == '_' || *str == '.' || *str == '~')
        {
            *p++ = *str;
        }
        else
        {
            *p++ = '%';
            *p++ = hex[*str >> 4];
            *p++ = hex[*str & 15];
        }
        str++;
    }
    *p = '\0';
    return encoded;
}
// task
void OnConnected( void *pvParameters ) {
    char callmebot_url[] = "https://api.callmebot.com/whatsapp.php?phone=%s&text=%s&apikey=%s";

    char URL[strlen(callmebot_url)];
    sprintf(URL, callmebot_url, whatsapp_num, url_encode(whatsapp_messgae), api_key);
    ESP_LOGI(TAG, "URL = %s", URL);
    esp_http_client_config_t config = {
        .url = URL,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        int status_code = esp_http_client_get_status_code(client);
        if (status_code == 200)
        {
            ESP_LOGI(TAG, "Message sent Successfully");
        }
        else
        {
            ESP_LOGI(TAG, "Message sent Failed");
        }
    }
    else
    {
        ESP_LOGI(TAG, "Message sent Failed");
    }
    esp_http_client_cleanup(client);

    vTaskDelete(NULL);
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
