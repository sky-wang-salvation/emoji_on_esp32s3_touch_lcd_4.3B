#ifndef __TWAI_PORT_H // Header guard to prevent multiple inclusions 

#include <stdio.h>             // Standard I/O library 
#include <stdlib.h>            // Standard library for memory allocation, etc. 
#include "freertos/FreeRTOS.h" // FreeRTOS core header 
#include "freertos/task.h"     // FreeRTOS task management header 
#include "freertos/queue.h"    // FreeRTOS queue management header 
#include "freertos/semphr.h"   // FreeRTOS semaphore management header 
#include "esp_err.h"           // ESP-IDF error handling header 
#include "esp_log.h"           // ESP-IDF logging header 
#include "driver/twai.h"       // TWAI driver header 
#include <esp_timer.h>         // ESP timer header 

#include "driver/i2c.h"  // I2C driver header 
#include "driver/gpio.h" // GPIO driver header 

/* --------------------- Definitions and static variables ------------------ */
// Example Configuration 
#define TX_GPIO_NUM CONFIG_EXAMPLE_TX_GPIO_NUM // Transmit GPIO pin number 
#define RX_GPIO_NUM CONFIG_EXAMPLE_RX_GPIO_NUM // Receive GPIO pin number 
#define EXAMPLE_TAG "TWAI Master"              // Log tag for TWAI master 

// Interval definitions 
#define TRANSMIT_RATE_MS 1000 // Message transmit rate in milliseconds 
#define POLLING_RATE_MS 1000  // Polling rate in milliseconds 

// Function prototypes 
esp_err_t waveshare_twai_init();     // Initialize TWAI 
esp_err_t waveshare_twai_transmit(); // Transmit data via TWAI 

#endif // End of header guard 
