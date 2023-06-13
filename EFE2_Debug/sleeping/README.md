# _Sample project_
MENUCONFIG
enable use of server web sockets
set SPIFFS_OBJ_NAME_LEN to 128 
enable CONFIG_HTTPD_WS_SUPPORT

MANAGED COMPONENTS
idf.py add-dependency "espressif/mdns^1.0.9"

REQUIRES
esp_wifi
mdns
