#include "Onewire_DS18B20.h"
#include "LED_iic.h"
// 单总线协议的思考和实现，这里默认已经初始化GPIO，且引脚为PA3，并且在PA3上仅仅***有一个DS....模块
// DS...每一次的通信：初始化 → ROM 命令 → 功能命令 → 数据
// ROM->0xcc 跳过寻址
// CONVERT T（0x44）：启动温度转换
#define WIRE_H() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET)
#define WIRE_L() HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET)
volatile long long DS18B20_launchConvertTick = 0; // DS1820启动转换的时间，外部可访问
_Bool firstrun = 1;
float last_temperature = 0;
uint8_t OneWire_init(TIM_HandleTypeDef *htimx)
{
    _Bool presence;
    WIRE_H(); // 先拉高总线
    delay_us(htimx, 20);
    WIRE_L(); // 拉低总线600us
    delay_us(htimx, 600);
    WIRE_H(); // 拉高，释放总线
    delay_us(htimx, 70);
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET) // 检测存在脉冲
    {
        presence = 1;
    }
    else
    {
        presence = 0;
    }
    delay_us(htimx, 550); // !关键如果这里不加的话会导致stm32没有走完一整个时序就急于发送指令，从而导致LSB,MSB都为0xFF ->temp=-0.0625
    return presence;
}
// 向ds发送八位数据
void Onewire_transmit(TIM_HandleTypeDef *htimx, uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        // 依次传输八位数据,这个温度传感器模块先发低位，例如1000 0000 发送顺序为0000 0001
        uint8_t temp = data & 0x01; // 提取最后一位
        data >>= 1;
        if (temp == 0x00)
        {
            WIRE_L();
            delay_us(htimx, 62);
            WIRE_H();
            delay_us(htimx, 3);
        }
        else if (temp == 0x01)
        {
            WIRE_L();
            delay_us(htimx, 3);
            WIRE_H();
            delay_us(htimx, 62);
        }
    }
}
uint8_t Onewire_read(TIM_HandleTypeDef *htimx)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        WIRE_L();
        delay_us(htimx, 2); // 拉低 ≥1us 启动读时隙
        WIRE_H();           // 释放，交给从机驱动
        delay_us(htimx, 5);

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET)
            data |= (uint8_t)(1u << i); // 第 i 位就放第 i 位
        delay_us(htimx, 55);            // 补足 ≥60us 时隙
    }
    return data;
}
void DS18B20_tempConvert(TIM_HandleTypeDef *htimx)
{
    OneWire_init(htimx);
    Onewire_transmit(htimx, 0xcc);
    Onewire_transmit(htimx, 0x44);
    DS18B20_launchConvertTick = HAL_GetTick();
}
float DS18B20_tempRead(TIM_HandleTypeDef *htimx)
{
    OneWire_init(htimx);
    Onewire_transmit(htimx, 0xcc);
    Onewire_transmit(htimx, 0xBE);     // 读取暂存器
    uint8_t LSB = Onewire_read(htimx); // 先读到低字节
    uint8_t MSB = Onewire_read(htimx); // 后读到高字节

    int16_t raw = (MSB << 8) | LSB;
    float temp = raw * 0.0625f;
    return temp;
}
float DS18B20_read(TIM_HandleTypeDef *htimx)
{
    if (firstrun)
    {
        last_temperature = DS18B20_tempRead(htimx);
        firstrun = 0;
        return last_temperature;
    }
    else
    {
        float current = DS18B20_tempRead(htimx);
        if (current - last_temperature > -3 && current - last_temperature < 3 && current < 85.0 && current > -30)
        // 软件消除抖策略，条件限制在abs(δ)<3，且温度在合理测量值内
        {
            last_temperature = current;
            return current;
        }
        else
        {
            return last_temperature;
        }
    }
}