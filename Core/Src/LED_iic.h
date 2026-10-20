/* Boost led using iic  --  软件 I2C + SSD1306 OLED */

#ifndef LED_IIC_H
#define LED_IIC_H

#include <stdint.h>
#include "main.h"

/* ---------------- 软件 I2C 底层 ---------------- */
void    delay_us(TIM_HandleTypeDef *htimx, unsigned short int time);
void    iic_initialize(void);
void    iic_startsig(TIM_HandleTypeDef *htimx);
void    iic_stopsig(TIM_HandleTypeDef *htimx);
uint8_t ACKs_judegment(TIM_HandleTypeDef *htimx);
uint8_t iic_transmit(TIM_HandleTypeDef *htimx, uint8_t databuffer);
uint8_t iic_receive(TIM_HandleTypeDef *htimx, _Bool askmore);

/* ---------------- OLED (SSD1306) ---------------- */
uint8_t OLED_init(TIM_HandleTypeDef *htimx);
void    OLED_SetCursor(TIM_HandleTypeDef *htimx, uint8_t Y, uint8_t X);
void    OLED_Clear(TIM_HandleTypeDef *htimx);

/* ---------------- 显存与字符 ---------------- */
extern uint8_t OLED_Vram[8][128];

void    OLED_ClearVram(void);
void    OLED_DrawChar(uint8_t line, uint8_t col, char ch);
uint8_t OLED_ShowString(uint8_t line, uint8_t col, const char *str);
void    OLED_Refresh(TIM_HandleTypeDef *htimx);

#endif /* LED_IIC_H */
