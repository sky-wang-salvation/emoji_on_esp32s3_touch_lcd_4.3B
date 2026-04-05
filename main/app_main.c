#include "bsp_board.h"
#include "display_service.h"
#include "emotion_engine.h"
#include "robot_link.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "app_main";

void app_main(void)
{
    bsp_board_init();
    display_service_init();
    emotion_engine_init();
    robot_link_init();

    ESP_LOGI(TAG, "Robot face display scaffold started");

    while (1) {
        robot_state_t requested_state = robot_link_poll();
        emotion_engine_request_state(requested_state);
        emotion_engine_update();
        display_service_render(emotion_engine_get_state());

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
