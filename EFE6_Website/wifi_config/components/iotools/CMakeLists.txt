idf_component_register(
    SRCS 
        "pushBtn.c" 
        "toggleLed.c" 
        "extflash.c"
        "filesystem.c"
    INCLUDE_DIRS 
    "."
    REQUIRES main driver json fatfs littlefs
)