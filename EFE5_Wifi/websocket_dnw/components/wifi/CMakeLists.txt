idf_component_register( SRCS "connect.c" "server.c"
                    INCLUDE_DIRS "." "../iotools"
                    REQUIRES esp_wifi esp_http_server mdns spiffs )
