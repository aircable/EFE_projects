

idf_component_register(
    SRCS 
        "main.c"
    INCLUDE_DIRS 
    "."
    "../components/wifi"
    "../components/settings"
    "../components/iotools"
    REQUIRES driver esp_pm iotools settings wifi

)
