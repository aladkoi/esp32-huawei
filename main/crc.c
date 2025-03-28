#include "crc.h"




volatile int erpm_step[32] = {
    0,80, 84, 88, 92, 96, 100, 104, 108,
    112, 116, 120, 124, 128, 132, 136, 140,
    144, 148, 152, 156, 160, 164, 168, 172,
    176, 180, 184, 188, 192, 196, 200
};





int find_index_linear(int target) {
    for (int i = 0; i < sizeof(erpm_step) / sizeof(erpm_step[0]); i++) {
        if (erpm_step[i] == target) {
            return i;
        }
    }
    return -1;
}

int getStep(bool forward, int value) {
    int index = find_index_linear(value);
    if (index < 0) return 0;
    if (forward) {
        index++;
        if (index >= sizeof(erpm_step) / sizeof(erpm_step[0])) index--;
    } else {
        index--;
        if (index < 0) index = 0;
    }
    return erpm_step[index];
}

int getCurrentIndexSpeed(int value) {
    for (int i = 0; i < sizeof(erpm_step) / sizeof(erpm_step[0]); i++) {
        if (erpm_step[i] >= value) {
            i++;
            if (i >= sizeof(erpm_step) / sizeof(erpm_step[0])) i--;
            return i;
        }
    }
    return -1;
}

int getCurrentDecIndexSpeed(int value) {
    for (int i = 0; i < sizeof(erpm_step) / sizeof(erpm_step[0]); i++) {
        if (erpm_step[i] <= value) {
            return i;
        }
    }
    return -1;
}

int getCurrentSpeed(int value) {
    for (int i = 0; i < sizeof(erpm_step) / sizeof(erpm_step[0]); i++) {
        if (erpm_step[i] >= value) {
            if (i >= sizeof(erpm_step) / sizeof(erpm_step[0])) i--;
            return erpm_step[i];
        }
    }
    return -1;
}

int getSpeed(int number) {
    int index = find_index_linear(number);   
    if (index != -1)return erpm_step[index];
    else  return 0;
}

