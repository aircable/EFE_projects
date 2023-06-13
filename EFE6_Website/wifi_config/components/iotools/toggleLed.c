#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "driver/gpio.h"

#include "sdkconfig.h"
#include "toggleLed.h"

void init_led(void){
    gpio_set_direction( CONFIG_LED_GPIO, GPIO_MODE_OUTPUT );
}

void toggle_led( bool is_on ){
  gpio_set_level( CONFIG_LED_GPIO, is_on );
}