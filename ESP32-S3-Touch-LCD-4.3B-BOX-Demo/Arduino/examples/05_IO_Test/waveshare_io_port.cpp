#include "waveshare_io_port.h"

uint8_t num = 0; // Variable to count events

using namespace esp_panel::drivers;
esp_expander::CH422G *expander = NULL;

static LCD *create_lcd_without_config(void)
{
    BusRGB *bus = new BusRGB(
#if EXAMPLE_LCD_RGB_DATA_WIDTH == 8
        /* 8-bit RGB IOs */
        EXAMPLE_LCD_RGB_IO_DATA0, EXAMPLE_LCD_RGB_IO_DATA1, EXAMPLE_LCD_RGB_IO_DATA2, EXAMPLE_LCD_RGB_IO_DATA3,
        EXAMPLE_LCD_RGB_IO_DATA4, EXAMPLE_LCD_RGB_IO_DATA5, EXAMPLE_LCD_RGB_IO_DATA6, EXAMPLE_LCD_RGB_IO_DATA7,
        EXAMPLE_LCD_RGB_IO_HSYNC, EXAMPLE_LCD_RGB_IO_VSYNC, EXAMPLE_LCD_RGB_IO_PCLK, EXAMPLE_LCD_RGB_IO_DE,
        EXAMPLE_LCD_RGB_IO_DISP,
        /* RGB timings */
        EXAMPLE_LCD_RGB_TIMING_FREQ_HZ, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT,
        EXAMPLE_LCD_RGB_TIMING_HPW, EXAMPLE_LCD_RGB_TIMING_HBP, EXAMPLE_LCD_RGB_TIMING_HFP,
        EXAMPLE_LCD_RGB_TIMING_VPW, EXAMPLE_LCD_RGB_TIMING_VBP, EXAMPLE_LCD_RGB_TIMING_VFP
#elif EXAMPLE_LCD_RGB_DATA_WIDTH == 16
        /* 16-bit RGB IOs */
        EXAMPLE_LCD_RGB_IO_DATA0, EXAMPLE_LCD_RGB_IO_DATA1, EXAMPLE_LCD_RGB_IO_DATA2, EXAMPLE_LCD_RGB_IO_DATA3,
        EXAMPLE_LCD_RGB_IO_DATA4, EXAMPLE_LCD_RGB_IO_DATA5, EXAMPLE_LCD_RGB_IO_DATA6, EXAMPLE_LCD_RGB_IO_DATA7,
        EXAMPLE_LCD_RGB_IO_DATA8, EXAMPLE_LCD_RGB_IO_DATA9, EXAMPLE_LCD_RGB_IO_DATA10, EXAMPLE_LCD_RGB_IO_DATA11,
        EXAMPLE_LCD_RGB_IO_DATA12, EXAMPLE_LCD_RGB_IO_DATA13, EXAMPLE_LCD_RGB_IO_DATA14, EXAMPLE_LCD_RGB_IO_DATA15,
        EXAMPLE_LCD_RGB_IO_HSYNC, EXAMPLE_LCD_RGB_IO_VSYNC, EXAMPLE_LCD_RGB_IO_PCLK, EXAMPLE_LCD_RGB_IO_DE,
        EXAMPLE_LCD_RGB_IO_DISP,
        /* RGB timings */
        EXAMPLE_LCD_RGB_TIMING_FREQ_HZ, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT,
        EXAMPLE_LCD_RGB_TIMING_HPW, EXAMPLE_LCD_RGB_TIMING_HBP, EXAMPLE_LCD_RGB_TIMING_HFP,
        EXAMPLE_LCD_RGB_TIMING_VPW, EXAMPLE_LCD_RGB_TIMING_VBP, EXAMPLE_LCD_RGB_TIMING_VFP
#endif
    );

    /**
     * Take `ST7262` as an example, the following is the actual code after macro expansion:
     *      LCD_ST7262(bus, 24, -1);
     */
    return new EXAMPLE_LCD_CLASS(
        EXAMPLE_LCD_NAME, bus, EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, EXAMPLE_LCD_COLOR_BITS, EXAMPLE_LCD_RST_IO
    );
}

