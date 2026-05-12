# set(component_srcs "ssd1306.c")
# set(component_requires "driver esp_driver_i2c esp_driver_gpio")
# set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")
# if(idf_version VERSION_GREATER_EQUAL "5.2")
#     if(CONFIG_LEGACY_DRIVER)
#         list(APPEND component_srcs "ssd1306_i2c_legacy.c")
#     else()
#         list(APPEND component_srcs "ssd1306_i2c_new.c")
#     endif()
# else()
#     list(APPEND component_srcs "ssd1306_i2c_legacy.c")
# endif()
# idf_component_register(SRCS "${component_srcs}" PRIV_REQUIRES "${component_requires}" INCLUDE_DIRS ".")
set(component_srcs
    "ssd1306.c"
    "ssd1306_i2c_legacy.c"
)

set(component_requires
    driver
    esp_driver_i2c
    esp_driver_gpio
)

idf_component_register(
    SRCS
        "ssd1306.c"
        "ssd1306_i2c_legacy.c"
        "ssd1306_spi.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        driver
        esp_driver_i2c
        esp_driver_gpio
        esp_driver_spi
)