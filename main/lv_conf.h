#ifndef LV_CONF_H
#define LV_CONF_H

#include "sdkconfig.h"

#define LV_USE_OS           LV_OS_FREERTOS
#define LV_COLOR_DEPTH      16
#define LV_COLOR_16_SWAP    0
#define LV_USE_LOG          1
#define LV_LOG_LEVEL        LV_LOG_LEVEL_INFO
#define LV_USE_DRAW_SW      1
#define LV_USE_GPU_STM32_DMA2D 0
#define LV_USE_PERF_MONITOR 1
#define LV_MEM_SIZE         (256U * 1024U)
#define LV_USE_DEBUG        0

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1

#define LV_BUILD_EXAMPLES   0

#endif /* LV_CONF_H */
