set(COMPONENT_SRCS "fat.c" "spiffs.c" "nvs.c" "file_in_code.c" "main.c" )
                    
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_EMBED_TXTFILES
    "files/index.html"
    "files/sample.txt"
)

set(COMPONENT_EMBED_FILES
    "files/pinout.jpg"
)

register_component()
                    