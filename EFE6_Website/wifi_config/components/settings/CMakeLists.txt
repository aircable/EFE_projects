idf_component_register(SRCS 
                    "settings.c"
                    INCLUDE_DIRS "." 
		REQUIRES app_update esp_hw_support nvs_flash main )

