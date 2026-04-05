#include "include/display_service.h"

#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_lvgl_port.h"
#include "drivers/touch/port/esp_lcd_touch.h"
#include "drivers/touch/port/esp_lcd_touch_gt911.h"
#include "lvgl.h"

static const char *TAG = "display_service";

#define DISPLAY_H_RES                   800
#define DISPLAY_V_RES                   480
#define DISPLAY_PIXEL_CLOCK_HZ          (16 * 1000 * 1000)
#define DISPLAY_RGB_BOUNCE_LINES        10
#define DISPLAY_RGB_FB_NUMS             2
#define DISPLAY_I2C_PORT                I2C_NUM_0
#define DISPLAY_I2C_SDA                 GPIO_NUM_8
#define DISPLAY_I2C_SCL                 GPIO_NUM_9
#define DISPLAY_I2C_FREQ_HZ             400000
#define DISPLAY_I2C_TIMEOUT_MS          1000
#define DISPLAY_IO_EXPANDER_ADDR        0x24
#define DISPLAY_BACKLIGHT_ADDR          0x38
#define DISPLAY_BACKLIGHT_ENABLE_REG    0x01
#define DISPLAY_BACKLIGHT_ON_REG        0x1E
#define DISPLAY_TOUCH_RESET_LOW_REG     0x2C
#define DISPLAY_TOUCH_RESET_HIGH_REG    0x2E
#define DISPLAY_TOUCH_INT_GPIO          GPIO_NUM_4
#define DISPLAY_IDLE_SLEEP_US           (180LL * 1000LL * 1000LL)
#define DISPLAY_SHY_HOLD_US             (2500LL * 1000LL)
#define DISPLAY_EXCITED_HOLD_US         (2200LL * 1000LL)
#define DISPLAY_TOUCH_LONG_PRESS_US     (1500LL * 1000LL)
#define DISPLAY_GAZE_MIN_INTERVAL_US    (3LL * 1000LL * 1000LL)
#define DISPLAY_GAZE_MAX_INTERVAL_US    (6LL * 1000LL * 1000LL)
#define DISPLAY_GAZE_DURATION_US        (1200LL * 1000LL)
#define DISPLAY_BLINK_MIN_INTERVAL_US   (10LL * 1000LL * 1000LL)
#define DISPLAY_BLINK_MAX_INTERVAL_US   (20LL * 1000LL * 1000LL)
#define DISPLAY_BLINK_SINGLE_US         (180LL * 1000LL)
#define DISPLAY_BLINK_GAP_US            (90LL * 1000LL)
#define DISPLAY_POSE_LERP_DIV           4
#define DISPLAY_FACE_SHIFT_PX           30
#define DISPLAY_FACE_SHIFT_STEP_PX      6
#define DISPLAY_EYE_LEFT_CENTER_X       230
#define DISPLAY_EYE_RIGHT_CENTER_X      570
#define DISPLAY_EYE_BASE_CENTER_Y       210
#define DISPLAY_EYE_BASE_W              120
#define DISPLAY_EYE_BASE_H              160
#define DISPLAY_EYE_RADIUS              52

#define DISPLAY_GPIO_VSYNC              GPIO_NUM_3
#define DISPLAY_GPIO_HSYNC              GPIO_NUM_46
#define DISPLAY_GPIO_DE                 GPIO_NUM_5
#define DISPLAY_GPIO_PCLK               GPIO_NUM_7
#define DISPLAY_GPIO_DISP               GPIO_NUM_NC
#define DISPLAY_GPIO_DATA0              GPIO_NUM_14
#define DISPLAY_GPIO_DATA1              GPIO_NUM_38
#define DISPLAY_GPIO_DATA2              GPIO_NUM_18
#define DISPLAY_GPIO_DATA3              GPIO_NUM_17
#define DISPLAY_GPIO_DATA4              GPIO_NUM_10
#define DISPLAY_GPIO_DATA5              GPIO_NUM_39
#define DISPLAY_GPIO_DATA6              GPIO_NUM_0
#define DISPLAY_GPIO_DATA7              GPIO_NUM_45
#define DISPLAY_GPIO_DATA8              GPIO_NUM_48
#define DISPLAY_GPIO_DATA9              GPIO_NUM_47
#define DISPLAY_GPIO_DATA10             GPIO_NUM_21
#define DISPLAY_GPIO_DATA11             GPIO_NUM_1
#define DISPLAY_GPIO_DATA12             GPIO_NUM_2
#define DISPLAY_GPIO_DATA13             GPIO_NUM_42
#define DISPLAY_GPIO_DATA14             GPIO_NUM_41
#define DISPLAY_GPIO_DATA15             GPIO_NUM_40

static esp_lcd_panel_handle_t s_panel_handle;
static esp_lcd_panel_io_handle_t s_touch_io_handle;
static esp_lcd_touch_handle_t s_touch_handle;
static lv_display_t *s_display;
static lv_obj_t *s_face_root;
static lv_obj_t *s_left_eye;
static lv_obj_t *s_right_eye;
static lv_obj_t *s_left_cheek;
static lv_obj_t *s_right_cheek;
static lv_obj_t *s_mouth_line;
static lv_obj_t *s_mouth_dot;
static lv_obj_t *s_sleep_label;
static lv_obj_t *s_state_label;
static lv_obj_t *s_hint_label;
static lv_obj_t *s_touch_label;
static bool s_display_ready;
static bool s_i2c_ready;
static bool s_touch_ready;
static esp_lcd_touch_io_gt911_config_t s_touch_gt911_driver_cfg = {
    .dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
};
static int64_t s_last_interaction_us;
static int64_t s_touch_press_started_us;
static int64_t s_touch_face_expire_us;
static int64_t s_auto_face_expire_us;
static int64_t s_next_gaze_us;
static int64_t s_next_blink_us;
static int64_t s_blink_started_us;
static int64_t s_blink_total_us;
static bool s_touch_pressed;
static bool s_long_press_triggered;

