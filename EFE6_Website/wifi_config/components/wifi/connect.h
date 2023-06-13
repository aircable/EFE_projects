#ifndef connect_h
#define connect_h

#include "esp_err.h"

void wifi_init(void);
void deinit_wifi( void );
esp_err_t wifi_connect_sta( void );
void wifi_connect_ap( void );
void wifi_disconnect(void);

#endif