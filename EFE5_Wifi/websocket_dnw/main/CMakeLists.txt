

idf_component_register(
    SRCS 
        "main.c"
    INCLUDE_DIRS 
    "."
    "../components/wifi"
    "../components/iotools"
    REQUIRES driver esp_pm wifi nvs_flash 
)
