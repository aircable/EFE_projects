#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "string.h"

#define TAG "UART_Config_Example"

#define RXD_PIN GPIO_NUM_4
#define TXD_PIN GPIO_NUM_5
#define UART_PORT_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_EVENT_QUEUE_SIZE 10

const int RD_UART_BUFSIZE = 1024;
const int WR_UART_BUFSIZE = 1024;
QueueHandle_t uart_queue;

/* Part 1: hardware init */
void uart_hardware_init( QueueHandle_t* uart_queue ){

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        //.flow_ctrl = UART_HW_FLOWCTRL_RTS,
        //.rx_flow_ctrl_thresh = 122,
    #if SOC_UART_SUPPORT_REF_TICK
        .source_clk = UART_SCLK_REF_TICK, // UART_SCLK_APB was unreliable
    #elif SOC_UART_SUPPORT_XTAL_CLK
        .source_clk = UART_SCLK_XTAL,
    #endif
    };

    // buffer size min 129 bytes
    // transmit buffer size of >512 does not make sense because the write_char can only
    // process 30 blocks (hardcoded) of 18 bytes in BLE
    // if installed already, it will say so
    // creates an event queue
    // should set flag ESP_INTR_FLAG_IRAM while CONFIG_UART_ISR_IN_IRAM is set
    uart_driver_install( UART_NUM_1, RD_UART_BUFSIZE, 2*WR_UART_BUFSIZE, UART_EVENT_QUEUE_SIZE, uart_queue, ESP_INTR_FLAG_IRAM );
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    fprintf( stderr, "%s uart %d initialize, baud %d\n", TAG, UART_NUM_1, uart_config.baud_rate );

    uart_param_config( UART_NUM_1, &uart_config );
    // clear data buffers
    uart_flush( UART_NUM_1 );
}

/* Part 3: Uart event handling */
static void rx_task(void *arg) {
    uart_event_t event;
    int received, total;
    uint8_t* temp = (uint8_t *)malloc( RD_UART_BUFSIZE );
    while (1) {
        //Waiting for UART event.
        if( xQueueReceive( uart_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
        
            switch( event.type ) {
                case UART_DATA: {
                    int counter = 0;
                    memset(temp, 0, RD_UART_BUFSIZE);
                    total = received = uart_read_bytes( UART_NUM_1, temp, RD_UART_BUFSIZE, 4 /portTICK_PERIOD_MS );

                    while(( total )&&( received < RD_UART_BUFSIZE )){
                        // read again until nothing left into buffer, 2ms, lower than 9600 baud
                        total = uart_read_bytes( UART_NUM_1, &temp[received], RD_UART_BUFSIZE-received, 4 / portTICK_PERIOD_MS );
                        received += total;
                        counter++;
                    }
                    // send the buffer or do something with it
                    ESP_LOGI(TAG, "size %d, loop %d", received, counter );
                    printf("RX BYTES: %d,\tRX BUFFER: %s\n", received, temp);
                } break;
                case UART_BREAK:
                case UART_DATA_BREAK:
                    ESP_LOGE(TAG, "uart event BREAK (%d)", event.type);
                    break;
                case UART_BUFFER_FULL:
                case UART_FIFO_OVF:
                    // UART_FIFO_OVF UART_BUFFER_FULL
                    ESP_LOGE(TAG, "Fifo overflow or ring buffer full");
                    uart_flush_input( UART_NUM_1 );
                    xQueueReset( uart_queue );
                    break;
                case UART_FRAME_ERR:
                case UART_PARITY_ERR:
                    break;
                default:
                    break;
            }
        }
    }
    ESP_LOGI( TAG, "uart rx task terminated");
    free( temp );
    vTaskDelete(NULL);
}

void app_main(void) 
{
    uart_queue = xQueueCreate(UART_EVENT_QUEUE_SIZE, sizeof(uart_event_t));

    uart_hardware_init(&uart_queue); 

    /* Part 2: Uart task create */
    xTaskCreate( &rx_task, "uart_rx_task", 4096, NULL, 15, NULL );
    //ESP_LOGI( TAG, "uart rx task started");

}