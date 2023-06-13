#pragma once


#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "time.h"
#include "esp_err.h"
#include "esp_wifi_types.h"

#include "sdkconfig.h"

typedef struct {
    uint16_t magic_number;
    uint32_t wakeup_counter;
    uint32_t loop_counter;

    time_t  current_time; // seconds after 1970, saved every hour and restored in reset
    time_t  saved_time;          // last time settings was saved
    char    time_zone[32];  // official time zone string

    // wifi station settings, ssid is 32 bytes long
    char    ssid[32];       // station ssid
    char    password[64];   // station password

    uint16_t node_id;
    char    ap_ssid[32];    // access point ssid and name of device incl. node_id

} __attribute__((packed)) settings_t; // careful, if not alligned printf can't print strings (timezone)

// global settings
extern settings_t  settings;
bool settings_init( bool reset );
esp_err_t settings_write(void);
esp_err_t settings_update( void );
void settings_print( void );

void settings_factory_reset( void );

