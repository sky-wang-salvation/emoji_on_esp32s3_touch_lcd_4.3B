#include "include/robot_link.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "robot_link";
static int64_t s_started_at_us = 0;
static robot_state_t s_current_state = ROBOT_STATE_IDLE;
static char s_rx_line[64];
static size_t s_rx_line_len = 0;
static bool s_usb_link_ready = false;

static robot_state_t robot_link_state_from_token(const char *token, bool *matched)
{
    if (matched != NULL) {
        *matched = true;
    }

    if (strcasecmp(token, "booting") == 0) {
        return ROBOT_STATE_BOOTING;
    }
    if ((strcasecmp(token, "idle") == 0) || (strcasecmp(token, "neutral") == 0)) {
        return ROBOT_STATE_IDLE;
    }
    if ((strcasecmp(token, "listening") == 0) || (strcasecmp(token, "listen") == 0)) {
        return ROBOT_STATE_LISTENING;
    }
    if ((strcasecmp(token, "processing") == 0) || (strcasecmp(token, "thinking") == 0)) {
        return ROBOT_STATE_PROCESSING;
    }
    if ((strcasecmp(token, "speaking") == 0) || (strcasecmp(token, "speak") == 0) ||
        (strcasecmp(token, "talking") == 0)) {
        return ROBOT_STATE_SPEAKING;
    }
    if ((strcasecmp(token, "happy") == 0) || (strcasecmp(token, "smile") == 0)) {
        return ROBOT_STATE_HAPPY;
    }
    if ((strcasecmp(token, "sleep") == 0) || (strcasecmp(token, "sleeping") == 0)) {
        return ROBOT_STATE_SLEEP;
    }
    if ((strcasecmp(token, "error") == 0) || (strcasecmp(token, "fault") == 0)) {
        return ROBOT_STATE_ERROR;
    }

    if (matched != NULL) {
        *matched = false;
    }
    return s_current_state;
}

static void robot_link_apply_line(const char *line)
{
    char token[32];
    size_t line_len = strlen(line);
    bool matched_any = false;
    robot_state_t next_state = s_current_state;

    for (size_t i = 0; i < line_len;) {
        size_t token_len = 0;

        while ((i < line_len) && !isalpha((unsigned char)line[i])) {
            i++;
        }

        while ((i < line_len) && (token_len < (sizeof(token) - 1U))) {
            char ch = line[i];
            if (!isalpha((unsigned char)ch) && (ch != '_')) {
                break;
            }
            token[token_len++] = ch;
            i++;
        }

        while ((i < line_len) && isalpha((unsigned char)line[i])) {
            i++;
        }

        if (token_len == 0) {
            continue;
        }

        token[token_len] = '\0';
        bool matched = false;
        robot_state_t parsed = robot_link_state_from_token(token, &matched);
        if (matched) {
            next_state = parsed;
            matched_any = true;
        }
    }

    if (!matched_any) {
        ESP_LOGW(TAG, "Ignore unknown state command: %s", line);
        return;
    }

    if (next_state != s_current_state) {
        s_current_state = next_state;
        ESP_LOGI(TAG, "USB state -> %s", emotion_engine_state_name(s_current_state));
    }
}

static void robot_link_consume_rx(const char *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        char ch = data[i];

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            s_rx_line[s_rx_line_len] = '\0';
            robot_link_apply_line(s_rx_line);
            s_rx_line_len = 0;
            continue;
        }

        if (!isprint((unsigned char)ch)) {
            continue;
        }

        if (s_rx_line_len < (sizeof(s_rx_line) - 1U)) {
            s_rx_line[s_rx_line_len++] = ch;
        } else {
            s_rx_line_len = 0;
        }
    }
}

void robot_link_init(void)
{
    usb_serial_jtag_driver_config_t usb_cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

    s_started_at_us = esp_timer_get_time();
    s_current_state = ROBOT_STATE_IDLE;

    if (!usb_serial_jtag_is_driver_installed()) {
        esp_err_t err = usb_serial_jtag_driver_install(&usb_cfg);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "USB Serial JTAG init failed, fallback to idle: %s", esp_err_to_name(err));
            return;
        }
    }

    s_usb_link_ready = true;
    ESP_LOGI(TAG, "Robot link ready on USB Serial JTAG");
    ESP_LOGI(TAG, "Send one line state: idle/listening/processing/speaking/happy/sleep/error");
}

robot_state_t robot_link_poll(void)
{
    char rx_buf[64];

    (void)s_started_at_us;

    if (!s_usb_link_ready) {
        return s_current_state;
    }

    int read_len = usb_serial_jtag_read_bytes(rx_buf, sizeof(rx_buf), 0);
    if (read_len > 0) {
        robot_link_consume_rx(rx_buf, (size_t)read_len);
    }

    return s_current_state;
}
