#include "include/bsp_board.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "bsp_board";

void bsp_board_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_flash_init returned %s", esp_err_to_name(err));
    }

    ESP_LOGI(TAG, "Board bootstrap finished");
}
