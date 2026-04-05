/*
* DI0 is the IO0 of CH422G, which corresponds to 0x01
* DI1 is IO5 for CH422G, corresponding to 0x20
* DO0 is the OC0 of CH422G, corresponding to 0x01
* DO0 is the OC1 of CH422G, corresponding to 0x02
*/
#include "LCD.h"
#include "CH422G.h"

uint8_t io_flag = 1; // IO flag

int app_main(void)
{
    DEV_Module_Init();                         // Initialize the device module
    uint8_t io[2] = {0}, DI_flag = 0, num = 0; // IO array, DI flag, and detection count
    while (1)
    {
        // Output a value to the CH422G GPIO
        CH422G_od_output(0x01);
        DEV_Delay_ms(1);               // After writing IO, the minimum delay is 1ms, otherwise the data cannot be read
        io[0] = CH422G_io_input(0x01); // Read IO pin 0x01
        io[1] = CH422G_io_input(0x20); // Read IO pin 0x20

        // Check if both pins match expected values
        if (io[0] == 0x01 && io[1] == 0x00)
        {
            DI_flag++; // Increment DI flag
        }

        // Output another value to the CH422G GPIO
        CH422G_od_output(0x02);
        DEV_Delay_ms(1);
        io[0] = CH422G_io_input(0x01); // Re-read IO pin 0x01
        io[1] = CH422G_io_input(0x20); // Re-read IO pin 0x20

        // Check again if both pins match expected values
        if (io[0] == 0x00 && io[1] == 0x20)
        {
            DI_flag++; // Increment DI flag
        }

        // If both conditions are met, DI & DO are working
        if (DI_flag >= 2)
        {
            printf("DI & DO OK!!!\r\n"); // DI and DO are functioning properly
            io_flag = 0;                 // Reset IO flag
            break;
        }
        else
        {
            num++;        // Add 1 to the count
            if (num == 3) // If the test fails three times, we quit
            {
                printf("DI & DO Failure!!!\r\n"); // DI and DO are not functioning
                break;
            }
        }
    }

    // Initialize the LCD and get the display object
    lv_disp_t *disp = LCD_init();

    // Run the custom LVGL UI demo
    example_lvgl_demo_ui(disp);

    // Loop to handle LVGL timers
    while (1)
    {
        lv_timer_handler(); // Handle LVGL tasks
        DEV_Delay_ms(10);   // Delay to prevent CPU overload
    }
}
