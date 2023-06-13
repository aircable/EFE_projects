# _Sample project_
MENUCONFIG
enable use of server web sockets
set SPIFFS_OBJ_NAME_LEN to 128 
enable CONFIG_HTTPD_WS_SUPPORT

MANAGED COMPONENTS
idf.py add-dependency "espressif/mdns^1.0.9"
idf.py add-dependency "joltwallet/littlefs^1.5.3"

REQUIRES
esp_wifi
mdns