typedef enum {
    DISPLAY_TOUCH_ZONE_NONE = 0,
    DISPLAY_TOUCH_ZONE_TOP_LEFT,
    DISPLAY_TOUCH_ZONE_TOP_MID,
    DISPLAY_TOUCH_ZONE_TOP_RIGHT,
    DISPLAY_TOUCH_ZONE_MID_LEFT,
    DISPLAY_TOUCH_ZONE_MID_MID,
    DISPLAY_TOUCH_ZONE_MID_RIGHT,
    DISPLAY_TOUCH_ZONE_BOTTOM_LEFT,
    DISPLAY_TOUCH_ZONE_BOTTOM_MID,
    DISPLAY_TOUCH_ZONE_BOTTOM_RIGHT,
} display_touch_zone_t;

typedef enum {
    DISPLAY_FACE_NONE = -1,
    DISPLAY_FACE_HAPPY = 0,
    DISPLAY_FACE_SHY,
    DISPLAY_FACE_EXCITED,
    DISPLAY_FACE_LISTENING,
    DISPLAY_FACE_PROCESSING,
    DISPLAY_FACE_SPEAKING,
    DISPLAY_FACE_LOOK_LEFT,
    DISPLAY_FACE_LOOK_RIGHT,
    DISPLAY_FACE_BLINK,
    DISPLAY_FACE_SLEEP,
    DISPLAY_FACE_ERROR,
} display_face_t;

typedef enum {
    DISPLAY_MOUTH_NONE = 0,
    DISPLAY_MOUTH_SMILE,
    DISPLAY_MOUTH_WAVE,
    DISPLAY_MOUTH_BLINK,
    DISPLAY_MOUTH_FLAT,
    DISPLAY_MOUTH_FROWN,
} display_mouth_t;

typedef enum {
    DISPLAY_BLINK_PATTERN_BOTH = 0,
    DISPLAY_BLINK_PATTERN_LEFT,
    DISPLAY_BLINK_PATTERN_RIGHT,
} display_blink_pattern_t;

typedef struct {
    int32_t eye_center_y;
    int32_t eye_width;
    int32_t left_eye_height;
    int32_t right_eye_height;
    int32_t face_offset_x;
    int32_t cheek_opa;
    int32_t mouth_dot_width;
    int32_t mouth_dot_height;
    int32_t mouth_dot_opa;
    int32_t sleep_opa;
    display_mouth_t mouth;
    lv_color_t eye_color;
} display_expression_t;

static display_face_t s_touch_face = DISPLAY_FACE_NONE;
static display_face_t s_auto_face = DISPLAY_FACE_NONE;
static display_touch_zone_t s_touch_active_zone = DISPLAY_TOUCH_ZONE_NONE;
static display_blink_pattern_t s_blink_pattern = DISPLAY_BLINK_PATTERN_BOTH;
static uint8_t s_blink_repeat_count = 0;
static bool s_expression_ready;
static display_expression_t s_current_expression;
static display_face_t s_last_status_face = DISPLAY_FACE_NONE;
static robot_state_t s_last_status_state = ROBOT_STATE_BOOTING;

static lv_point_t s_runtime_mouth_points[5];


static const lv_point_t s_smile_points[] = {
    {345, 334},
    {425, 354},
    {455, 334},
};

static const lv_point_t s_shy_points[] = {
    {332, 338},
    {372, 326},
    {400, 340},
    {428, 352},
    {468, 340},
};

static const lv_point_t s_error_points[] = {
    {352, 348},
    {448, 348},
};

static const lv_point_t s_blink_points[] = {
    {355, 342},
    {398, 356},
    {442, 338},
    {458, 332},
};

static void display_service_style_rect(lv_obj_t *obj,
                                       lv_coord_t w,
                                       lv_coord_t h,
                                       lv_coord_t radius,
                                       lv_color_t color,
                                       lv_opa_t opa);
static void display_service_start_touch_face_locked(display_face_t face, int64_t now, int64_t hold_us);
static void display_service_clear_touch_face_locked(void);
static void display_service_trigger_blink_locked(int64_t now,
                                                 display_blink_pattern_t pattern,
                                                 uint8_t repeat_count);
static display_touch_zone_t display_service_touch_zone_from_point(uint16_t x, uint16_t y)
{
    uint16_t col = (x >= ((DISPLAY_H_RES * 2U) / 3U)) ? 2U : ((x >= (DISPLAY_H_RES / 3U)) ? 1U : 0U);
    uint16_t row = (y >= ((DISPLAY_V_RES * 2U) / 3U)) ? 2U : ((y >= (DISPLAY_V_RES / 3U)) ? 1U : 0U);

    return (display_touch_zone_t)(1U + row * 3U + col);
}

static const char *display_service_touch_zone_name(display_touch_zone_t zone)
{
    switch (zone) {
    case DISPLAY_TOUCH_ZONE_TOP_LEFT:
        return "top-left";
    case DISPLAY_TOUCH_ZONE_TOP_MID:
        return "top-mid";
    case DISPLAY_TOUCH_ZONE_TOP_RIGHT:
        return "top-right";
    case DISPLAY_TOUCH_ZONE_MID_LEFT:
        return "mid-left";
    case DISPLAY_TOUCH_ZONE_MID_MID:
        return "mid-mid";
    case DISPLAY_TOUCH_ZONE_MID_RIGHT:
        return "mid-right";
    case DISPLAY_TOUCH_ZONE_BOTTOM_LEFT:
        return "bottom-left";
    case DISPLAY_TOUCH_ZONE_BOTTOM_MID:
        return "bottom-mid";
    case DISPLAY_TOUCH_ZONE_BOTTOM_RIGHT:
        return "bottom-right";
    case DISPLAY_TOUCH_ZONE_NONE:
    default:
        return "none";
    }
}

