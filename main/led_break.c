#include "led_break.h"
//#include <stdio.h> // Для возможного вывода отладочной информации

void initBreakLight(void) {
    // Настройка LEDC таймера
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BREAK_LIGHT_PIN),    // Маска для GPIO 33
        .mode = GPIO_MODE_OUTPUT,                     // Режим вывода
        .pull_up_en = GPIO_PULLUP_DISABLE,             // Включаем подтяжку вверх
        .pull_down_en = GPIO_PULLDOWN_ENABLE,        // Отключаем подтяжку вниз
        .intr_type = GPIO_INTR_DISABLE                // Отключаем прерывания
    };
    gpio_config(&io_conf);

    // Устанавливаем начальное состояние (HIGH благодаря pull-up, реле выключено)
    gpio_set_level(BREAK_LIGHT_PIN, 1);
   
}

void initSpeedBut(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LEVEL_PIN1),    // Маска для GPIO 33
        .mode = GPIO_MODE_OUTPUT,                     // Режим вывода
        .pull_up_en = GPIO_PULLUP_DISABLE,             // Включаем подтяжку вверх
        .pull_down_en = GPIO_PULLDOWN_ENABLE,        // Отключаем подтяжку вниз
        .intr_type = GPIO_INTR_DISABLE                // Отключаем прерывания
    };
    gpio_config(&io_conf);
    // Устанавливаем начальное состояние (HIGH благодаря pull-up, реле выключено)
    gpio_set_level(LEVEL_PIN1, 1);

    gpio_config_t io_conf1 = {
        .pin_bit_mask = (1ULL << LEVEL_PIN2),    // Маска для GPIO 33
        .mode = GPIO_MODE_OUTPUT,                     // Режим вывода
        .pull_up_en = GPIO_PULLUP_DISABLE,             // Включаем подтяжку вверх
        .pull_down_en = GPIO_PULLDOWN_ENABLE,        // Отключаем подтяжку вниз
        .intr_type = GPIO_INTR_DISABLE                // Отключаем прерывания
    };
    gpio_config(&io_conf1);
    // Устанавливаем начальное состояние (HIGH благодаря pull-up, реле выключено)
    gpio_set_level(LEVEL_PIN2, 1);   
}


void DoLight(int data) {
    if (data==0) gpio_set_level(BREAK_LIGHT_PIN, 1); // LOW - включаем реле (активный LOW)
    else gpio_set_level(BREAK_LIGHT_PIN, 0); // LOW - включаем реле (активный LOW)
}