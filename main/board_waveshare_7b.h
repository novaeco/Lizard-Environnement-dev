#pragma once

#include "driver/gpio.h"
#include "esp_lcd_types.h"

/**
 * Résolution native ST7262 telle que fournie par Waveshare (datasheet Rev.1.1)
 * et fréquence PCLK recommandée. 16 MHz fonctionne mais peut induire du
 * tearing ; augmenter à 18-20 MHz si l'alimentation le permet.
 */
#define BOARD_LCD_H_RES             1024
#define BOARD_LCD_V_RES             600
#define BOARD_LCD_PIXEL_CLOCK_HZ    (18 * 1000 * 1000)

#define BOARD_LCD_BK_LIGHT_ON_LEVEL 1
#define BOARD_LCD_BK_LIGHT_OFF_LEVEL !BOARD_LCD_BK_LIGHT_ON_LEVEL

#define BOARD_LCD_PIN_DE            GPIO_NUM_41
#define BOARD_LCD_PIN_PCLK          GPIO_NUM_46
#define BOARD_LCD_PIN_VSYNC         GPIO_NUM_3
#define BOARD_LCD_PIN_HSYNC         GPIO_NUM_42
#define BOARD_LCD_PIN_BACKLIGHT     GPIO_NUM_45
#define BOARD_LCD_PIN_RST           GPIO_NUM_NC

#define BOARD_LCD_PIN_DATA0         GPIO_NUM_0
#define BOARD_LCD_PIN_DATA1         GPIO_NUM_2
#define BOARD_LCD_PIN_DATA2         GPIO_NUM_14
#define BOARD_LCD_PIN_DATA3         GPIO_NUM_15
#define BOARD_LCD_PIN_DATA4         GPIO_NUM_16
#define BOARD_LCD_PIN_DATA5         GPIO_NUM_17
#define BOARD_LCD_PIN_DATA6         GPIO_NUM_18
#define BOARD_LCD_PIN_DATA7         GPIO_NUM_19
#define BOARD_LCD_PIN_DATA8         GPIO_NUM_20
#define BOARD_LCD_PIN_DATA9         GPIO_NUM_21
#define BOARD_LCD_PIN_DATA10        GPIO_NUM_33
#define BOARD_LCD_PIN_DATA11        GPIO_NUM_34
#define BOARD_LCD_PIN_DATA12        GPIO_NUM_35
#define BOARD_LCD_PIN_DATA13        GPIO_NUM_36
#define BOARD_LCD_PIN_DATA14        GPIO_NUM_37
#define BOARD_LCD_PIN_DATA15        GPIO_NUM_38

#define BOARD_LCD_DATA_WIDTH        16

/*
 * Timings issus de la note d'application Waveshare (HSYNC 20/140/160,
 * VSYNC 3/20/12). Ajustez ces constantes si votre dalle nécessite un
 * blanking différent.
 */
#define BOARD_LCD_TIMING_PCLK_ACTIVE_NEG   true
#define BOARD_LCD_TIMING_HPW               20
#define BOARD_LCD_TIMING_HBP               140
#define BOARD_LCD_TIMING_HFP               160
#define BOARD_LCD_TIMING_VPW               3
#define BOARD_LCD_TIMING_VBP               20
#define BOARD_LCD_TIMING_VFP               12

#define BOARD_GT911_SDA_IO         GPIO_NUM_8
#define BOARD_GT911_SCL_IO         GPIO_NUM_9
#define BOARD_GT911_RST_IO         GPIO_NUM_NC
#define BOARD_GT911_IRQ_IO         GPIO_NUM_4
#define BOARD_GT911_I2C_PORT       I2C_NUM_0
#define BOARD_GT911_I2C_FREQ_HZ    400000

/**
 * Active l'alimentation LCD via l'expandeur CH422G (EXIO6) et le
 * rétroéclairage (EXIO2) si votre design le nécessite. Une implémentation
 * faible est fournie dans `app_main.c` et peut être surchargée.
 */
void board_ch422g_enable(void);

