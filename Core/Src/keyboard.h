#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

#define KEY_DEBOUNCE_MS   20u

typedef struct
{
    GPIO_TypeDef *area;    
    uint16_t      key;
    bool          status;
    bool          flag;
} SingleBtn;

void SingleBtn_Init(SingleBtn *self, GPIO_TypeDef *port, uint16_t key);
void GetKeyboardStatus(SingleBtn *buttonArray, uint16_t length);

#endif