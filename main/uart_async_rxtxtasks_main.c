/* UART asynchronous example with state structure
   This example code is in the Public Domain (or CC0 licensed, at your option.)
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include <math.h>
#include "esp_intr_alloc.h"
#include "esp_pm.h"
#include "iot_button.h"
#include "esp_sleep.h"
#include <stdio.h>
#include <inttypes.h>
#include "button_gpio.h"
//#include "crc.h"  // Подсчет по duty
//#include "led_break.h"
#include "ble_spp.c"
#include "driver/dac_oneshot.h"
//#include "esp_vfs_dev.h"
#include "controller.h"
#define TAG "BUTTON"
#define BUTTON_NUM1 22 // Правая кнопка
#define BUTTON_NUM2 23 // Левая кнопка
#define analogOutPin 26  // GPIO25 для аналогового выхода на контроллер ручки газа



#define CROISE_PIN 33 /// включение круиза

//const int start_level=112;

#define BUTTON_ACTIVE_LEVEL 0
#define BLUE_LED_PIN GPIO_NUM_2
#define POWER_PIN 4
#define BREAK_PIN 21 // Вход стоп-сигнала
//#define SPEED_PIN 19 /// датчик скорости
#define PIN 14 /// включение преобразоватля

 
//#define UART_NUM UART_NUM_1
//#define BUF_SIZE 2048
//#define RX_TIMEOUT (1000 / portTICK_PERIOD_MS)
// #define TXD_PIN (GPIO_NUM_1)      // TXD0
// #define RXD_PIN (GPIO_NUM_3)      // RXD0
// #define UART_PORT UART_NUM_0   
dac_oneshot_handle_t dac_handle;

// Очередь UART
//static QueueHandle_t uart_queue;



// Названия событий кнопок
static const char *button_event_names[] = {
    [BUTTON_PRESS_DOWN] = "BUTTON_PRESS_DOWN",
    [BUTTON_PRESS_UP] = "BUTTON_PRESS_UP",
    [BUTTON_PRESS_REPEAT] = "BUTTON_PRESS_REPEAT",
    [BUTTON_SINGLE_CLICK] = "BUTTON_SINGLE_CLICK",
    [BUTTON_DOUBLE_CLICK] = "BUTTON_DOUBLE_CLICK",
    [BUTTON_LONG_PRESS_START] = "BUTTON_LONG_PRESS_START",
    [BUTTON_LONG_PRESS_HOLD] = "BUTTON_LONG_PRESS_HOLD",
    [BUTTON_LONG_PRESS_UP] = "BUTTON_LONG_PRESS_UP",
};



// static void do_add_speed(void* parameter) {
//     const uint32_t DELAY_MS = 450;
//     //const uint32_t CHECK_INTERVAL_MS = 450;
//     //TickType_t last_check_time;
//     int last_index = 0;

//     while (1) {
//         if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
//             bool local_addspeed = state.addspeed;
//             if (local_addspeed){
//                 state.break_level=1;
//                 //printf("state.break_level333=%d",state.break_level=1); 
//             }
//             xSemaphoreGive(state_mutex);
            
//             if (local_addspeed) {
//                 //last_check_time = xTaskGetTickCount();
//                 if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
//                     last_index = find_index_linear(state.volt_bl);
//                     if (last_index < 0) last_index = 0;
//                     state.target_erpm = erpm_step[state.current_amper];
//                     xSemaphoreGive(state_mutex);
//                 }

//                 while (state.break_level == 1 && local_addspeed && state.current_amper <= last_index) {
//                     if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
//                         state.target_erpm = erpm_step[state.current_amper];
//                         state.current_amper++;
//                         local_addspeed = state.addspeed;
//                         xSemaphoreGive(state_mutex);
//                     }
//                     vTaskDelay(pdMS_TO_TICKS(200));
//                 }

//                 if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
//                     state.addspeed = false;
//                     local_addspeed = false;
//                     xSemaphoreGive(state_mutex);
//                 }
//             }
//         }
//         vTaskDelay(pdMS_TO_TICKS(DELAY_MS));
//     }
//     vTaskDelete(NULL);
// }



// Обработчики событий кнопок
static void button_event_break(void *arg, void *data) {
    printf("button_event_break\n");
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        //printf("do button_event_break\n");
        state.break_long = false;
        state.break_level = 0;
        gpio_set_level(BLUE_LED_PIN, 1);
        DoLight(128);
        state.controllerBrake = false;
        state.break_volt_bl = state.volt_bl;
        state.break_croiuse_level = state.croiuse_level;
        state.addspeed = false;
        stop_Speed(true);
        setCurrentLevel();
        state.change_event=20;
        xSemaphoreGive(state_mutex);
    }
}

static void button_event_break_long(void *arg, void *data) { /// сработал долгий тормоз
    printf("button_event_break_long\n");
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        state.break_long = true;
        state.addspeed = false;
        state.current_amper = 3; // Сбрасываем в исходное состояние
        xSemaphoreGive(state_mutex);
    }
    printf("button_event_break_long_end--------\n");
}

static void button_event_break_end(void *arg, void *data) {
    printf("button_event_break_end\n");
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        gpio_set_level(BLUE_LED_PIN, 0);
        if (!state.break_long) { /// не было длинного стоп
            printf("state.break_volt_bl=%d",state.break_volt_bl);            
            state.volt_bl = state.break_volt_bl;
            if (state.volt_bl > 0) {
                state.croiuse_level = get_level(true) - 1;
                if (state.croiuse_level < 1) state.croiuse_level = 0;
                state.cr = -1;
                state.crouise_on = true;
                state.speed_up = true;
                /// для данного контроллера без использования add_speed  state.break_level = 1;
                state.break_level = 1;
                state.volt_bl = 0;
                setCrouise(state.croiuse_level);
                state.change_event=state.croiuse_level+1;
            }
        }
        else state.break_level = 1;
        state.break_volt_bl = 0;
        state.break_long = false;
        DoLight(0);
        xSemaphoreGive(state_mutex);
    }
     //printf("button_event_break_end_end--------\n");
     //printf("state.break_level=%d\n",state.break_level);
}

static void button_event_cb1(void *arg, void *data) {
    printf("button_event_cb1\n");
    button_handle_t btn_handle = (button_handle_t)arg;
    button_event_t event = iot_button_get_event(btn_handle);
    if (event >= sizeof(button_event_names) / sizeof(button_event_names[0]) || !button_event_names[event]) return;

    const char *event_name = button_event_names[event];
    printf(event_name);
    printf("\n");
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {  
        if (strcmp(event_name, "BUTTON_SINGLE_CLICK") == 0) {
            printf("button_event_single\n");
            printf("state.break_level=%d\n",state.break_level);
            printf("state.croiuse_level=%d\n",state.croiuse_level);
            
            BUTTON_SINGLE_CLICK_ADD();
        } else if (strcmp(event_name, "BUTTON_PRESS_REPEAT") == 0) {
            BUTTON_PRESS_REPEAT_ADD();          
        } else if (strcmp(event_name, "BUTTON_LONG_PRESS_START") == 0) {
            //AddSpeed();
            state.change_event=21;
            set_relay_state();
        }
        xSemaphoreGive(state_mutex);
    }
    printf("button_event_cb1_end\n");
}

static void button_event_cb2(void *arg, void *data) {
    printf("button_event_cb2\n");
    button_handle_t btn_handle = (button_handle_t)arg;
    button_event_t event = iot_button_get_event(btn_handle);
    if (event >= sizeof(button_event_names) / sizeof(button_event_names[0]) || !button_event_names[event]) return;

    const char *event_name = button_event_names[event];
    if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        state.addspeed = false;
        if (strcmp(event_name, "BUTTON_SINGLE_CLICK") == 0) {
            BUTTON_SINGLE_CLICK_DEC();
          
        } else if (strcmp(event_name, "BUTTON_PRESS_REPEAT") == 0) {
            BUTTON_PRESS_REPEAT_DEC();
        } else if (strcmp(event_name, "BUTTON_LONG_PRESS_START") == 0) {
            // state.crouise_on = false;
            // if (state.break_level == 1 && state.volt_bl >= state.start_level)
            //     state.volt_bl = getStep(false, state.volt_bl);
            // if (state.volt_bl < 0) state.volt_bl = 0;
            // setCurrentLevel();
            pulse_relay();
        }
        xSemaphoreGive(state_mutex);
    }
    printf("button_event_cb2_end\n");
}

// Инициализация кнопок
static void button_init_break(uint32_t button_num) {
    button_config_t btn_cfg = {
        .long_press_time = 3000,
        .short_press_time = 50,
    };
    button_gpio_config_t gpio_cfg = {
        .gpio_num = button_num,
        .active_level = BUTTON_ACTIVE_LEVEL,
    };
    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    assert(ret == ESP_OK);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_DOWN, NULL, button_event_break, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_event_break_long, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_END, NULL, button_event_break_end, NULL);
    ESP_ERROR_CHECK(ret);
}

static void button_init1(uint32_t button_num) {
    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = button_num,
        .active_level = BUTTON_ACTIVE_LEVEL,
    };
    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    assert(ret == ESP_OK);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_REPEAT, NULL, button_event_cb1, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb1, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb1, NULL);
    ESP_ERROR_CHECK(ret);
}

static void button_init2(uint32_t button_num) {
    button_config_t btn_cfg = {0};
    button_gpio_config_t gpio_cfg = {
        .gpio_num = button_num,
        .active_level = BUTTON_ACTIVE_LEVEL,
    };
    button_handle_t btn;
    esp_err_t ret = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg, &btn);
    assert(ret == ESP_OK);
    ret |= iot_button_register_cb(btn, BUTTON_PRESS_REPEAT, NULL, button_event_cb2, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_SINGLE_CLICK, NULL, button_event_cb2, NULL);
    ret |= iot_button_register_cb(btn, BUTTON_LONG_PRESS_START, NULL, button_event_cb2, NULL);
    ESP_ERROR_CHECK(ret);
}

static void init_button(void) {
    button_init1(BUTTON_NUM1);
    button_init2(BUTTON_NUM2);
    button_init_break(BREAK_PIN);
}





static void loop_controller(void* parameter) {
    printf("Start loop_controller\n");
    int my_voltbl = 0;
    uint8_t speed_data=0;
   TickType_t wait_time = 2000 / portTICK_PERIOD_MS;

    while (1) {
        if (xSemaphoreTake(state_mutex, portMAX_DELAY) == pdTRUE) {
        
                wait_time = 250 / portTICK_PERIOD_MS;
       
                        
                        if (state.break_level == 0) {
                            my_voltbl = 0;
                            /// отправляем 
                            //sendBufferToController(stop_data, sizeof(stop_data));
                            speed_data=(uint8_t)(0);
                            printf("speed_data=%d\n",speed_data);
                            ESP_ERROR_CHECK(dac_oneshot_output_voltage(dac_handle,speed_data));
                        } else {
                            if (state.volt_bl == 0 && state.current_level == 0) {
                                speed_data=(uint8_t)0;
                                my_voltbl =0;
                            }    
                            // else if (state.addspeed) {
                            //     speed_data=(uint8_t)getSpeed(state.target_erpm);
                            //     ESP_ERROR_CHECK(dac_continuous_write_cyclically(dac_handle,&speed_data,1,NULL));
                            //      state.croiuse_level = get_level(true) - 1;
                            //     if (state.croiuse_level < 1) state.croiuse_level = 0;
                            // } 
                            else {                                
                                if (state.volt_bl == 0 && state.current_level > 0) state.volt_bl = state.current_level;
                                if (state.volt_bl > rpm_crouise[len_crouise]) state.volt_bl = rpm_crouise[len_crouise];
                                if (my_voltbl != state.volt_bl) {
                                    my_voltbl = state.volt_bl;                                    
                                    speed_data=(uint8_t)getSpeed(state.volt_bl);
                                    setCurrentLevel();
                                }
                            }
                            //printf("speed_data=%d\n",speed_data);
                            ESP_ERROR_CHECK(dac_oneshot_output_voltage(dac_handle,speed_data));
                        }
                    

                   
            
            xSemaphoreGive(state_mutex);
        }
        vTaskDelay(wait_time);
    }
}

static void configure_led(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BLUE_LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// // Функция-заглушка, которая игнорирует вывод
static int log_dummy(const char *fmt, va_list args) {
    return 0; // Просто возвращаем 0, ничего не выводим
}
static int null_output(struct _reent *r, void *fd, const char *ptr, int len) {
    (void)r;   // Подавляем предупреждение о неиспользуемой переменной
    (void)fd;  // То же самое для fd
    return len; // Возвращаем длину, как будто вывод успешен
}
void app_main(void) {
    esp_log_set_vprintf(log_dummy); // Устанавливаем заглушку на логи esp

    setvbuf(stdout, NULL, _IONBF, 0); // Отключаем буферизацию  
    stdout->_write = null_output;     // отключает вывод логов по printf


    controller_init();
    init_button();
    configure_led();
    //getDuty(0.0);
    initBreakLight();
    initSpeedBut();
    configure_gpio();
    configure_pulse_gpio();
    gpio_set_direction(POWER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_PIN, 0);

    dac_oneshot_config_t oneshot_cfg = {
        .chan_id = DAC_CHAN_1  // GPIO25
    };
    esp_err_t ret = dac_oneshot_new_channel(&oneshot_cfg, &dac_handle);
    if (ret != ESP_OK) {
        printf("Ошибка инициализации DAC: %s\n", esp_err_to_name(ret));
        return;
    }
    

    
    xTaskCreatePinnedToCore(loop_controller, "Loop_controller", 4096, NULL, 10, NULL, 1);
    start_ble();
    //xTaskCreatePinnedToCore(do_add_speed, "Do add speed", 10000, NULL, 10, NULL, 0);
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN),    // Маска для выбора GPIO14
        .mode = GPIO_MODE_OUTPUT,         // Режим выхода
        .pull_up_en = GPIO_PULLUP_DISABLE,  // Отключение подтяжки вверх
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Отключение подтяжки вниз
        .intr_type = GPIO_INTR_DISABLE    // Отключение прерываний
    };

    // Применение конфигурации
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // Установка высокого уровня (1) на GPIO14
    ESP_ERROR_CHECK(gpio_set_level(PIN, 1));

}