static LCD *create_lcd_with_config(void)
{
    BusRGB::Config bus_config = {
        .refresh_panel = BusRGB::RefreshPanelPartialConfig{
            .pclk_hz = EXAMPLE_LCD_RGB_TIMING_FREQ_HZ,
            .h_res = EXAMPLE_LCD_WIDTH,
            .v_res = EXAMPLE_LCD_HEIGHT,
            .hsync_pulse_width = EXAMPLE_LCD_RGB_TIMING_HPW,
            .hsync_back_porch = EXAMPLE_LCD_RGB_TIMING_HBP,
            .hsync_front_porch = EXAMPLE_LCD_RGB_TIMING_HFP,
            .vsync_pulse_width = EXAMPLE_LCD_RGB_TIMING_VPW,
            .vsync_back_porch = EXAMPLE_LCD_RGB_TIMING_VBP,
            .vsync_front_porch = EXAMPLE_LCD_RGB_TIMING_VFP,
            .data_width = EXAMPLE_LCD_RGB_DATA_WIDTH,
            .bits_per_pixel = EXAMPLE_LCD_RGB_COLOR_BITS,
            .bounce_buffer_size_px = EXAMPLE_LCD_RGB_BOUNCE_BUFFER_SIZE,
            .hsync_gpio_num = EXAMPLE_LCD_RGB_IO_HSYNC,
            .vsync_gpio_num = EXAMPLE_LCD_RGB_IO_VSYNC,
            .de_gpio_num = EXAMPLE_LCD_RGB_IO_DE,
            .pclk_gpio_num = EXAMPLE_LCD_RGB_IO_PCLK,
            .disp_gpio_num = EXAMPLE_LCD_RGB_IO_DISP,
            .data_gpio_nums = {
                EXAMPLE_LCD_RGB_IO_DATA0, EXAMPLE_LCD_RGB_IO_DATA1, EXAMPLE_LCD_RGB_IO_DATA2, EXAMPLE_LCD_RGB_IO_DATA3,
                EXAMPLE_LCD_RGB_IO_DATA4, EXAMPLE_LCD_RGB_IO_DATA5, EXAMPLE_LCD_RGB_IO_DATA6, EXAMPLE_LCD_RGB_IO_DATA7,
#if EXAMPLE_LCD_RGB_DATA_WIDTH > 8
                EXAMPLE_LCD_RGB_IO_DATA8, EXAMPLE_LCD_RGB_IO_DATA9, EXAMPLE_LCD_RGB_IO_DATA10, EXAMPLE_LCD_RGB_IO_DATA11,
                EXAMPLE_LCD_RGB_IO_DATA12, EXAMPLE_LCD_RGB_IO_DATA13, EXAMPLE_LCD_RGB_IO_DATA14, EXAMPLE_LCD_RGB_IO_DATA15,
#endif
            },
        },
    };
    LCD::Config lcd_config = {
        .device = LCD::DevicePartialConfig{
            .reset_gpio_num = EXAMPLE_LCD_RST_IO,
            .bits_per_pixel = EXAMPLE_LCD_COLOR_BITS,
        },
        .vendor = LCD::VendorPartialConfig{
            .hor_res = EXAMPLE_LCD_WIDTH,
            .ver_res = EXAMPLE_LCD_HEIGHT,
        },
    };

    /**
     * Take `ST7262` as an example, the following is the actual code after macro expansion:
     *      LCD_ST7262(bus_config, lcd_config);
     */
    return new EXAMPLE_LCD_CLASS(EXAMPLE_LCD_NAME, bus_config, lcd_config);
}

#if EXAMPLE_LCD_ENABLE_PRINT_FPS

DRAM_ATTR int frame_count = 0;
DRAM_ATTR int fps = 0;
DRAM_ATTR long start_time = 0;

IRAM_ATTR bool onLCD_RefreshFinishCallback(void *user_data)
{
    if (start_time == 0) {
        start_time = millis();

        return false;
    }

    frame_count++;
    if (frame_count >= EXAMPLE_LCD_PRINT_FPS_COUNT_MAX) {
        fps = EXAMPLE_LCD_PRINT_FPS_COUNT_MAX * 1000 / (millis() - start_time);
        esp_rom_printf("LCD FPS: %d\n", fps);
        frame_count = 0;
        start_time = millis();
    }

    return false;
}
#endif // EXAMPLE_LCD_ENABLE_PRINT_FPS

#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
IRAM_ATTR bool onLCD_DrawFinishCallback(void *user_data)
{
    esp_rom_printf("LCD draw finish callback\n");

    return false;
}
#endif

