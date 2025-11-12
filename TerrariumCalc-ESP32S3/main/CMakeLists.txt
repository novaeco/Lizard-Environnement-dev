idf_component_register(SRCS "app_main.c"
                       INCLUDE_DIRS "."
                       REQUIRES calc gt911 lvgl esp_lcd esp_timer driver esp_psram)
