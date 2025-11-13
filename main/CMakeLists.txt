idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES esp_timer esp_lcd lvgl calc gt911
    PRIV_REQUIRES driver esp_driver_gpio esp_driver_i2c esp_system
)
