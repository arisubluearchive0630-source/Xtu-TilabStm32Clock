#include "TB6612FNG.h"
#include "LED_iic.h"
// 我的意志也会高耸、屹立、崩落，那我也是山吗？

/* ================= 调速 PWM 参数（写死在本文件） ================= */
#define MOTOR_PWM_MAX 255u /* 8bit 占空比分辨率：0~255 */
/* 调速延时用的定时器：写死为 htim3。CubeMX 只把 htim3 定义在 main.c、
   没在 main.h 里导出，所以这里自己 extern 声明；换定时器只改这一行。 */
extern TIM_HandleTypeDef htim3;
#define MOTOR_DELAY_TIM (&htim3)

/* ================= STBY 常量（整颗芯片共用，不属于某一路电机） ================= */
/* 用 const 占位，按实际接线修改；GPIO 仍需调用方预先初始化为推挽输出。 */
static GPIO_TypeDef *const STBY_AREA = GPIOC; /* STBY 端口常量 */
static const uint16_t STBY_PIN = GPIO_PIN_13; /* STBY 引脚号常量 */

// 初始化：绑定一路电机的引脚，STBY 长时间置高（PINSET，HAL 库）使能驱动
void SingleMotor_Init(SingleMotor *self,
                      GPIO_TypeDef *ain1_area, uint16_t ain1_pin,
                      GPIO_TypeDef *ain2_area, uint16_t ain2_pin,
                      GPIO_TypeDef *pwm_area, uint16_t pwm_pin)
{
    self->ain1_area = ain1_area;
    self->ain1_pin = ain1_pin;
    self->ain2_area = ain2_area;
    self->ain2_pin = ain2_pin;
    self->pwm_area = pwm_area;
    self->pwm_pin = pwm_pin;

    HAL_GPIO_WritePin(STBY_AREA, STBY_PIN, GPIO_PIN_SET); /* 使能，长时间保持高 */
    HAL_GPIO_WritePin(self->ain1_area, self->ain1_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(self->ain2_area, self->ain2_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(self->pwm_area, self->pwm_pin, GPIO_PIN_RESET);
}

// 正转（全速）：AIN1=1, AIN2=0, PWMA=1
void SingleMotor_Forward(SingleMotor *self)
{
    HAL_GPIO_WritePin(self->ain1_area, self->ain1_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(self->ain2_area, self->ain2_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(self->pwm_area, self->pwm_pin, GPIO_PIN_SET);
}

// 反转（全速）：AIN1=0, AIN2=1, PWMA=1
void SingleMotor_Reverse(SingleMotor *self)
{
    HAL_GPIO_WritePin(self->ain1_area, self->ain1_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(self->ain2_area, self->ain2_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(self->pwm_area, self->pwm_pin, GPIO_PIN_SET);
}

// 短路制动：AIN1=1, AIN2=1, PWMA=0
void SingleMotor_Brake(SingleMotor *self)
{
    HAL_GPIO_WritePin(self->ain1_area, self->ain1_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(self->ain2_area, self->ain2_pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(self->pwm_area, self->pwm_pin, GPIO_PIN_RESET);
}

// 滑行停止：AIN1=0, AIN2=0, PWMA=0
void SingleMotor_Stop(SingleMotor *self)
{
    HAL_GPIO_WritePin(self->ain1_area, self->ain1_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(self->ain2_area, self->ain2_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(self->pwm_area, self->pwm_pin, GPIO_PIN_RESET);
}

// 待机：STBY=0，关闭所有输出（整颗芯片共用的使能脚，无需传电机实例）
void SingleMotor_Standby(void)
{
    HAL_GPIO_WritePin(STBY_AREA, STBY_PIN, GPIO_PIN_RESET);
}

// 调速：speed -255~255，正=正转、负=反转、0=停止；软件 PWM 由 delay_us 产生
// PWM 周期/分辨率写死在本文件（MOTOR_PWM_MAX=255，约 255us / 3.9kHz），
// 每调用一次输出一个 PWM 周期，连续调速请在主循环中反复调用
void SingleMotor_Speed(SingleMotor *self, int16_t speed)
{
    uint8_t duty;

    if (speed > 255)
    {
        speed = 255;
    }
    if (speed < -255)
    {
        speed = -255;
    }
    if (speed == 0)
    {
        SingleMotor_Stop(self);
        return;
    }

    if (speed > 0)
    {
        HAL_GPIO_WritePin(self->ain1_area, self->ain1_pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(self->ain2_area, self->ain2_pin, GPIO_PIN_RESET);
        duty = (uint8_t)speed;
    }
    else
    {
        HAL_GPIO_WritePin(self->ain1_area, self->ain1_pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(self->ain2_area, self->ain2_pin, GPIO_PIN_SET);
        duty = (uint8_t)(-speed);
    }

    // 软件 PWM：高电平 duty us，低电平 (255-duty) us
    HAL_GPIO_WritePin(self->pwm_area, self->pwm_pin, GPIO_PIN_SET);
    delay_us(MOTOR_DELAY_TIM, duty);
    HAL_GPIO_WritePin(self->pwm_area, self->pwm_pin, GPIO_PIN_RESET);
    delay_us(MOTOR_DELAY_TIM, (uint16_t)(MOTOR_PWM_MAX - duty));
}
