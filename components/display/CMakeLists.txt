idf_component_register(
    SRCS "display_service.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_lcd esp_timer emotion esp32_display_panel esp_lvgl_port lvgl
)

target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>)
