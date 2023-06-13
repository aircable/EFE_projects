#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

extern void file_in_code( void );
extern void nvs( void );
extern void spiffs( void );
extern void fat( void );

void app_main(void)
{
  file_in_code();
  nvs();
  spiffs();
  fat();
}
