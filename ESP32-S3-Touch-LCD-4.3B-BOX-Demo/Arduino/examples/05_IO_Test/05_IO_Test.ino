#include "waveshare_io_port.h"

void setup()
{
    Serial.begin(115200); // Initialize serial communication
    waveshare_io_test();   // Perform IO test
    waveshare_lcd_init();  // Initialize LCD
    Serial.println("IO test example end"); // Print end of IO test
}

void loop()
{
    delay(1000); // Delay for 1 second
#if EXAMPLE_ENABLE_PRINT_LCD_FPS
    Serial.println("RGB refresh rate: " + String(fps)); // Print RGB refresh rate
#else
    Serial.println("IDLE loop"); // Print idle loop message
#endif
}
