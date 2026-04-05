#include "waveshare_pcf85063a.h"

// Set current time
static datetime_t Set_Time = {
    .year = 2024,
    .month = 02,
    .day = 02,
    .dotw = 5,    // Day of the week, 5 means Friday
    .hour = 9,
    .min = 0,
    .sec = 0
};

// Set alarm time
static datetime_t Set_Alarm_Time = {
    .year = 2024,
    .month = 02,
    .day = 02,
    .dotw = 5,   
    .hour = 9,
    .min = 0,
    .sec = 2     
};

char datetime_str[256]; // Variable to store time string
datetime_t Now_time;    // Current time variable
int Alarm_flag = 0;     // Alarm flag

// The alarm interrupt callback function
void ARDUINO_ISR_ATTR Alarm_INT_callback() {
    Alarm_flag = 1; // Set alarm flag
}

void setup() {
    Serial.begin(115200); // Initialize serial communication

    PCF85063A_Init();     // Initialize PCF85063A RTC module
    PCF85063A_Set_All(Set_Time); // Set current time
    PCF85063A_Set_Alarm(Set_Alarm_Time); // Set alarm time
    PCF85063A_Enable_Alarm(); // Enable the alarm

    pinMode(RTC_MASTER_INT_IO, INPUT_PULLUP); // Set the alarm interrupt pin to a pull-up input
    attachInterrupt(RTC_MASTER_INT_IO, Alarm_INT_callback, FALLING); // Set interrupt, call callback on falling edge
}

void loop() {
    PCF85063A_Read_now(&Now_time); // Read current time
    datetime_to_str(datetime_str, Now_time); // Convert to string format
    Serial.print("Now_time is "); 
    Serial.println(datetime_str); // Print current time
    if (Alarm_flag == 1) { // If alarm flag is set
        Alarm_flag = 0; // Reset alarm flag
        // Restart the alarm. Comment out this function if it only needs to run once
        PCF85063A_Enable_Alarm(); // Re-enable the alarm
        Serial.println("The alarm clock goes off."); // Print alarm goes off message
    }
    delay(1000); // Delay for 1 second
}
