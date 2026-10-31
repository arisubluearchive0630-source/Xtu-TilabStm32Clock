/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "LED_iic.h"
#include "keyboard.h"
#include "Onewire_DS18B20.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUTTONS_ATTACHED_NUM 3
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
SingleBtn buttons[BUTTONS_ATTACHED_NUM]; // 设置-上-下
static uint16_t buttons_GPIO[] = {GPIO_PIN_12, GPIO_PIN_15, GPIO_PIN_5};
static GPIO_TypeDef *buttons_GPIO_area[] = {GPIOB, GPIOB, GPIOB};
short int SettingStatus = 0; // 当前选择位置状态机

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
struct Platformtime
{
  volatile short int hours, minutes, seconds;
  volatile short int years, months, days;
};
struct State
{
  char _bool;
};

volatile struct Platformtime Time_present;
uint8_t rx_buffer[64];
char string_up[24];
char string_down[24];
char string_3[24];
char string_4[24];
short int alarm_hours = 14, alarm_minutes = 10; // 闹铃时间
_Bool alarm_trigger = false;                    // 闹铃是否正在触发
short int ALARM_TEMPTURE = 26;                  // 报警温度
volatile float current_tempture = 0;            // 当前温度
volatile short int *volume_pointer;             // !上下键要修改的值的指针，易变
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int max(int a, int b)
{
  if (a > b)
  {
    return a;
  }
  return b;
}
int min(int a, int b)
{
  if (a < b)
  {
    return a;
  }
  return b;
}
int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, 2);
  return len;
}
short int fixSettingStatus(short int value)
{
  if (SettingStatus > 8)
  {
    SettingStatus = 0;
  }
  else if (SettingStatus < 0)
  {
    SettingStatus = 0;
  }
  else
  {
    SettingStatus = value;
  }
  return SettingStatus;
}
void SleepDisplayBasedOnSetting(short int ignore)
{
  OLED_generateChace(&htim3);
  if (ignore == 0)
  {
    switch (SettingStatus)
    {
    case 0:
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = NULL;
      break;
    case 1:
      // 秒
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:%02d:  ", Time_present.hours, Time_present.minutes);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &Time_present.seconds;
      break;
    case 2:
      // 分
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:  :%02d", Time_present.hours, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &Time_present.minutes;
      break;
    case 3:
      // hours
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "  :%02d:%02d", Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &Time_present.hours;
      break;
    case 4:
      // days
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-  ", Time_present.years, Time_present.months);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &Time_present.days;
      break;
    case 5:
      // months
      OLED_ClearVram();
      sprintf(string_up, "%04d-  -%02d", Time_present.years, Time_present.days);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &Time_present.months;
      break;
    case 6:
      // years
      OLED_ClearVram();
      sprintf(string_up, "    -%02d-%02d", Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%.d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &Time_present.years;
      break;
    case 7:
      // 闹铃时
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:  :%02d ", alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &alarm_hours;
      break;
    case 8:
      // 闹铃分
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:  ", alarm_hours);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &alarm_minutes;
      break;
    case 9:
      // 温度报警
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:  ", current_tempture);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = &ALARM_TEMPTURE;
      break;
    default:
      OLED_ClearVram();
      sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
      sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
      sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
      sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
      OLED_ShowString(1, 1, string_up);
      OLED_ShowString(2, 1, string_down);
      OLED_ShowString(3, 1, string_3);
      OLED_ShowString(4, 1, string_4);
      OLED_Refresh(&htim3);
      volume_pointer = NULL;
      break;
    }
  }
  else
  {
    OLED_ClearVram();
    sprintf(string_up, "%04d-%02d-%02d", Time_present.years, Time_present.months, Time_present.days);
    sprintf(string_down, "%02d:%02d:%02d", Time_present.hours, Time_present.minutes, Time_present.seconds);
    sprintf(string_3, "C:%.2f A:%d", current_tempture, ALARM_TEMPTURE);
    sprintf(string_4, "A:%02d:%02d", alarm_hours, alarm_minutes);
    OLED_ShowString(1, 1, string_up);
    OLED_ShowString(2, 1, string_down);
    OLED_ShowString(3, 1, string_3);
    OLED_ShowString(4, 1, string_4);
    OLED_Refresh(&htim3);
    volume_pointer = NULL;
  }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */
  for (int i = 0; i < BUTTONS_ATTACHED_NUM; i++)
  {
    SingleBtn_Init(&buttons[i], buttons_GPIO_area[i], buttons_GPIO[i]);
  }
  //! Button initialization
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start(&htim3);
  HAL_UARTEx_ReceiveToIdle_IT(&huart1, rx_buffer, sizeof rx_buffer);
  iic_initialize();
  OLED_init(&htim3);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // 显示当前时间
    short int temp_time = Time_present.seconds;
    long long tmep_time_unix = HAL_GetTick();
    bool flag = false;
    if (HAL_GetTick() - DS18B20_launchConvertTick > 1000)
    {
      current_tempture = DS18B20_read(&htim3);
      DS18B20_tempConvert(&htim3);
    }
    while (Time_present.seconds == temp_time && HAL_GetTick() - tmep_time_unix < 500)
    {
      if (flag == false)
      {
        SleepDisplayBasedOnSetting(0);
        flag = true;
        continue;
      }
    }
    SleepDisplayBasedOnSetting(1);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 7;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3 | SCK_Pin | SDA_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB15 PB5 */
  GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_15 | GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SCK_Pin SDA_Pin */
  GPIO_InitStruct.Pin = SCK_Pin | SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    // * only run when the callback is triggered by TIM1
    // * update time
    if (!(SettingStatus <= 6 && SettingStatus >= 1))
    {
      // 在设置时间的时候暂停时间的流动（很高级吧！对吧对吧？！）
      Time_present.seconds++;
    }
    if (Time_present.seconds >= 60)
    {
      Time_present.seconds = 0;
      Time_present.minutes++;
    }
    if (Time_present.minutes >= 60)
    {
      Time_present.minutes = 0;
      Time_present.hours++;
    }
    if (Time_present.hours >= 24)
    {
      Time_present.hours = 0;
      Time_present.minutes = 0;
      Time_present.seconds = 0;
      Time_present.days++;
    }
    // ways to calculate years,month,days
    unsigned char isrunyears = ((Time_present.years % 4 == 0 && Time_present.years % 100 != 0) || (Time_present.years % 100 == 0 && Time_present.years % 400 == 0));
    short int DAYS_MAX;
    switch (Time_present.months)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      DAYS_MAX = 31;
      break;
    case 4:
    case 6:
    case 9:
    case 11:
      DAYS_MAX = 30;
      break;
    case 2:
      if (isrunyears)
      {
        DAYS_MAX = 29;
      }
      else
      {
        DAYS_MAX = 28;
      }
      break;
    default:
      DAYS_MAX = 31;
      // *although there is no possible to reach this sector
      break;
    }
    // * end calculating run years
    if (Time_present.days > DAYS_MAX)
    {
      Time_present.months++;
      Time_present.days = 1;
    }
    if (Time_present.months > 12)
    {
      Time_present.years++;
      Time_present.months = 1;
    }
    // 闹铃合法性
    alarm_hours = max(min(alarm_hours, 23), 0);
    alarm_minutes = max(min(alarm_minutes, 59), 0);
    // 闹铃触发的判断
    if (Time_present.minutes == alarm_minutes && Time_present.hours == alarm_hours && Time_present.seconds == 0 && alarm_trigger == false)
    {
      //* 闹铃使能，当前没有触发闹钟，在设定的时间的第零秒触发
      alarm_trigger = true;
    }
  }
  // !
}
// 按键触发中断函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /*  简单思考了一下，这里似乎没有办法区分端口，例如A1,B1，搜索资料发现这似乎是stm32的限制
      需要多加注意 */
  if (GPIO_Pin == GPIO_PIN_12 || GPIO_Pin == GPIO_PIN_5 || GPIO_Pin == GPIO_PIN_15)
  {
    // 若触发了按钮事件
    GetKeyboardStatus(buttons, 3);
    if (buttons[0].status == true)
    {
      // 假若设置键被按下
      fixSettingStatus(SettingStatus + 1);
    }
    // 处理上
    if (buttons[1].status == true)
    {
      *volume_pointer += 1;
    }
    // 处理下
    if (buttons[2].status == true && (*volume_pointer >= 1 || SettingStatus == 9))
    {
      *volume_pointer -= 1;
    }
  }
}
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1)
  {
    HAL_UARTEx_ReceiveToIdle_IT(huart, rx_buffer, sizeof rx_buffer); /* 重新武装，等下一包 */
    if (strncmp((char *)rx_buffer, "t?", 2) == 0)
    {
      // if t?
      printf("current time:%d-%d-%d %d:%d:%d\n", Time_present.years, Time_present.months, Time_present.days,
             Time_present.hours, Time_present.minutes, Time_present.seconds);
    }
    if (strncmp((char *)rx_buffer, "#date", 5) == 0 && Size >= 15 && rx_buffer[9] == '-' && rx_buffer[12] == '-')
    {
      // #date2026-05-10
      // a normal request should be 15 char long
      for (int i = 0; i < 15; i++)
      {
        rx_buffer[i] = rx_buffer[i] - '0';
      }
      short int years = rx_buffer[5] * 1000 + rx_buffer[6] * 100 + rx_buffer[7] * 10 + rx_buffer[8] * 1;
      short int months = rx_buffer[10] * 10 + rx_buffer[11] * 1;
      short int days = rx_buffer[13] * 10 + rx_buffer[14] * 1;
      Time_present.days = days;
      Time_present.months = months;
      Time_present.years = years;
    }
    if (strncmp((char *)rx_buffer, "#time", 5) == 0 && Size >= 13 && rx_buffer[7] == ':' && rx_buffer[10] == ':')
    {
      // #time12:30:11
      // a normal request should be 13 char long
      for (int i = 0; i < 13; i++)
      {
        rx_buffer[i] = rx_buffer[i] - '0';
      }
      short int hours = rx_buffer[5] * 10 + rx_buffer[6] * 1;
      short int minutes = rx_buffer[8] * 10 + rx_buffer[9] * 1;
      short int seconds = rx_buffer[11] * 10 + rx_buffer[12] * 1;
      Time_present.seconds = seconds;
      Time_present.minutes = minutes;
      Time_present.hours = hours;
    }
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
