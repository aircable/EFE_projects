#pragma once

#include "esp_err.h"
#include "esp_pm.h"

void list_files( const char* basedir );
esp_err_t filesystem_init( bool reset );
bool test_fatfs( void );
bool create_config_file( char* basedir );
esp_err_t fcopy( const char* to, const char* from );
