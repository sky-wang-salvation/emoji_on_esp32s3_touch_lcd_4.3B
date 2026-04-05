/*****************************************************************************
* | File      	:   LCD.c
* | Author      :
* | Function    :   LCD Display
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-02-03
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/

#include "LCD.h"

// Define the callback function for the next scene task
static void scene_next_task_cb(lv_timer_t *timer);

static int32_t scene_act = -1; // Variable to track the current scene
static lv_obj_t *scr;          // Pointer to the screen object

// We use two semaphores to sync the VSYNC event and the LVGL task, to avoid potential tearing effect
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
SemaphoreHandle_t sem_vsync_end; // Semaphore to signal the end of VSYNC
SemaphoreHandle_t sem_gui_ready; // Semaphore to signal the GUI is ready
#endif

void example_lvgl_demo_ui(lv_disp_t *disp)
{
    scr = lv_disp_get_scr_act(disp);                     // Get the active screen from the display
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0); // Set the screen background to white
    scene_act = 1;                                       // Initialize the scene to 1
    scene_next_task_cb(NULL);                            // Trigger the next scene task
}

// VSYNC event handler
static bool example_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    // Synchronize VSYNC and GUI ready events
    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE)
    {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken); // Signal the end of VSYNC
    }
#endif
    return high_task_awoken == pdTRUE; // Return if a higher priority task was awoken
}

// LVGL flush callback function
static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1; // Set the X-axis start position
    int offsetx2 = area->x2; // Set the X-axis end position
    int offsety1 = area->y1; // Set the Y-axis start position
    int offsety2 = area->y2; // Set the Y-axis end position
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    xSemaphoreGive(sem_gui_ready);                // Signal GUI is ready
    xSemaphoreTake(sem_vsync_end, portMAX_DELAY); // Wait for the VSYNC signal
#endif
    // Pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv); // Signal that flushing is done
}

// Timer callback to increase LVGL tick
static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds have elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

lv_disp_t *LCD_init()
{
    static lv_disp_draw_buf_t disp_buf; // Contains internal graphic buffer(s)
    static lv_disp_drv_t disp_drv;      // Contains callback functions
    static lv_disp_t *disp;             // Display object
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    sem_vsync_end = xSemaphoreCreateBinary(); // Create a semaphore for VSYNC
    assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary(); // Create a semaphore for GUI ready
    assert(sem_gui_ready);
#endif

    // Install RGB LCD panel driver
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit width
        .psram_trans_align = 64,
        .num_fbs = EXAMPLE_LCD_NUM_FB, // Number of frame buffers
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * EXAMPLE_LCD_H_RES, // Bounce buffer size
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,           // Clock source
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN, // Display enable pin
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,    // Pixel clock pin
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,  // VSYNC pin
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,  // HSYNC pin
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,        // Data enable pin
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ, // Pixel clock frequency
            .h_res = EXAMPLE_LCD_H_RES,            // Horizontal resolution
            .v_res = EXAMPLE_LCD_V_RES,            // Vertical resolution
            // The following parameters should refer to LCD spec
#if ESP_PANEL_USE_1024_600_LCD
            .hsync_back_porch = 188,
            .hsync_front_porch = 44,
            .hsync_pulse_width = 88,
            .vsync_back_porch = 16,
            .vsync_front_porch = 3,
            .vsync_pulse_width = 6,
#else
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .hsync_pulse_width = 4,
            .vsync_back_porch = 16,
            .vsync_front_porch = 16,
            .vsync_pulse_width = 4,
#endif

            .flags.pclk_active_neg = true, // Negative pixel clock

        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
    };
    // Initialize RGB LCD panel
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    // Register event callbacks
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = example_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));

    // Initialize the LCD panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Initialize the LVGL library
    lv_init();

    void *buf1 = NULL;
    void *buf2 = NULL;

#if CONFIG_EXAMPLE_DOUBLE_FB
    // Use frame buffers as LVGL draw buffers
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
    // Initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES);
#else
    // Allocate separate LVGL draw buffers from PSRAM
    buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 100 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 100 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf2);
    // Initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 100);
#endif

    // Register display driver to LVGL
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;      // Horizontal resolution
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;      // Vertical resolution
    disp_drv.flush_cb = example_lvgl_flush_cb; // Flush callback
    disp_drv.draw_buf = &disp_buf;             // Draw buffer 
    disp_drv.user_data = panel_handle;         // User data 

#if CONFIG_EXAMPLE_DOUBLE_FB
    disp_drv.full_refresh = true; // Full refresh mode 
#endif

    // Register the display driver 
    disp = lv_disp_drv_register(&disp_drv);

    // Install LVGL tick timer 
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"};
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    return disp;
}

#define SCENE_TIME 1000 /*ms*/
extern uint8_t io_flag;

void scene_next_task_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    // Clear the screen object 
    lv_obj_clean(scr);
    // Set the background color based on io_flag 
    if (io_flag == 0)
        lv_obj_set_style_bg_color(scr, lv_palette_darken(LV_PALETTE_GREEN, 1), 0); // Green background 
    else
        lv_obj_set_style_bg_color(scr, lv_palette_darken(LV_PALETTE_RED, 4), 0); // Red background 
}
