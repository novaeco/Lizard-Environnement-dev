set(required_components
    freertos
    driver
)

if(DEFINED IDF_VERSION_MAJOR AND IDF_VERSION_MAJOR GREATER 4)
    list(APPEND required_components esp_driver_gpio esp_driver_i2c)
endif()

idf_component_register(
    SRCS "gt911.c"
    INCLUDE_DIRS "."
    REQUIRES ${required_components}
)
