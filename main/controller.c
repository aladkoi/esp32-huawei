#include "controller.h"
//#include "esp_log.h"


// Инициализация состояния
ControllerState_t state = {
    .name_cont = "",
    .Vbatt = 0.0f,
    .volt_bl = 0,
    .break_volt_bl = 0,
    .start_level = 0,
    .current_rpm = 0,
    .current_level = 0,
    .current_level_speed = 0,
    .current_level_rpm = 0,
    .croiuse_level = -1,
    .break_croiuse_level = -1,
    .controllerBrake = false,
    .currentSpeed = 0,
    .cr = -1,
    .current_amper = 3, // Начальный индекс шага скорости
    .target_erpm = 0,
    .crouise_on = false,
    .isn = true,
    .numberCrouise = 255,
    .stop_from_mobile = false,
    .break_long = false,
    .speed_up = false,
    .addspeed = false,
    .break_level = 1,
    .change_event=0,
    .volt_add_speed=0,
    .pulse_count = 0
};

//volatile float level_crouise[6] = {0, 21.4, 33.2, 46.0, 58.4, 95.0};
volatile int rpm_crouise[6] = {132, 160, 192, 220, 240,255};
const int len_crouise = 5; // Размер массива круиза минус 1


SemaphoreHandle_t state_mutex = NULL;

void controller_init(void) {
    if (state_mutex == NULL) {
        state_mutex = xSemaphoreCreateMutex();
        if (state_mutex == NULL) {
            //printf("Failed to create state mutex");
        } else {
            //printf("State mutex initialized");
        }
    }
}

// Функции управления состоянием
void stop_Speed(bool status) {
    if (status && !state.controllerBrake) {
        state.controllerBrake = true;
        if (!state.isn) { // Умный тормоз
            state.current_level = 0;
            state.current_rpm = 0;
        }
        state.volt_bl = 0;
        state.numberCrouise = 255;
        state.crouise_on = false;
        state.croiuse_level = -1;
        state.cr = -1;
        //getDuty(0.0);
}
}

void setCurrentLevel(void) {
    //printf("setCurrentLevel\n");
    state.current_level_speed = state.volt_bl;
    state.current_level = state.volt_bl;  
}

int get_level(bool forward) {
int cr = -1;
    if (forward && state.volt_bl >= rpm_crouise[len_crouise]) {
        cr = len_crouise;
    } else if (forward) {
        for (int i = 0; i <= len_crouise; i++) {
            if (rpm_crouise[i] > state.volt_bl) {
                cr = i;
                break;
            }
        }
    } else {
        for (int i = len_crouise; i >= 0; i--) {
            if (state.volt_bl > rpm_crouise[i]) {
                cr = i;
                break;
            }
        }
    }
    if (!forward && cr == -1) cr = -1;
    else if (cr == -1) cr = 0;
return cr;
}

void select_level(int crouise) {
    printf("select_level=%d\n",crouise);
    printf("state.volt_bl=%d\n",state.volt_bl);      
    if (state.volt_bl == 0 || (crouise > -1 && crouise < (len_crouise + 1))) {
    //   printf("state.speed_up=");
    //   printf(state.speed_up);  
    //   printf("\n");
    //   if (!state.speed_up) {
          state.volt_bl = rpm_crouise[crouise];
          state.currentSpeed = rpm_crouise[crouise];
          printf("state.volt_bl1=%d\n",state.volt_bl);      
    //   } else {
          
    //       state.volt_bl = rpm_crouise[crouise];
    //     //   if (crouise > 0) {
    //     //       state.addspeed = true;
    //     //   }
    //   }
      state.controllerBrake = false;
  }  
}

float start_crouise(void) {
float result = 0.0f;
  state.croiuse_level = 0;
  select_level(state.croiuse_level);
  if (state.volt_bl <= state.start_level) state.volt_bl = rpm_crouise[0];
  if (state.volt_bl > 20800) state.volt_bl = 20800;
  //getDuty(state.volt_bl);
  state.current_level = state.volt_bl;
  result = state.volt_bl;
return result;
}


void AddSpeed(void) {
  if (state.break_level == 1 && state.volt_bl < rpm_crouise[len_crouise]) {
      if (state.volt_bl <= state.start_level || state.volt_bl == 0) {
          state.volt_bl = start_crouise();
      } else {
          state.volt_bl = getStep(true, state.volt_bl);
          if (state.volt_bl > rpm_crouise[len_crouise]) state.volt_bl = rpm_crouise[len_crouise];
      }
  }
  //printf("AddSpeed1\n");
  setCurrentLevel();
}

void setCrouise(int crouise) {
 if (state.cr == crouise) {            
      return;
  }
  printf("setCrouise=%d\n",crouise);
  state.crouise_on = true;
  state.cr = crouise;
  state.numberCrouise = crouise;
  select_level(crouise);
  //printf("setCrouise1\n");
  setCurrentLevel();
}


void BUTTON_PRESS_REPEAT_ADD(void){
    state.cr=-1;
    if (state.croiuse_level < len_crouise) {
        state.croiuse_level = get_level(true);
        if (state.croiuse_level < 1) state.croiuse_level = 0;
        if (state.croiuse_level == 0) state.croiuse_level = 1;
        state.crouise_on = true;
        state.speed_up = true;
        setCrouise(state.croiuse_level);
        state.change_event=state.croiuse_level+1;
    }
}

void BUTTON_SINGLE_CLICK_ADD(void){
    printf("BUTTON_SINGLE_CLICK_ADD\n");
    printf("state.croiuse_level =%d\n",state.croiuse_level);
    state.break_level=1;
    if (state.croiuse_level <0) {
        state.croiuse_level = 0;
        state.cr=-1;
        state.crouise_on = true;
        state.speed_up = true;
        state.change_event=1;
        printf("Запускаем первую скорость");
        setCrouise(state.croiuse_level);
    } else {
        AddSpeed();
        state.currentSpeed = 0;
    }
}

void BUTTON_SINGLE_CLICK_DEC(void){
    state.crouise_on = false;
    if (state.break_level == 1 && state.volt_bl >= state.start_level)
        state.volt_bl = getStep(false, state.volt_bl);
    if (state.volt_bl < 0) {
        state.volt_bl = 0;
        state.change_event=20;
    }
    setCurrentLevel();
}

void BUTTON_PRESS_REPEAT_DEC(void){
    state.croiuse_level = get_level(false);
    state.change_event=state.croiuse_level+1;
    printf("state.croiuse_level=%d\n",state.croiuse_level);
    if (state.croiuse_level < 0) {
        state.volt_bl = 0;
        setCurrentLevel();
        state.change_event=20;
    }
    if (state.croiuse_level > len_crouise) state.croiuse_level = len_crouise;
    if (state.croiuse_level != -1) {
        state.crouise_on = true;
        state.speed_up = false;
        setCrouise(state.croiuse_level);
    } else {
        state.stop_from_mobile = true;
    }
}
