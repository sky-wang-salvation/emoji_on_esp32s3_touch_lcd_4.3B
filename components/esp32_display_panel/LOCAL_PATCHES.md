# Local Patches For `esp32_display_panel`

This project vendors `esp32_display_panel` locally under `components/esp32_display_panel`
instead of relying on the ESP-IDF component registry for this package.

## Upstream Base

- Component: `espressif/esp32_display_panel`
- Upstream version: `1.0.4`
- Upstream repository: `https://github.com/esp-arduino-libs/ESP32_Display_Panel`
- Upstream commit recorded in `idf_component.yml`: `12a01d6569e5c0918389793cee5776924e905975`

## Local Modifications

### GT911 address initialization

File:
`src/drivers/touch/port/esp_lcd_touch_gt911.c`

Reason:
Some Waveshare board reset sequences preselect the GT911 I2C address before the
driver runs. In that case, the original upstream log `Unable to initialize the I2C address`
is misleading even though the controller is already reachable.

Behavior of the local patch:

- Detects whether the controller already responds on the configured address
- Skips the internal address switch when the address is already valid
- Replaces the misleading warning with an informational log

## Maintenance Notes

- If you upgrade this vendored component, diff the local `esp_lcd_touch_gt911.c`
  against the new upstream version and reapply only the GT911 board-specific logic
- Keep `main/idf_component.yml` free of `espressif/esp32_display_panel` so ESP-IDF
  continues using this local component
- `dependencies.lock` should only track the remaining registry-managed dependencies
  such as `esp32_io_expander`, `esp-lib-utils`, `esp_lvgl_port`, and `lvgl`
