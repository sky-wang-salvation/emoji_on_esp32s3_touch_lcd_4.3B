/*  RTC - Simple example

    RTC simple example, how to initialize I2C, configure PCF85063A
    As well as reading and writing to the register of the sensor connected through I2C and alarm interrupt.
*/

#include "rtc_pcf85063a.h"

static const char *TAG = "RTC";

// Set initial time
static datetime_t Set_Time = {
    .year = 2024,
    .month = 02,
    .day = 02,
    .dotw = 5,
    .hour = 9,
    .min = 0,
    .sec = 0};

// Set alarm time
static datetime_t Set_Alarm_Time = {
    .year = 2024,
    .month = 02,
    .day = 02,
    .dotw = 5,
    .hour = 9,
    .min = 0,
    .sec = 2};

char datetime_str[256]; // String to store formatted date-time
char ip[20]; // Placeholder for IP address

// External interrupt handler function
int Alarm_flag = 0;
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    Alarm_flag = 1; // Set alarm flag when interrupt occurs
}

// Main application function
int app_main(void)
{
    datetime_t Now_time;

    // Initialize PCF85063A
    PCF85063A_Init();

    // Set time
    PCF85063A_Set_All(Set_Time);

    // Set alarm
    PCF85063A_Set_Alarm(Set_Alarm_Time);

    // Start alarm interrupt
    PCF85063A_Enable_Alarm();

    // Initialize the ESP32 interrupt input pin with callback function
    DEV_GPIO_INT(6, gpio_isr_handler);

    while (1)
    {
        // Read current time
        PCF85063A_Read_now(&Now_time);

        // Convert time to string
        datetime_to_str(datetime_str, Now_time);
        ESP_LOGI(TAG, "Now_time is %s", datetime_str);  // Log the current time

        if (Alarm_flag == 1)
        {
            Alarm_flag = 0;  // Reset alarm flag
            // Start the alarm again. If the alarm only needs to run once, comment out the following function
            PCF85063A_Enable_Alarm();
            ESP_LOGI(TAG, "The alarm clock goes off.");
        }

        // Delay 1 second
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