static const char *display_service_face_name(display_face_t face)
{
    switch (face) {
    case DISPLAY_FACE_NONE:
        return "none";
    case DISPLAY_FACE_HAPPY:
        return "happy";
    case DISPLAY_FACE_SHY:
        return "shy";
    case DISPLAY_FACE_EXCITED:
        return "excited";
    case DISPLAY_FACE_LISTENING:
        return "listening";
    case DISPLAY_FACE_PROCESSING:
        return "processing";
    case DISPLAY_FACE_SPEAKING:
        return "speaking";
    case DISPLAY_FACE_LOOK_LEFT:
        return "look-left";
    case DISPLAY_FACE_LOOK_RIGHT:
        return "look-right";
    case DISPLAY_FACE_BLINK:
        return "blink";
    case DISPLAY_FACE_SLEEP:
        return "sleep";
    case DISPLAY_FACE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static int64_t display_service_random_range_us(int64_t min_us, int64_t max_us)
{
    if (max_us <= min_us) {
        return min_us;
    }
    return min_us + ((int64_t)(esp_random() % (uint32_t)(max_us - min_us)));
}

static int32_t display_service_step_value(int32_t current, int32_t target, int32_t divisor)
{
    int32_t delta = target - current;

    if ((delta == 0) || (divisor <= 1)) {
        return target;
    }

    int32_t step = delta / divisor;
    if (step == 0) {
        step = (delta > 0) ? 1 : -1;
    }

    return current + step;
}

static int32_t display_service_step_value_limited(int32_t current, int32_t target, int32_t max_step)
{
    int32_t delta = target - current;

    if ((delta == 0) || (max_step <= 0)) {
        return target;
    }
    if (delta > max_step) {
        return current + max_step;
    }
    if (delta < -max_step) {
        return current - max_step;
    }
    return target;
}

static void display_service_apply_touch_zone_locked(display_touch_zone_t zone, int64_t now)
{
    s_touch_active_zone = zone;

    switch (zone) {
    case DISPLAY_TOUCH_ZONE_TOP_LEFT:
        display_service_start_touch_face_locked(DISPLAY_FACE_LOOK_LEFT, now, 900LL * 1000LL);
        break;
    case DISPLAY_TOUCH_ZONE_TOP_MID:
        display_service_start_touch_face_locked(DISPLAY_FACE_SHY, now, DISPLAY_SHY_HOLD_US);
        break;
    case DISPLAY_TOUCH_ZONE_TOP_RIGHT:
        display_service_start_touch_face_locked(DISPLAY_FACE_LOOK_RIGHT, now, 900LL * 1000LL);
        break;
    case DISPLAY_TOUCH_ZONE_MID_LEFT:
        display_service_start_touch_face_locked(DISPLAY_FACE_LISTENING, now, 1500LL * 1000LL);
        break;
    case DISPLAY_TOUCH_ZONE_MID_MID:
        display_service_clear_touch_face_locked();
        break;
    case DISPLAY_TOUCH_ZONE_MID_RIGHT:
        display_service_start_touch_face_locked(DISPLAY_FACE_PROCESSING, now, 1500LL * 1000LL);
        break;
    case DISPLAY_TOUCH_ZONE_BOTTOM_LEFT:
        display_service_start_touch_face_locked(DISPLAY_FACE_BLINK, now, 520LL * 1000LL);
        display_service_trigger_blink_locked(now, DISPLAY_BLINK_PATTERN_BOTH, 1);
        break;
    case DISPLAY_TOUCH_ZONE_BOTTOM_MID:
        display_service_start_touch_face_locked(DISPLAY_FACE_SPEAKING, now, 1500LL * 1000LL);
        break;
    case DISPLAY_TOUCH_ZONE_BOTTOM_RIGHT:
        display_service_start_touch_face_locked(DISPLAY_FACE_SLEEP, now, 1700LL * 1000LL);
        break;
    case DISPLAY_TOUCH_ZONE_NONE:
    default:
        break;
    }
}

static void display_service_schedule_gaze_locked(int64_t now)
{
    s_next_gaze_us = now + display_service_random_range_us(DISPLAY_GAZE_MIN_INTERVAL_US,
                                                           DISPLAY_GAZE_MAX_INTERVAL_US);
}

static void display_service_schedule_blink_locked(int64_t now)
{
    s_next_blink_us = now + display_service_random_range_us(DISPLAY_BLINK_MIN_INTERVAL_US,
                                                            DISPLAY_BLINK_MAX_INTERVAL_US);
}

static void display_service_stop_blink_locked(void)
{
    s_blink_started_us = 0;
    s_blink_total_us = 0;
    s_blink_repeat_count = 0;
}

static void display_service_clear_auto_face_locked(void)
{
    s_auto_face = DISPLAY_FACE_NONE;
    s_auto_face_expire_us = 0;
}

static void display_service_start_touch_face_locked(display_face_t face, int64_t now, int64_t hold_us)
{
    if (face == DISPLAY_FACE_NONE) {
        s_touch_face = DISPLAY_FACE_NONE;
        s_touch_face_expire_us = 0;
    } else {
        s_touch_face = face;
        s_touch_face_expire_us = (hold_us > 0) ? (now + hold_us) : 0;
    }

    display_service_clear_auto_face_locked();
    display_service_stop_blink_locked();
    display_service_schedule_gaze_locked(now);
    display_service_schedule_blink_locked(now);
}

static void display_service_clear_touch_face_locked(void)
{
    s_touch_face = DISPLAY_FACE_NONE;
    s_touch_face_expire_us = 0;
}

static void display_service_trigger_blink_locked(int64_t now,
                                                 display_blink_pattern_t pattern,
                                                 uint8_t repeat_count)
{
    if (repeat_count == 0) {
        repeat_count = 1;
    }

    s_blink_pattern = pattern;
    s_blink_repeat_count = repeat_count;
    s_blink_started_us = now;
    s_blink_total_us = ((int64_t)repeat_count * DISPLAY_BLINK_SINGLE_US) +
                       ((int64_t)(repeat_count - 1U) * DISPLAY_BLINK_GAP_US);
}

static bool display_service_is_blink_active_locked(int64_t now)
{
    if ((s_blink_started_us <= 0) || (s_blink_total_us <= 0)) {
        return false;
    }

    if ((now - s_blink_started_us) >= s_blink_total_us) {
        display_service_stop_blink_locked();
        return false;
    }

    return true;
}

static int32_t display_service_blink_openness_locked(int64_t now, bool left_eye)
{
    if (!display_service_is_blink_active_locked(now)) {
        return 1000;
    }

    if ((s_blink_pattern == DISPLAY_BLINK_PATTERN_LEFT) && !left_eye) {
        return 1000;
    }
    if ((s_blink_pattern == DISPLAY_BLINK_PATTERN_RIGHT) && left_eye) {
        return 1000;
    }

    int64_t elapsed = now - s_blink_started_us;
    for (uint8_t i = 0; i < s_blink_repeat_count; ++i) {
        int64_t blink_start = (int64_t)i * (DISPLAY_BLINK_SINGLE_US + DISPLAY_BLINK_GAP_US);
        int64_t blink_end = blink_start + DISPLAY_BLINK_SINGLE_US;

        if ((elapsed < blink_start) || (elapsed >= blink_end)) {
            continue;
        }

        int32_t progress = (int32_t)(((elapsed - blink_start) * 1000LL) / DISPLAY_BLINK_SINGLE_US);
        if (progress < 360) {
            return lv_map(progress, 0, 360, 1000, 18);
        }
        if (progress < 470) {
            return 18;
        }
        return lv_map(progress, 470, 1000, 18, 1000);
    }

    return 1000;
}

static display_face_t display_service_face_from_robot_state(robot_state_t state)
{
    switch (state) {
    case ROBOT_STATE_LISTENING:
        return DISPLAY_FACE_LISTENING;
    case ROBOT_STATE_PROCESSING:
        return DISPLAY_FACE_PROCESSING;
    case ROBOT_STATE_SPEAKING:
        return DISPLAY_FACE_SPEAKING;
    case ROBOT_STATE_SLEEP:
        return DISPLAY_FACE_SLEEP;
    case ROBOT_STATE_ERROR:
        return DISPLAY_FACE_ERROR;
    case ROBOT_STATE_HAPPY:
    case ROBOT_STATE_IDLE:
    case ROBOT_STATE_BOOTING:
    default:
        return DISPLAY_FACE_HAPPY;
    }
}

static bool display_service_state_supports_idle_animations(robot_state_t state)
{
    return (state == ROBOT_STATE_BOOTING) ||
           (state == ROBOT_STATE_IDLE) ||
           (state == ROBOT_STATE_HAPPY);
}

static display_face_t display_service_visual_face_from_state(robot_state_t state, int64_t now)
{
    if (state == ROBOT_STATE_ERROR) {
        return DISPLAY_FACE_ERROR;
    }
    if (state == ROBOT_STATE_SLEEP) {
        return DISPLAY_FACE_SLEEP;
    }

    if ((s_last_interaction_us > 0) && ((now - s_last_interaction_us) >= DISPLAY_IDLE_SLEEP_US)) {
        return DISPLAY_FACE_SLEEP;
    }

    if ((s_touch_face != DISPLAY_FACE_NONE) && (s_touch_face_expire_us > 0) && (now >= s_touch_face_expire_us)) {
        display_service_clear_touch_face_locked();
    }
    if (s_touch_face != DISPLAY_FACE_NONE) {
        return s_touch_face;
    }

    if ((s_auto_face != DISPLAY_FACE_NONE) && (s_auto_face_expire_us > 0) && (now >= s_auto_face_expire_us)) {
        display_service_clear_auto_face_locked();
    }

    if (!display_service_state_supports_idle_animations(state)) {
        display_service_clear_auto_face_locked();
        display_service_stop_blink_locked();
        return display_service_face_from_robot_state(state);
    }

    if (!s_touch_pressed) {
        if ((s_next_blink_us > 0) && (now >= s_next_blink_us)) {
            uint32_t pick = esp_random() % 10U;
            display_blink_pattern_t pattern = DISPLAY_BLINK_PATTERN_BOTH;
            uint8_t repeat_count = ((esp_random() % 5U) == 0U) ? 2U : 1U;

            if (pick == 0U) {
                pattern = DISPLAY_BLINK_PATTERN_LEFT;
            } else if (pick == 1U) {
                pattern = DISPLAY_BLINK_PATTERN_RIGHT;
            }

            display_service_trigger_blink_locked(now, pattern, repeat_count);
            display_service_schedule_blink_locked(now);
        }

        if ((s_auto_face == DISPLAY_FACE_NONE) && (s_next_gaze_us > 0) && (now >= s_next_gaze_us)) {
            s_auto_face = ((esp_random() & 1U) == 0U) ? DISPLAY_FACE_LOOK_LEFT : DISPLAY_FACE_LOOK_RIGHT;
            s_auto_face_expire_us = now + DISPLAY_GAZE_DURATION_US;
            display_service_schedule_gaze_locked(now);
        }
    }

    if (s_auto_face != DISPLAY_FACE_NONE) {
        return s_auto_face;
    }

    return display_service_face_from_robot_state(state);
}

static void display_service_style_rect(lv_obj_t *obj, lv_coord_t w, lv_coord_t h, lv_coord_t radius, lv_color_t color, lv_opa_t opa)
{
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, opa, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
}

static void display_service_style_mouth_dot(lv_obj_t *obj,
                                            lv_coord_t x,
                                            lv_coord_t y,
                                            lv_coord_t w,
                                            lv_coord_t h,
                                            lv_color_t color,
                                            lv_opa_t opa)
{
    if (opa == LV_OPA_TRANSP) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    display_service_style_rect(obj, w, h, h / 2, color, opa);
    lv_obj_set_pos(obj, x, y);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void display_service_style_eye(lv_obj_t *obj,
                                      lv_coord_t x,
                                      lv_coord_t y,
                                      lv_coord_t w,
                                      lv_coord_t h,
                                      lv_color_t color,
                                      lv_opa_t opa)
{
    if (opa == LV_OPA_TRANSP) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    display_service_style_rect(obj,
                               w,
                               h,
                               LV_MIN(DISPLAY_EYE_RADIUS, LV_MIN(w / 2, h / 2)),
                               color,
                               opa);
    lv_obj_set_pos(obj, x, y);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void display_service_style_cheek(lv_obj_t *obj, lv_coord_t x, lv_coord_t y, lv_opa_t opa)
{
    if (opa == LV_OPA_TRANSP) {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    display_service_style_rect(obj, 116, 40, 20, lv_color_hex(0xFF6B9D), opa);
    lv_obj_set_pos(obj, x, y);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static void display_service_style_mouth(display_mouth_t mouth, lv_color_t color)
{
    const lv_point_t *points = NULL;
    uint32_t point_count = 0;

    switch (mouth) {
    case DISPLAY_MOUTH_SMILE:
        points = s_smile_points;
        point_count = sizeof(s_smile_points) / sizeof(s_smile_points[0]);
        break;
    case DISPLAY_MOUTH_WAVE:
        points = s_shy_points;
        point_count = sizeof(s_shy_points) / sizeof(s_shy_points[0]);
        break;
    case DISPLAY_MOUTH_BLINK:
        points = s_blink_points;
        point_count = sizeof(s_blink_points) / sizeof(s_blink_points[0]);
        break;
    case DISPLAY_MOUTH_FLAT:
        points = s_error_points;
        point_count = sizeof(s_error_points) / sizeof(s_error_points[0]);
        break;
    case DISPLAY_MOUTH_FROWN:
        s_runtime_mouth_points[0] = (lv_point_t){345, 352};
        s_runtime_mouth_points[1] = (lv_point_t){378, 336};
        s_runtime_mouth_points[2] = (lv_point_t){422, 336};
        s_runtime_mouth_points[3] = (lv_point_t){455, 352};
        points = s_runtime_mouth_points;
        point_count = 4;
        break;
    case DISPLAY_MOUTH_NONE:
    default:
        lv_obj_add_flag(s_mouth_line, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_line_set_points(s_mouth_line, points, point_count);
    lv_obj_set_style_line_color(s_mouth_line, color, LV_PART_MAIN);
    lv_obj_set_style_line_width(s_mouth_line, 8, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(s_mouth_line, true, LV_PART_MAIN);
    lv_obj_clear_flag(s_mouth_line, LV_OBJ_FLAG_HIDDEN);
}

static void display_service_expression_defaults(display_expression_t *expr, lv_color_t color)
{
    expr->eye_center_y = DISPLAY_EYE_BASE_CENTER_Y;
    expr->eye_width = DISPLAY_EYE_BASE_W;
    expr->left_eye_height = DISPLAY_EYE_BASE_H;
    expr->right_eye_height = DISPLAY_EYE_BASE_H;
    expr->face_offset_x = 0;
    expr->cheek_opa = LV_OPA_TRANSP;
    expr->mouth_dot_width = 104;
    expr->mouth_dot_height = 42;
    expr->mouth_dot_opa = LV_OPA_TRANSP;
    expr->sleep_opa = LV_OPA_TRANSP;
    expr->mouth = DISPLAY_MOUTH_SMILE;
    expr->eye_color = color;
}

static void display_service_fill_expression_locked(display_face_t face, display_expression_t *expr)
{
    const lv_color_t green = lv_color_hex(0x39FF6E);
    const lv_color_t red = lv_color_hex(0xFF5A5A);

    display_service_expression_defaults(expr, green);

    switch (face) {
    case DISPLAY_FACE_SHY:
        expr->eye_center_y = 230;
        expr->left_eye_height = 72;
        expr->right_eye_height = 72;
        expr->cheek_opa = 82;
        expr->mouth = DISPLAY_MOUTH_WAVE;
        break;
    case DISPLAY_FACE_EXCITED:
        expr->eye_center_y = 206;
        expr->eye_width = 126;
        expr->left_eye_height = 176;
        expr->right_eye_height = 176;
        expr->mouth = DISPLAY_MOUTH_NONE;
        expr->mouth_dot_width = 104;
        expr->mouth_dot_height = 42;
        expr->mouth_dot_opa = LV_OPA_COVER;
        break;
    case DISPLAY_FACE_LISTENING:
        expr->eye_width = 124;
        expr->left_eye_height = 144;
        expr->right_eye_height = 144;
        expr->mouth = DISPLAY_MOUTH_WAVE;
        break;
    case DISPLAY_FACE_PROCESSING:
        expr->left_eye_height = 118;
        expr->right_eye_height = 118;
        expr->mouth = DISPLAY_MOUTH_FLAT;
        break;
    case DISPLAY_FACE_SPEAKING:
        expr->eye_width = 122;
        expr->left_eye_height = 152;
        expr->right_eye_height = 152;
        expr->mouth = DISPLAY_MOUTH_NONE;
        expr->mouth_dot_width = 96;
        expr->mouth_dot_height = 36;
        expr->mouth_dot_opa = LV_OPA_COVER;
        break;
    case DISPLAY_FACE_LOOK_LEFT:
        expr->face_offset_x = -DISPLAY_FACE_SHIFT_PX;
        break;
    case DISPLAY_FACE_LOOK_RIGHT:
        expr->face_offset_x = DISPLAY_FACE_SHIFT_PX;
        break;
    case DISPLAY_FACE_BLINK:
        expr->mouth = DISPLAY_MOUTH_BLINK;
        break;
    case DISPLAY_FACE_SLEEP:
        expr->left_eye_height = 20;
        expr->right_eye_height = 20;
        expr->mouth = DISPLAY_MOUTH_NONE;
        expr->sleep_opa = LV_OPA_COVER;
        break;
    case DISPLAY_FACE_ERROR:
        expr->mouth = DISPLAY_MOUTH_FROWN;
        expr->eye_color = red;
        break;
    case DISPLAY_FACE_NONE:
    case DISPLAY_FACE_HAPPY:
    default:
        break;
    }
}

static void display_service_step_expression_locked(const display_expression_t *target)
{
    if (!s_expression_ready) {
        s_current_expression = *target;
        s_expression_ready = true;
        return;
    }

    s_current_expression.eye_center_y = display_service_step_value(s_current_expression.eye_center_y, target->eye_center_y, DISPLAY_POSE_LERP_DIV);
    s_current_expression.eye_width = display_service_step_value(s_current_expression.eye_width, target->eye_width, DISPLAY_POSE_LERP_DIV);
    s_current_expression.left_eye_height = display_service_step_value(s_current_expression.left_eye_height, target->left_eye_height, DISPLAY_POSE_LERP_DIV);
    s_current_expression.right_eye_height = display_service_step_value(s_current_expression.right_eye_height, target->right_eye_height, DISPLAY_POSE_LERP_DIV);
    s_current_expression.face_offset_x = display_service_step_value_limited(s_current_expression.face_offset_x,
                                                                            target->face_offset_x,
                                                                            DISPLAY_FACE_SHIFT_STEP_PX);
    s_current_expression.cheek_opa = display_service_step_value(s_current_expression.cheek_opa, target->cheek_opa, DISPLAY_POSE_LERP_DIV);
    s_current_expression.mouth_dot_width = display_service_step_value(s_current_expression.mouth_dot_width, target->mouth_dot_width, DISPLAY_POSE_LERP_DIV);
    s_current_expression.mouth_dot_height = display_service_step_value(s_current_expression.mouth_dot_height, target->mouth_dot_height, DISPLAY_POSE_LERP_DIV);
    s_current_expression.mouth_dot_opa = display_service_step_value(s_current_expression.mouth_dot_opa, target->mouth_dot_opa, DISPLAY_POSE_LERP_DIV);
    s_current_expression.sleep_opa = display_service_step_value(s_current_expression.sleep_opa, target->sleep_opa, DISPLAY_POSE_LERP_DIV);
    s_current_expression.mouth = target->mouth;
    s_current_expression.eye_color = target->eye_color;
}

static void display_service_apply_face_locked(display_face_t face, int64_t now)
{
    display_expression_t target;

    display_service_fill_expression_locked(face, &target);
    display_service_step_expression_locked(&target);

    int32_t left_eye_height = s_current_expression.left_eye_height;
    int32_t right_eye_height = s_current_expression.right_eye_height;
    int32_t left_blink_open = display_service_blink_openness_locked(now, true);
    int32_t right_blink_open = display_service_blink_openness_locked(now, false);
    int32_t left_eye_height_blinked = LV_MAX(14, (left_eye_height * left_blink_open) / 1000);
    int32_t right_eye_height_blinked = LV_MAX(14, (right_eye_height * right_blink_open) / 1000);
    int32_t left_eye_x = DISPLAY_EYE_LEFT_CENTER_X - (s_current_expression.eye_width / 2);
    int32_t right_eye_x = DISPLAY_EYE_RIGHT_CENTER_X - (s_current_expression.eye_width / 2);
    int32_t left_eye_y = s_current_expression.eye_center_y - (left_eye_height_blinked / 2);
    int32_t right_eye_y = s_current_expression.eye_center_y - (right_eye_height_blinked / 2);

    lv_obj_set_pos(s_face_root, s_current_expression.face_offset_x, 0);

    display_service_style_eye(s_left_eye,
                              left_eye_x,
                              left_eye_y,
                              s_current_expression.eye_width,
                              left_eye_height_blinked,
                              s_current_expression.eye_color,
                              LV_OPA_COVER);
    display_service_style_eye(s_right_eye,
                              right_eye_x,
                              right_eye_y,
                              s_current_expression.eye_width,
                              right_eye_height_blinked,
                              s_current_expression.eye_color,
                              LV_OPA_COVER);

    display_service_style_cheek(s_left_cheek, 104, 280, (lv_opa_t)s_current_expression.cheek_opa);
    display_service_style_cheek(s_right_cheek, 580, 280, (lv_opa_t)s_current_expression.cheek_opa);
    display_service_style_mouth(s_current_expression.mouth, s_current_expression.eye_color);
    display_service_style_mouth_dot(s_mouth_dot,
                                    400 - (s_current_expression.mouth_dot_width / 2),
                                    336,
                                    s_current_expression.mouth_dot_width,
                                    s_current_expression.mouth_dot_height,
                                    s_current_expression.eye_color,
                                    (lv_opa_t)s_current_expression.mouth_dot_opa);

    if (s_current_expression.sleep_opa == LV_OPA_TRANSP) {
        lv_obj_add_flag(s_sleep_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_set_style_text_opa(s_sleep_label, (lv_opa_t)s_current_expression.sleep_opa, LV_PART_MAIN);
        lv_obj_clear_flag(s_sleep_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static esp_err_t display_service_i2c_init(void)
{
    if (s_i2c_ready) {
        return ESP_OK;
    }

    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = DISPLAY_I2C_SDA,
        .scl_io_num = DISPLAY_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = DISPLAY_I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(DISPLAY_I2C_PORT, &i2c_conf);
    ESP_RETURN_ON_ERROR(err, TAG, "i2c_param_config failed");

    err = i2c_driver_install(DISPLAY_I2C_PORT, i2c_conf.mode, 0, 0, 0);
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        ESP_RETURN_ON_ERROR(err, TAG, "i2c_driver_install failed");
    }

    s_i2c_ready = true;
    return ESP_OK;
}

static esp_err_t display_service_backlight_on(void)
{
    uint8_t write_buf = DISPLAY_BACKLIGHT_ENABLE_REG;
    esp_err_t err = i2c_master_write_to_device(DISPLAY_I2C_PORT,
                                               DISPLAY_IO_EXPANDER_ADDR,
                                               &write_buf,
                                               sizeof(write_buf),
                                               pdMS_TO_TICKS(DISPLAY_I2C_TIMEOUT_MS));
    ESP_RETURN_ON_ERROR(err, TAG, "enable io expander failed");

    write_buf = DISPLAY_BACKLIGHT_ON_REG;
    err = i2c_master_write_to_device(DISPLAY_I2C_PORT,
                                     DISPLAY_BACKLIGHT_ADDR,
                                     &write_buf,
                                     sizeof(write_buf),
                                     pdMS_TO_TICKS(DISPLAY_I2C_TIMEOUT_MS));
    ESP_RETURN_ON_ERROR(err, TAG, "turn on backlight failed");

    return ESP_OK;
}

static esp_err_t display_service_touch_reset(void)
{
    const gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DISPLAY_TOUCH_INT_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "touch int gpio config failed");

    uint8_t write_buf = DISPLAY_BACKLIGHT_ENABLE_REG;
    esp_err_t err = i2c_master_write_to_device(DISPLAY_I2C_PORT,
                                               DISPLAY_IO_EXPANDER_ADDR,
                                               &write_buf,
                                               sizeof(write_buf),
                                               pdMS_TO_TICKS(DISPLAY_I2C_TIMEOUT_MS));
    ESP_RETURN_ON_ERROR(err, TAG, "touch expander enable failed");

    write_buf = DISPLAY_TOUCH_RESET_LOW_REG;
    err = i2c_master_write_to_device(DISPLAY_I2C_PORT,
                                     DISPLAY_BACKLIGHT_ADDR,
                                     &write_buf,
                                     sizeof(write_buf),
                                     pdMS_TO_TICKS(DISPLAY_I2C_TIMEOUT_MS));
    ESP_RETURN_ON_ERROR(err, TAG, "touch reset low failed");

    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(DISPLAY_TOUCH_INT_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    write_buf = DISPLAY_TOUCH_RESET_HIGH_REG;
    err = i2c_master_write_to_device(DISPLAY_I2C_PORT,
                                     DISPLAY_BACKLIGHT_ADDR,
                                     &write_buf,
                                     sizeof(write_buf),
                                     pdMS_TO_TICKS(DISPLAY_I2C_TIMEOUT_MS));
    ESP_RETURN_ON_ERROR(err, TAG, "touch reset high failed");

    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_reset_pin(DISPLAY_TOUCH_INT_GPIO);
    return ESP_OK;
}

static esp_err_t display_service_lcd_init(void)
{
    const esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = DISPLAY_PIXEL_CLOCK_HZ,
            .h_res = DISPLAY_H_RES,
            .v_res = DISPLAY_V_RES,
            .hsync_pulse_width = 4,
            .hsync_back_porch = 8,
            .hsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .vsync_back_porch = 8,
            .vsync_front_porch = 8,
            .flags = {
                .pclk_active_neg = 1,
            },
        },
        .data_width = 16,
        .bits_per_pixel = 16,
        .num_fbs = DISPLAY_RGB_FB_NUMS,
        .bounce_buffer_size_px = DISPLAY_H_RES * DISPLAY_RGB_BOUNCE_LINES,
        .sram_trans_align = 4,
        .psram_trans_align = 64,
        .hsync_gpio_num = DISPLAY_GPIO_HSYNC,
        .vsync_gpio_num = DISPLAY_GPIO_VSYNC,
        .de_gpio_num = DISPLAY_GPIO_DE,
        .pclk_gpio_num = DISPLAY_GPIO_PCLK,
        .disp_gpio_num = DISPLAY_GPIO_DISP,
        .data_gpio_nums = {
            DISPLAY_GPIO_DATA0,
            DISPLAY_GPIO_DATA1,
            DISPLAY_GPIO_DATA2,
            DISPLAY_GPIO_DATA3,
            DISPLAY_GPIO_DATA4,
            DISPLAY_GPIO_DATA5,
            DISPLAY_GPIO_DATA6,
            DISPLAY_GPIO_DATA7,
            DISPLAY_GPIO_DATA8,
            DISPLAY_GPIO_DATA9,
            DISPLAY_GPIO_DATA10,
            DISPLAY_GPIO_DATA11,
            DISPLAY_GPIO_DATA12,
            DISPLAY_GPIO_DATA13,
            DISPLAY_GPIO_DATA14,
            DISPLAY_GPIO_DATA15,
        },
        .flags = {
            .fb_in_psram = 1,
        },
    };

    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &s_panel_handle);
    ESP_RETURN_ON_ERROR(err, TAG, "esp_lcd_new_rgb_panel failed");

    err = esp_lcd_panel_init(s_panel_handle);
    ESP_RETURN_ON_ERROR(err, TAG, "esp_lcd_panel_init failed");

    return ESP_OK;
}

static esp_err_t display_service_touch_init(void)
{
    if (s_touch_ready) {
        return ESP_OK;
    }

    esp_err_t err = display_service_touch_reset();
    ESP_RETURN_ON_ERROR(err, TAG, "display_service_touch_reset failed");

    esp_lcd_panel_io_i2c_config_t touch_io_config =
        ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG_WITH_ADDR(s_touch_gt911_driver_cfg.dev_addr);
    err = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)DISPLAY_I2C_PORT,
                                   &touch_io_config,
                                   &s_touch_io_handle);
    ESP_RETURN_ON_ERROR(err, TAG, "esp_lcd_new_panel_io_i2c failed");

    const esp_lcd_touch_config_t touch_cfg = {
        .x_max = DISPLAY_H_RES,
        .y_max = DISPLAY_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = DISPLAY_TOUCH_INT_GPIO,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .driver_data = &s_touch_gt911_driver_cfg,
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    err = esp_lcd_touch_new_i2c_gt911(s_touch_io_handle, &touch_cfg, &s_touch_handle);
    ESP_RETURN_ON_ERROR(err, TAG, "esp_lcd_touch_new_i2c_gt911 failed");

    s_touch_ready = true;
    ESP_LOGI(TAG, "GT911 address preselected by board reset: 0x%02X", s_touch_gt911_driver_cfg.dev_addr);
    ESP_LOGI(TAG, "Touch controller GT911 initialized");
    return ESP_OK;
}

static esp_err_t display_service_lvgl_init(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 8192,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT,
        .timer_period_ms = 5,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .panel_handle = s_panel_handle,
        .buffer_size = DISPLAY_H_RES * DISPLAY_V_RES,
        .double_buffer = false,
        .hres = DISPLAY_H_RES,
        .vres = DISPLAY_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = false,
            .buff_spiram = false,
            .sw_rotate = false,
            .direct_mode = true,
            .full_refresh = false,
        },
    };
    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = true,
            .avoid_tearing = true,
        },
    };

    s_display = lvgl_port_add_disp_rgb(&disp_cfg, &rgb_cfg);
    if (s_display == NULL) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void display_service_update_touch_locked(void)
{
    int64_t now = esp_timer_get_time();

    if (s_touch_label == NULL) {
        return;
    }

    if (!s_touch_ready || (s_touch_handle == NULL)) {
        lv_label_set_text(s_touch_label, "Touch: unavailable");
        return;
    }

    esp_err_t err = esp_lcd_touch_read_data(s_touch_handle);
    if (err != ESP_OK) {
        lv_label_set_text(s_touch_label, "Touch: read error");
        return;
    }

    uint16_t touch_x = 0;
    uint16_t touch_y = 0;
    uint16_t touch_strength = 0;
    uint8_t touch_points = 0;
    display_touch_zone_t zone = DISPLAY_TOUCH_ZONE_NONE;
    bool touched = esp_lcd_touch_get_coordinates(s_touch_handle,
                                                 &touch_x,
                                                 &touch_y,
                                                 &touch_strength,
                                                 &touch_points,
                                                 1);

    if (touched && (touch_points > 0)) {
        zone = display_service_touch_zone_from_point(touch_x, touch_y);
        s_last_interaction_us = now;
        s_touch_active_zone = zone;
        (void)touch_strength;

        if (!s_touch_pressed) {
            s_touch_press_started_us = now;
            s_long_press_triggered = false;
            s_auto_face = DISPLAY_FACE_NONE;
            s_auto_face_expire_us = 0;
            display_service_schedule_gaze_locked(now);
            display_service_schedule_blink_locked(now);
            display_service_apply_touch_zone_locked(zone, now);
        }

        if (!s_long_press_triggered &&
            (zone != DISPLAY_TOUCH_ZONE_BOTTOM_RIGHT) &&
            (s_touch_press_started_us > 0) &&
            ((now - s_touch_press_started_us) >= DISPLAY_TOUCH_LONG_PRESS_US)) {
            display_service_start_touch_face_locked(DISPLAY_FACE_EXCITED, now, DISPLAY_EXCITED_HOLD_US);
            s_long_press_triggered = true;
        }

        lv_label_set_text_fmt(s_touch_label,
                              "Touch: pressed (%u, %u) [%s]",
                              touch_x,
                              touch_y,
                              display_service_touch_zone_name(zone));
    } else {
        lv_label_set_text(s_touch_label, "Touch: released");
        s_touch_press_started_us = 0;
        s_long_press_triggered = false;
        s_touch_active_zone = DISPLAY_TOUCH_ZONE_NONE;
    }

    s_touch_pressed = touched && (touch_points > 0);
}

static void display_service_update_ui_locked(robot_state_t state, int64_t now)
{
    display_face_t face = display_service_visual_face_from_state(state, now);

    display_service_apply_face_locked(face, now);
    if ((face != s_last_status_face) || (state != s_last_status_state)) {
        lv_label_set_text_fmt(s_state_label,
                              "Face: %s | Link: %s",
                              display_service_face_name(face),
                              emotion_engine_state_name(state));
        s_last_status_face = face;
        s_last_status_state = state;
    }
}

static void display_service_create_ui(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    s_face_root = lv_obj_create(scr);
    lv_obj_remove_style_all(s_face_root);
    lv_obj_set_size(s_face_root, DISPLAY_H_RES, DISPLAY_V_RES);
    lv_obj_set_pos(s_face_root, 0, 0);
    lv_obj_clear_flag(s_face_root, LV_OBJ_FLAG_SCROLLABLE);

    s_left_eye = lv_obj_create(s_face_root);
    s_right_eye = lv_obj_create(s_face_root);
    s_left_cheek = lv_obj_create(s_face_root);
    s_right_cheek = lv_obj_create(s_face_root);
    s_mouth_line = lv_line_create(s_face_root);
    s_mouth_dot = lv_obj_create(s_face_root);
    s_sleep_label = lv_label_create(scr);

    lv_obj_add_flag(s_left_cheek, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_right_cheek, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_mouth_dot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_sleep_label, LV_OBJ_FLAG_HIDDEN);

    lv_obj_set_style_bg_opa(s_left_eye, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_right_eye, LV_OPA_COVER, LV_PART_MAIN);

    lv_label_set_text(s_sleep_label, "Z z");
    lv_obj_set_style_text_color(s_sleep_label, lv_color_hex(0xA78BFA), LV_PART_MAIN);
    lv_obj_align(s_sleep_label, LV_ALIGN_TOP_RIGHT, -72, 34);

    s_state_label = lv_label_create(scr);
    lv_label_set_text(s_state_label, "Face: happy");
    lv_obj_set_style_text_color(s_state_label, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_align(s_state_label, LV_ALIGN_BOTTOM_MID, 0, -42);

    s_hint_label = lv_label_create(scr);
    lv_label_set_text(s_hint_label,
                      "TL=lookL TM=shy TR=lookR | ML=listen MM=happy MR=proc | BL=blink BM=speak BR=sleep | Hold=excited");
    lv_obj_set_style_text_color(s_hint_label, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_align(s_hint_label, LV_ALIGN_BOTTOM_MID, 0, -24);

    s_touch_label = lv_label_create(scr);
    lv_label_set_text(s_touch_label, s_touch_ready ? "Touch: ready" : "Touch: unavailable");
    lv_obj_set_style_text_color(s_touch_label, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_align(s_touch_label, LV_ALIGN_BOTTOM_MID, 0, -6);

    s_last_interaction_us = esp_timer_get_time();
    display_service_schedule_gaze_locked(s_last_interaction_us);
    display_service_schedule_blink_locked(s_last_interaction_us);
    display_service_update_ui_locked(ROBOT_STATE_BOOTING, s_last_interaction_us);
    display_service_update_touch_locked();
}

void display_service_init(void)
{
    esp_err_t err = display_service_i2c_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Display I2C init failed: %s", esp_err_to_name(err));
        return;
    }

    err = display_service_lcd_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LCD init failed: %s", esp_err_to_name(err));
        return;
    }

    err = display_service_touch_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch init failed: %s", esp_err_to_name(err));
    }

    err = display_service_backlight_on();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Backlight enable failed: %s", esp_err_to_name(err));
    }

    err = display_service_lvgl_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LVGL init failed: %s", esp_err_to_name(err));
        return;
    }

    if (lvgl_port_lock(0)) {
        display_service_create_ui();
        lvgl_port_unlock();
        s_display_ready = true;
    }

    ESP_LOGI(TAG, "Display service initialized");
}

void display_service_render(robot_state_t state)
{
    static robot_state_t last_state = ROBOT_STATE_BOOTING;
    bool state_changed = (state != last_state);

    if (s_display_ready && lvgl_port_lock(0)) {
        display_service_update_touch_locked();

        int64_t now = esp_timer_get_time();
        display_service_update_ui_locked(state, now);
        lvgl_port_unlock();
    }

    if (state_changed) {
        ESP_LOGI(TAG, "Render face state: %s", emotion_engine_state_name(state));
    }

    last_state = state;
}
