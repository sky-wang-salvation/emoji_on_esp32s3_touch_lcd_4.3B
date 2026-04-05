idf_component_register(
    SRCS "robot_link.c"
    INCLUDE_DIRS "include"
    REQUIRES emotion esp_driver_usb_serial_jtag
)
