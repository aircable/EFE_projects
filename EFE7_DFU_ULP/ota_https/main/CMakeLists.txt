
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)

idf_component_register(
    SRCS
        "main.c"
    INCLUDE_DIRS
        "." "$ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include"
    EMBED_TXTFILES
        "../cert/ca_cert.pem"
    REQUIRES
        driver esp_http_client esp_https_ota esp_netif esp_wifi nvs_flash app_update
)

