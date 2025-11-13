idf_component_register(
    SRCS "gt911.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES
        freertos
        driver
)

idf_component_optional_requires(PRIVATE esp_driver_gpio esp_driver_i2c)
