cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components/lvgl)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(TerrariumCalc-ESP32S3)

# Ensure the upstream LVGL component can locate the project's lv_conf.h
idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
target_include_directories(${lvgl_lib} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/main")
