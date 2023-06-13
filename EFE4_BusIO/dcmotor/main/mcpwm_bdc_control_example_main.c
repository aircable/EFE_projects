/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "sdkconfig.h"
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"


static const char *TAG = "DC";


// update your hardware
#define BDC_ENABLE              GPIO_NUM_12
#define BDC_MCPWM_GPIO_A        GPIO_NUM_13
#define BDC_MCPWM_GPIO_B        GPIO_NUM_14


#define LEDC_DUTY_50               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_DUTY_20               (1638) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_DUTY_100               (8191) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

static void motor_init(void){
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .freq_hz          = 5000,  // Set output frequency at 5 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BDC_ENABLE,           // PWM output to enable pin
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // bridge output
    gpio_set_direction( BDC_MCPWM_GPIO_A, GPIO_MODE_OUTPUT );
    gpio_set_direction( BDC_MCPWM_GPIO_B, GPIO_MODE_OUTPUT );

}

void motor_direction( bool forward ){
    if( forward ){
        gpio_set_level( BDC_MCPWM_GPIO_A, 1 );
        gpio_set_level( BDC_MCPWM_GPIO_B, 0 );
    } else {
        gpio_set_level( BDC_MCPWM_GPIO_A, 0 );
        gpio_set_level( BDC_MCPWM_GPIO_B, 1 );
    }
}

unsigned int 
duty( unsigned int percent ){
    return( pow( 2, 13 ) - 1 ) * percent / 100 ;
}

void app_main(void)
{
        ESP_LOGI(TAG, "Create DC motor");

    // Set the LEDC peripheral configuration
    motor_init();
    motor_direction( true );

    while( true ){
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty(20)));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay( 1000 / portTICK_PERIOD_MS );

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty(50)));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay( 1000 / portTICK_PERIOD_MS );

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty(100)));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay( 1000 / portTICK_PERIOD_MS );       

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty(0)));
        // Update duty to apply the new value
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
        vTaskDelay( 1000 / portTICK_PERIOD_MS );             
    }
}

