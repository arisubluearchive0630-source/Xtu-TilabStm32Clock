#include "keyboard.h"

void SingleBtn_Init(SingleBtn *self, GPIO_TypeDef *port, uint16_t key)
{
    self->area   = port;
    self->key    = key;
    self->status = (HAL_GPIO_ReadPin(port, key) == GPIO_PIN_SET);
    self->flag   = false;
}

void GetKeyboardStatus(SingleBtn *buttonArray, uint16_t length)
{
    uint32_t start = HAL_GetTick();

    for (uint16_t i = 0; i < length; i++) {
        buttonArray[i].status = (HAL_GPIO_ReadPin(buttonArray[i].area,
                                                  buttonArray[i].key) == GPIO_PIN_SET);
    }

    while ((HAL_GetTick() - start) < KEY_DEBOUNCE_MS) {
        for (uint16_t i = 0; i < length; i++) {
            if ((HAL_GPIO_ReadPin(buttonArray[i].area,
                                  buttonArray[i].key) == GPIO_PIN_SET) != buttonArray[i].status) {
                buttonArray[i].status = false;
            }
        }
    }
}