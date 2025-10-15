/**
 * @file 
 * @author
 * @brief 
 * @version 
 * @date 
 * 
 * @copyright 
 * 
 */
#pragma once
#include "Button/Button.h"

/* Add your button with this */
#define Button_Add(name, pin) __Button name = __Button(pin)

class Button {
    private:
    public:
    
        #if HAL_A>= 0
        Button_Add(A, HAL_A);
        #endif

        #if HAL_B>= 0
        Button_Add(B, HAL_B);
        #endif

        #if HAL_PIN_PWR_ON >= 0
        Button_Add(PWR_ON, HAL_PIN_PWR_ON);
        #endif

};
