#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

extern void heap( void );
extern void stack( void );
extern void psram( void );

void app_main(void)
{
  heap();
  stack();
  psram();
}
