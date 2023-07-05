#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/temperature_sensor.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"



/* constants, functions and variables for the RTOS tasks */
static const char *TAG_ACQ = "Acquiring process : ";
static const char *TAG_PRT = "Printing process : ";
static const char *TAG_SV = "Storing process : ";

float tsens_value;
void acquire_temperature_task(void * params);
void save_temperature_task(void * params);
void print_temperature_task(void * params);
/* end of RTOS constants, functions and variales*/

/* 
 * Semaphores
 */
SemaphoreHandle_t xSemaphore;
SemaphoreHandle_t xSemaphoreRedyFile;

/* file system constants, functions and variables*/
wl_handle_t fh;
wl_handle_t mount_fat(void);
void write_file(char *path, char *content);
void read_file(char *path, char *buffer, int len);
void unmount_fat(wl_handle_t wl_handle);
/* end of file system constants, functions and variables*/

void app_main(void)
{
    //create a mutex for temperature acquisition
    xSemaphore = xSemaphoreCreateMutex();
    //create a mutex for file access
    xSemaphoreRedyFile = xSemaphoreCreateMutex();
    fh = mount_fat();
    xTaskCreate(&acquire_temperature_task, "acquiring temperature", 4096, "acquire_temperature task", 2, NULL);
    xTaskCreate(&save_temperature_task, "storing temperature", 4096, "storing_temperature task", 2, NULL);
    xTaskCreate(&print_temperature_task, "printing temperature", 4096, "printing_temperature task", 2, NULL);

}


void acquire_temperature_task(void * params){

   
   ESP_LOGI(TAG_ACQ, "Install temperature sensor, expected temp ranger range: 10~50 ℃");
    temperature_sensor_handle_t temp_sensor = NULL;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 50);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));

    ESP_LOGI(TAG_ACQ, "Enable temperature sensor");
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));

    int cnt = 2;
    while (cnt--) {
        if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){    
            ESP_LOGI(TAG_ACQ, "Acquiring temperature");
            ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_sensor, &tsens_value));        
            //vTaskDelay(pdMS_TO_TICKS(1000));
            //release semaphore
            xSemaphoreGive( xSemaphore );
        }
    }

    // Disable the temperature sensor if it's not needed and save the power
    ESP_ERROR_CHECK(temperature_sensor_disable(temp_sensor));  
    vTaskDelete( NULL );
}

void save_temperature_task(void * params){ 
    char buffer[25];    
    int cnt = 2;

    if( xSemaphoreTake( xSemaphoreRedyFile, ( TickType_t ) 10 ) == pdTRUE ){    
        //for file printing
    }

    while (cnt--) {
        if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE ){    
            ESP_LOGI(TAG_SV, "Temperature value %.02f ℃", tsens_value);
            int ret = snprintf(buffer, sizeof buffer, "read %d : %f", cnt, tsens_value);   
            write_file("/store/text.txt", buffer);
            //release semaphore
            xSemaphoreGive( xSemaphore );
        } 
    }

    xSemaphoreGive(xSemaphoreRedyFile);

    vTaskDelete( NULL );
}

void print_temperature_task(void * params){
    char buffer[200];
    if( xSemaphoreTake( xSemaphoreRedyFile, ( TickType_t ) 1000 ) == pdTRUE ){ 
        read_file("/store/text.txt",buffer,200);        
        xSemaphoreGive(xSemaphoreRedyFile);
    }

    ESP_LOGI(TAG_PRT, "End of printing task");
    unmount_fat(fh);
    vTaskDelete( NULL );
} 