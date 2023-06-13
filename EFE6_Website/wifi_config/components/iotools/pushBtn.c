#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "cJSON.h"
#include "app.h"

#include "sdkconfig.h"
#include "pushBtn.h"

const static char* TAG = "BTN";

static QueueHandle_t interruptQueue = NULL;

static void IRAM_ATTR on_btn_pushed(void *args){
  int pinNumber = (int)args;
  esp_rom_printf( "btn irq %d\n", gpio_get_level( pinNumber ));
  xQueueSendFromISR( interruptQueue, &pinNumber, NULL );
}

static void btn_push_task(void *params){

  int pinNumber, count = 0;

  while( true ){
    // stops here if not given before
    if( xQueueReceive(interruptQueue, &pinNumber, portMAX_DELAY )){
      // disable the interrupt
      gpio_isr_handler_remove(pinNumber);
      // wait some time while we check for the button to be released
      do{
          vTaskDelay( 20 / portTICK_PERIOD_MS );
      } while( gpio_get_level(pinNumber) == 1 );

      printf("GPIO %d was pressed %d times. The state is %d\n", pinNumber, count++, gpio_get_level( pinNumber ));

      cJSON *payload = cJSON_CreateObject(); // careful: cJson allocs memory
      cJSON_AddBoolToObject( payload, "btn_state", gpio_get_level( CONFIG_BUTTON_GPIO ));
      char *message = cJSON_Print(payload); // allocs memory

      // websocket message
      send_ws_message( message );

      // clean up after json
      cJSON_Delete( payload );
      free( message );

      // re-enable the interrupt
      gpio_isr_handler_add( pinNumber, on_btn_pushed, (void *)pinNumber );
    }
  }
  vTaskDelete( NULL );
}

void init_btn(void){

  gpio_set_direction( CONFIG_BUTTON_GPIO, GPIO_MODE_INPUT );
  gpio_set_pull_mode( CONFIG_BUTTON_GPIO, GPIO_PULLUP_ENABLE );
  gpio_set_pull_mode( CONFIG_BUTTON_GPIO, GPIO_PULLDOWN_DISABLE );
  gpio_set_intr_type( CONFIG_BUTTON_GPIO, GPIO_INTR_NEGEDGE );
  
  interruptQueue = xQueueCreate(10, sizeof(int));
  xTaskCreate( btn_push_task, "btn_push_task", 2048, NULL, 5, NULL );
  // install the interrupt handlers for the pins
  gpio_isr_handler_add( CONFIG_BUTTON_GPIO, on_btn_pushed, (void*)CONFIG_BUTTON_GPIO );
}