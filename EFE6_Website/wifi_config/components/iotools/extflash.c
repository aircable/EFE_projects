#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h> 

#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"

#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "extflash.h"

static const char *TAG = "SD";



static sdmmc_card_t* card = NULL; // not initialized
static sdmmc_host_t host = SDSPI_HOST_DEFAULT();
static const char mount_point[] = "/sdcard";

esp_err_t
sdcard_init(void) {

    esp_err_t ret;
    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    // sector size always 512
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,                     // max number of open files
        .allocation_unit_size = 512   // format the card with this, 512 - 128*512, save space over speed
    };
    
    ESP_LOGI(TAG, "init SD card SPI mode");

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize( host.slot, &bus_cfg, SPI_DMA_CH_AUTO );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount( mount_point, &host, &slot_config, &mount_config, &card );

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.\n"
                "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).\n"
                "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return ret;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info( stderr, card );

    return ret;
}


void
sdcard_deinit(void) {

    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdcard_unmount( mount_point, card );
    ESP_LOGI(TAG, "Card unmounted");
    //deinitialize the bus after all devices are removed
    spi_bus_free( host.slot );
}

// return size in bytes
uint64_t sdcard_size( void ){
    if( card != NULL ){
        return ((uint64_t) card->csd.capacity) * card->csd.sector_size;
    } else {
        return 0;
    }
}
