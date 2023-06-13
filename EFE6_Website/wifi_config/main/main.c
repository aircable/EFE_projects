#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "esp_pm.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_app_format.h"
#include "esp_event.h"
#include "driver/uart.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc_periph.h"
#include "driver/rtc_io.h"
#include "soc/soc_ulp.h"
#include "esp_ota_ops.h"

#include "sdkconfig.h"

#include "settings.h"
#include "pushBtn.h"
#include "toggleLed.h"
#include "connect.h"
#include "filesystem.h"
#include "server.h"


static const char *TAG = "MAIN";
#define ESP_INTR_FLAG_DEFAULT 0


// variable must be in RTC to keep value in deep sleep
// declare here and only here to be placed right after ULP code
RTC_DATA_ATTR struct timeval sleep_enter_time; // 4 bytes
RTC_DATA_ATTR uint64_t start_time ;
RTC_DATA_ATTR struct timespec ts_now;

esp_pm_config_t pm_config = {
    .max_freq_mhz = 240, // Wifi min is 80MHz
    .min_freq_mhz = 10, // set to crystal frequency, 40MHz or divided by integer, min 10 MHz
    .light_sleep_enable = false, // deep sleep only
};


// reduce wake-up time use the CONFIG_BOOTLOADER_SKIP_VALIDATE_IN_DEEP_SLEEP
// will be automatically called immediately when wakeup when declared here
// code must be in RTC fast memory, data in RTC slow memory
void RTC_IRAM_ATTR esp_wake_deep_sleep(void) {
    // default wakeup routine, must be called first, or crash
    esp_default_wake_deep_sleep();

    static RTC_RODATA_ATTR const char fmt_str[] = "Wake count %d\n";
    esp_rom_printf( fmt_str, settings.wakeup_counter++ );
    // get time and store to calculate boot and init time
    //start_time = esp_timer_get_time(); << see alternative below
}

// performance and light sleep modes config, requires TICKLESS_IDLE and PM_ENABLE
static void
sleep_init( uint64_t pinmap, bool wakeup_on_high ){
    esp_err_t err;
    esp_log_level_set( "pm", ESP_LOG_ERROR ); // eliminates light sleep change notifications  
    // LIGHT SLEEP MODE CRASHES  
    pm_config.light_sleep_enable = false;
    if(( err = esp_pm_configure( &pm_config )) != ESP_OK ){
       ESP_LOGE( TAG, "power management failed: %s", esp_err_to_name( err ));
    }      
    if( pinmap ){
        // allow deep sleep wakeup with button
        esp_sleep_enable_ext0_wakeup( pinmap, wakeup_on_high ); // false = wakeup on low
        // must then keep GPIO pin on
        esp_sleep_pd_config( ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON );
    }
}

// enter deep sleep in microseconds
void
sleep_enter( uint64_t sleeping ){

    if( sleeping < 100 * 1000 ){
        sleeping = 100 * 1000; // minimum 100ms sleep
    }
    // disable wifi and bluetoth, release the locks
    //deinit_wifi();
    // wait for shutdown of Wifi, necessary!
    vTaskDelay( 500 / portTICK_PERIOD_MS );
    // global in RTC memory
    gettimeofday( &sleep_enter_time, NULL ); // best way to get system time


    ESP_LOGI( TAG, "Entering sleep, %lld msec", sleeping/1000 );
    uart_wait_tx_idle_polling( CONFIG_ESP_CONSOLE_UART_NUM ); // UART flush

    // NOW enter sleep mode
    esp_sleep_enable_timer_wakeup( sleeping );
    // allow ULP to wakeup processor
    esp_sleep_enable_ulp_wakeup();  
    esp_deep_sleep_start();    
    // does not return
}


