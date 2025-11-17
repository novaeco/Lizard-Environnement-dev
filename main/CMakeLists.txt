idf_component_register(
    SRCS
        "app_main.c"
        "calc_heating_pad.c"
        "calc_heating_cable.c"
        "calc_lighting.c"
        "calc_substrate.c"
        "calc_misting.c"
        "storage.c"
        "ui_main.c"
        "ui_keyboard.c"
        "ui_screens_home.c"
        "ui_screens_pad.c"
        "ui_screens_cable.c"
        "ui_screens_lighting.c"
        "ui_screens_substrate.c"
        "ui_screens_misting.c"
        "ui_screens_about.c"
    INCLUDE_DIRS "."
    REQUIRES esp_timer esp_lcd lvgl gt911 nvs_flash
    INCLUDE_DIRS "."
    REQUIRES esp_timer esp_lcd lvgl gt911
    PRIV_REQUIRES driver esp_driver_gpio esp_driver_i2c esp_system
)
