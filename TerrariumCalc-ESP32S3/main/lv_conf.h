#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_USE_OS          LV_OS_FREERTOS
#define LV_COLOR_DEPTH     16
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00FF00)
#define LV_MEM_CUSTOM      0
#define LV_USE_LOG         1
#define LV_LOG_LEVEL       LV_LOG_LEVEL_INFO
#define LV_FONT_DEFAULT    &lv_font_montserrat_20
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_USE_DRAW_SW     1
#define LV_USE_DRAW_SW_COMPLEX 1
#define LV_USE_PERF_MONITOR 0
#define LV_USE_OS_WRAPPER  1

#endif /* LV_CONF_H */
