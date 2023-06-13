idf_component_register(SRCS "server.c" "connect.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_http_server spiffs json settings esp_wifi mdns )

