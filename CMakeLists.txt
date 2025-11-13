cmake_minimum_required(VERSION 3.24)

if(NOT DEFINED ENV{IDF_PATH})
    message(FATAL_ERROR "IDF_PATH n'est pas défini : exécutez export.sh depuis l'ESP-IDF 6.1")
endif()

set(EXTRA_COMPONENT_DIRS "components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(
    TerrariumCalc_ESP32S3
    VERSION 1.1.0
    DESCRIPTION "Calculateur LVGL pour terrarium avec écran Waveshare 7B"
)
