idf_component_register(
    SRCS "gt911.c"
    INCLUDE_DIRS "."
    REQUIRES
        freertos
        driver
        esp_driver_gpio
        esp_driver_i2c
)
