#ifndef TB6612FNG_H
#define TB6612FNG_H

#include "main.h"
#include <stdint.h>

/* 单个 TB6612FNG 电机通道：把一路电机的方向/调速引脚装进一个结构体，
   用法与 SingleBtn 类似——先 Init 绑定引脚，再用 SingleMotor_xxx 操作。
   STBY 是整颗芯片共用的使能脚，不随单路电机变化，已放在 TB6612FNG.c 里做常量。
   GPIO 由调用方预先初始化好（默认已经初始化过），本模块不碰 GPIO 配置。 */
typedef struct
{
    GPIO_TypeDef *ain1_area;    /* 方向 AIN1 端口 */
    uint16_t      ain1_pin;     /* 方向 AIN1 引脚号 */
    GPIO_TypeDef *ain2_area;    /* 方向 AIN2 端口 */
    uint16_t      ain2_pin;     /* 方向 AIN2 引脚号 */
    GPIO_TypeDef *pwm_area;     /* 调速 PWMA 端口 */
    uint16_t      pwm_pin;      /* 调速 PWMA 引脚号 */
} SingleMotor;

void SingleMotor_Init(SingleMotor *self,
                      GPIO_TypeDef *ain1_area, uint16_t ain1_pin,
                      GPIO_TypeDef *ain2_area, uint16_t ain2_pin,
                      GPIO_TypeDef *pwm_area,  uint16_t pwm_pin);
void SingleMotor_Forward(SingleMotor *self);
void SingleMotor_Reverse(SingleMotor *self);
void SingleMotor_Brake(SingleMotor *self);
void SingleMotor_Stop(SingleMotor *self);
void SingleMotor_Standby(void);
void SingleMotor_Speed(SingleMotor *self, int16_t speed);

#endif