static bool
wakeup_processor( void ){
        // wake up from sleep or reset
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    struct timeval enter;
    int sleep_time_ms = 0;

    if( wakeup_cause == ESP_SLEEP_WAKEUP_EXT1 ){
        uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();

        if( wakeup_pin_mask != 0 ){
            int pin = __builtin_ffsll(wakeup_pin_mask) - 1;
            ESP_LOGI( TAG,  "Wake up from GPIO %d, level %d", pin, gpio_get_level( pin ));

        } else {
            ESP_LOGI( TAG,  "Wake up from unknown GPIO" );
        }

        // this is startup from interrupt
        gettimeofday(&enter, NULL); // best way to get system time
        start_time = enter.tv_sec;
        
    } else if( wakeup_cause == ESP_SLEEP_WAKEUP_ULP ){ 
        ESP_LOGI( TAG,  "Wake up from ULP" );
        //handle_ulp_wakeup_isr();

    } else if( wakeup_cause == ESP_SLEEP_WAKEUP_TIMER) {
        ESP_LOGI( TAG,  "Wake up from TIMER: %ld", settings.wakeup_counter );

    } else if(( wakeup_cause == ESP_SLEEP_WAKEUP_TOUCHPAD )||( wakeup_cause == ESP_SLEEP_WAKEUP_EXT0 )) {
        ESP_LOGI( TAG,  "Wake up from touch on pad\n" );
        // crashes: ESP_LOGI( TAG, "Wake up from touch on pad %d\n", esp_sleep_get_touchpad_wakeup_status());
    }
    
    if ( wakeup_cause != ESP_SLEEP_WAKEUP_ULP ){
        ESP_LOGI( TAG, "Not ULP wakeup, initializing ULP");
        //init_ulp_program();
    }

    // COCPU trap signal trigger
    if ( wakeup_cause == ESP_SLEEP_WAKEUP_COCPU ){
        // happens when the ULP crashes
        ESP_LOGE( TAG, "Crash, resetting the ULP" );
        //ulp_riscv_reset();
        esp_rom_delay_us( 20 );        
    }    

    if( wakeup_cause != ESP_SLEEP_WAKEUP_UNDEFINED ){
        // was a wakeup from timer or interrupt or touchpad
        gettimeofday( &enter, NULL ); // best way to get system time
        sleep_time_ms = (enter.tv_sec - sleep_enter_time.tv_sec) * 1000 + 
                        (enter.tv_usec - sleep_enter_time.tv_usec) / 1000;
        ESP_LOGI( TAG, "Time spent in deep sleep: %dms", sleep_time_ms);
        return false;

    } else {
        // power up reset
        gettimeofday( &enter, NULL ); // best way to get system time
        start_time = enter.tv_sec;
        ESP_LOGI( TAG,  "Power up reset, time %llds", start_time );
        return true;
    }
}


void app_main( void ){
    struct timeval enter;

    // start up, was it reset or sleep wakeup
    bool reset = wakeup_processor();

    sleep_init( 1ULL << CONFIG_BUTTON_GPIO, false ); // wakeup on button press go low

    // default event loop task and IRQ services
    ESP_ERROR_CHECK( esp_event_loop_create_default());
    ESP_ERROR_CHECK( gpio_install_isr_service( ESP_INTR_FLAG_DEFAULT ));

	// nvm flash and settings
	bool was_read = settings_init( true );  
    // file system
    if( filesystem_init( reset )){
        ESP_LOGE( TAG, "error file system init" );
    }
    // get system RTC time
    gettimeofday( &enter, NULL ); // best way to get system time
    ESP_LOGI( TAG, "running since %llds", enter.tv_sec ); 	
    // init hardware
	init_led();
	init_btn();
    // init wifi
	wifi_init();
    // will wait until we have IP
	wifi_connect_sta(); 
    // MDNS and HTTPD will have started then 

	while( true ){
		// settings update (save every 5 minutes)
		settings_update();
        // in microseconds
        vTaskDelay( 5000 / portTICK_PERIOD_MS );
        // does not return
	}
}
