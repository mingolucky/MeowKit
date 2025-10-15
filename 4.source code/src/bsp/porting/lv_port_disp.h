/**
 * @file lv_port_disp_templ.h
 *
 */

/*Copy this file as "lv_port_disp.h" and set this value to "1" to enable content*/
#if 1

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H


/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl.h"
#endif
#include "display.hpp"
#include <PCA9557.h>
/*********************
 *      DEFINES
 *********************/

#define LVGL_USE_PSRAM 1

#define LVGL_LCD_HORIZON 1

#ifdef LVGL_LCD_HORIZON
    #define MY_DISP_HOR_RES 320
    #define MY_DISP_VER_RES 240
    #define LV_VER_RES_MAX  320
#else
#ifdef LVGL_LCD_PORTRAIT
    #define MY_DISP_HOR_RES     240
    #define MY_DISP_VER_RES     280
    #define LV_VER_RES_MAX      240
#endif
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/* Initialize low level display driver */
void lv_port_disp_init(Meow_LGFX* lcd);

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void);

/**********************
 *      MACROS
 **********************/


#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*Disable/Enable content*/
