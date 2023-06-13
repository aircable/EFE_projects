idf_component_register(
    SRCS 
        "toggleLed.c" 
        "filesystem.c"
    INCLUDE_DIRS 
    "."
    REQUIRES main driver json fatfs littlefs
)