// Function to test IO functionality
void waveshare_io_test(void)
{
    Serial.println("Initialize IO expander"); // Print initialization message
    /* Initialize IO expander */
    expander = new esp_expander::CH422G(EXAMPLE_I2C_SCL_PIN, EXAMPLE_I2C_SDA_PIN, EXAMPLE_I2C_ADDR);
    // Create an instance of the IO expander
    expander->init(); // Initialize the expander
    expander->begin(); // Begin expander operation

    Serial.println("Set the OC pin to push-pull output mode.");
    expander->enableOC_PushPull();

    Serial.println("Set the IO0-7 pin to input mode.");
    expander->enableAllIO_Input();

    // Set output pins to default high
    expander->digitalWrite(DO0, 1); // Set DO0 high
    expander->digitalWrite(DO1, 1); // Set DO1 high

    Serial.println("IO test example start"); // Print start message
    while(1)
    {
        // Control output pins
        expander->digitalWrite(DO0, 1); // Set DO0 high
        expander->digitalWrite(DO1, 0); // Set DO1 low
        delay(1); // Delay for 1 millisecond
        if (expander->multiDigitalRead(DI0_mask | DI1_mask) == 0x01) // Check input state
            num++; // Increment num if DI0 is detected

        expander->digitalWrite(DO0, 0); // Set DO0 low
        expander->digitalWrite(DO1, 1); // Set DO1 high
        delay(1); // Delay for 1 millisecond
        if (expander->multiDigitalRead(DI0_mask | DI1_mask) == 0x20) // Check input state
            num++; // Increment num if DI1 is detected
        
        // Break the loop if num reaches 2
        if(num == 2)
            break; // Exit the loop
        else
            num = 3; // Reset num
        break; // Exit the loop
    }
}

bool colorFill(auto lcd, uint16_t width, uint16_t height, uint16_t color)
{
  int bits_per_piexl = lcd->getFrameColorBits();
  if(bits_per_piexl < 0)
    printf("Invalid color bits\r\n");

  int bytes_per_piexl = bits_per_piexl / 8;
  uint8_t *single_bar_buf = NULL;

  /* Malloc memory for a single color bar */
  try {
      single_bar_buf = new uint8_t[height * width * bytes_per_piexl];
  } catch (std::bad_alloc &e) {
      printf("Malloc color buffer failed\r\n");

  }

  bool ret = true;

    /* Draws full-screen colors */
    for (int j = 0; j < bits_per_piexl; j++) {
        for (int i = 0; i < height * width; i++) {
            for (int k = 0; k < bytes_per_piexl; k++) {
                single_bar_buf[i * bytes_per_piexl + k] = (color >> (k * 8)) & 0xff;
            }  
        }
        ret = lcd->drawBitmap(0, 0, width, height, single_bar_buf);
        if (ret != true) {
            ESP_LOGE(TAG, "Draw bitmap failed");
            goto end;
        }
    }

end:
    delete[] single_bar_buf;
    return ret;
}

// Function to initialize the LCD
void waveshare_lcd_init(void)
{
#if EXAMPLE_LCD_ENABLE_CREATE_WITH_CONFIG
    Serial.println("Initializing \"RGB\" LCD with config");
    auto lcd = create_lcd_with_config();
#else
    Serial.println("Initializing \"RGB\" LCD without config");
    auto lcd = create_lcd_without_config();
#endif

    // Configure bounce buffer to avoid screen drift
    auto bus = static_cast<BusRGB *>(lcd->getBus());
    bus->configRGB_BounceBufferSize(EXAMPLE_LCD_RGB_BOUNCE_BUFFER_SIZE); // Set bounce buffer to avoid screen drift

    lcd->init();
#if EXAMPLE_LCD_ENABLE_PRINT_FPS
    // Attach a callback function which will be called when the Vsync signal is detected
    lcd->attachRefreshFinishCallback(onLCD_RefreshFinishCallback);
#endif
#if EXAMPLE_LCD_ENABLE_DRAW_FINISH_CALLBACK
    // Attach a callback function which will be called when every bitmap drawing is completed
    lcd->attachDrawBitmapFinishCallback(onLCD_DrawFinishCallback);
#endif
    lcd->reset();
    assert(lcd->begin());
    if (lcd->getBasicAttributes().basic_bus_spec.isFunctionValid(LCD::BasicBusSpecification::FUNC_DISPLAY_ON_OFF)) {
        lcd->setDisplayOnOff(true);
    }
    Serial.println("Draws full-screen colors"); // Print message
    // Fill the LCD with a color based on the num variable
    if(num == 2)
        colorFill(lcd,EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, GREEN); // Fill with green
    else
        colorFill(lcd,EXAMPLE_LCD_WIDTH, EXAMPLE_LCD_HEIGHT, RED); // Fill with red
}
