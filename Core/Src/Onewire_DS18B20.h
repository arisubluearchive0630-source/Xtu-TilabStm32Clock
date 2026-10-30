#ifndef ONEWIRE_DS18B20_H
#define ONEWIRE_DS18B20_H
#include "main.h"
#include <stdint.h>
#include "LED_iic.h"
#define DS18B20_MAXCONVERT_TIME 800 // 数据处理时间最长为800ms
uint8_t Onewire_read(TIM_HandleTypeDef *htimx);
void Onewire_transmit(TIM_HandleTypeDef *htimx, uint8_t data);
uint8_t OneWire_init(TIM_HandleTypeDef *htimx);
void DS18B20_tempConvert(TIM_HandleTypeDef *htimx);
float DS18B20_tempRead(TIM_HandleTypeDef *htimx);
float DS18B20_read(TIM_HandleTypeDef *htimx);
extern volatile long long DS18B20_launchConvertTick;
#endif
