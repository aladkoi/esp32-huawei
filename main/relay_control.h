///это реле включения передней фары и подсветки
#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include "driver/gpio.h"

// Определяем контакт для реле
#define RELAY_PIN GPIO_NUM_18  // включение выключение света

// Глобальная переменная для хранения состояния реле
extern volatile bool relay_state; // true - включено, false - выключено

// Функции для управления реле
void configure_gpio(void);
void set_relay_state();

#endif // RELAY_CONTROL_H