///это реле включения фары тормоза
#ifndef LED_BREAK_H
#define LED_BREAK_H

#include <driver/gpio.h>
#include <driver/ledc.h>

// Определение пина для стоп-сигнала
#define BREAK_LIGHT_PIN GPIO_NUM_5 // Можно заменить на ваш пин

#define LEVEL_PIN1 27 /// низкая скорость
#define LEVEL_PIN2 13 /// средняя скорость

// Объявления функций
void initBreakLight(void);
void DoLight(int data);
void initSpeedBut(void); 

#endif // LED_BREAK_H