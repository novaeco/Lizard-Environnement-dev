# TerrariumCalc ESP32-S3 LVGL Platform

This repository contains a reference ESP-IDF project configured for ESP32-S3 hardware with an RGB LCD panel driven through LVGL v9. The implementation demonstrates how to bring up the display pipeline, register the LVGL flush callback, and schedule the LVGL tick timer using the native ESP timer service.

## Building

```bash
idf.py set-target esp32s3
idf.py build
```

## Flashing

```bash
idf.py flash monitor
```

Adjust the pin assignments in `main/app_main.c` to match your hardware wiring if it differs from the default TerrariumCalc board layout.
