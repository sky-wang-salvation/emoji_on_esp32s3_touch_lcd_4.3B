#include "include/emotion_engine.h"

#include <stdint.h>

#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "emotion_engine";

static robot_state_t s_active_state = ROBOT_STATE_BOOTING;
static robot_state_t s_pending_state = ROBOT_STATE_IDLE;
static int64_t s_last_change_us = 0;

void emotion_engine_init(void)
{
    s_active_state = ROBOT_STATE_BOOTING;
    s_pending_state = ROBOT_STATE_IDLE;
    s_last_change_us = esp_timer_get_time();
    ESP_LOGI(TAG, "Emotion engine initialized");
}

void emotion_engine_request_state(robot_state_t requested_state)
{
    s_pending_state = requested_state;
}

void emotion_engine_update(void)
{
    int64_t now = esp_timer_get_time();

    if (s_active_state == ROBOT_STATE_BOOTING && now - s_last_change_us > 1500 * 1000) {
        s_active_state = ROBOT_STATE_IDLE;
        s_last_change_us = now;
        ESP_LOGI(TAG, "Auto transition: %s", emotion_engine_state_name(s_active_state));
        return;
    }

    if (s_pending_state != s_active_state) {
        s_active_state = s_pending_state;
        s_last_change_us = now;
        ESP_LOGI(TAG, "State changed to %s", emotion_engine_state_name(s_active_state));
    }
}

robot_state_t emotion_engine_get_state(void)
{
    return s_active_state;
}

const char *emotion_engine_state_name(robot_state_t state)
{
    switch (state) {
    case ROBOT_STATE_BOOTING:
        return "booting";
    case ROBOT_STATE_IDLE:
        return "idle";
    case ROBOT_STATE_LISTENING:
        return "listening";
    case ROBOT_STATE_PROCESSING:
        return "processing";
    case ROBOT_STATE_SPEAKING:
        return "speaking";
    case ROBOT_STATE_HAPPY:
        return "happy";
    case ROBOT_STATE_ERROR:
        return "error";
    case ROBOT_STATE_SLEEP:
        return "sleep";
    default:
        return "unknown";
    }
}
