idf_component_register(
    SRCS
        "main.cpp"
        "sensors.cpp"
        "display.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_driver_i2c
        esp_timer
        log
        ssd1306
)