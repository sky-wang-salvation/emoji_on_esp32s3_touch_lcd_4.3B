#ifndef __TWAI_PORT_H // Header guard to prevent multiple inclusions 
#define __TWAI_PORT_H

#include <stdio.h>             // Standard input/output library 
#include <stdlib.h>            // Standard library for memory allocation, etc. 
#include "freertos/FreeRTOS.h" // FreeRTOS header 
#include "freertos/task.h"     // FreeRTOS task management 
#include "freertos/queue.h"    // FreeRTOS queue management 
#include "freertos/semphr.h"   // FreeRTOS semaphore management 
#include "esp_err.h"           // ESP-IDF error codes 
#include "esp_log.h"           // ESP-IDF logging library 
#include "driver/twai.h"       // TWAI driver header 
#include <esp_timer.h>         // ESP timer library 

#include "driver/i2c.h"  // I2C driver header 
#include "driver/gpio.h" // GPIO driver header 

#define GPIO_OUTPUT_PIN_SEL ((1ULL << CONFIG_EXAMPLE_TX_GPIO_NUM) | (1ULL << CONFIG_EXAMPLE_RX_GPIO_NUM)) // GPIO output pin selection 

/* --------------------- Definitions and static variables ------------------ */
// Example Configuration
#define TX_GPIO_NUM CONFIG_EXAMPLE_TX_GPIO_NUM // Transmit GPIO number 
#define RX_GPIO_NUM CONFIG_EXAMPLE_RX_GPIO_NUM // Receive GPIO number 
#define EXAMPLE_TAG "TWAI Master"              // Log tag for example 

// Intervals:
#define TRANSMIT_RATE_MS 1000 // Transmission interval in milliseconds 
#define POLLING_RATE_MS 1000  // Polling interval in milliseconds 

esp_err_t waveshare_twai_init();    // Function to initialize TWAI 
esp_err_t waveshare_twai_receive(); // Function to receive TWAI messages 

#endif // End of header guard 
