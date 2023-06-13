
#pragma once


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "esp_err.h"

// Pin mapping when using SPI mode. This is for ESP32S3 Freenove
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO GPIO_NUM_40
#define PIN_NUM_MOSI GPIO_NUM_38
#define PIN_NUM_CLK  GPIO_NUM_39
#define PIN_NUM_CS   GPIO_NUM_NC

extern esp_err_t sdcard_init(void);
extern void sdcard_deinit(void);
extern uint64_t sdcard_size( void );