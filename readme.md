# 引脚图

引脚|用途
|--|--|
|b12|设置按钮|
|b15|上调整按钮|
|b5|下调整按钮|
|b9|iic-sda总线|
|b8|iic-sck总线|
|a9|uart|
|a10|uart|
|b3|LED指示灯|
|C13|STBY|
|a3|DS18B20 单总线数据|
|a5|电机 AIN1|
|a6|电机 AIN2|
|a7|电机 PWMA|
|b10|蜂鸣器I/O|

# 功能

- **时钟**：上电默认 2026-09-12 10:00:00，三个按键分别用于进入设置、上调、下调，可依次修改年/月/日/时/分/秒。
- **温度**：PA3 上的 DS18B20 约每 1s 读取一次，显示在 OLED 第 3 行。
- **温度报警**：温度高于 `ALARM_TEMPTURE`（默认 26℃）时置位 `alarm_trigger`，触发后 LED（PB3）闪烁、电机正转，整段动作持续约 2.8s（28 次 × 100ms）后停止。
- **闹铃**：可设置闹铃时/分，到点后同样置位 `alarm_trigger`，与温度报警共用 LED + 电机的提示动作。
- **显示**：SSD1306 OLED，软件 I2C（PB8/PB9）。
- **按键**：PB12/PB15/PB5 配置为 EXTI 上升沿中断，消抖逻辑在 `Core/Src/keyboard.c`。

# 电机驱动（TB6612FNG）

`Core/Src/TB6612FNG.c` / `TB6612FNG.h` 用结构体 `SingleMotor` 封装一路电机，引脚在调用前需完成 GPIO 初始化。

| 函数 | 作用 |
|--|--|
| `SingleMotor_Init(&motor, AIN1端口, AIN1脚, AIN2端口, AIN2脚, PWMA端口, PWMA脚)` | 绑定引脚，STBY 置高使能 |
| `SingleMotor_Forward(&motor)` / `SingleMotor_Reverse(&motor)` | 全速正转 / 反转 |
| `SingleMotor_Brake(&motor)` | 短路制动 |
| `SingleMotor_Stop(&motor)` | 滑行停止 |
| `SingleMotor_Standby()` | STBY 拉低，关闭输出 |
| `SingleMotor_Speed(&motor, -255~255)` | 软件 PWM 调速，需在主循环中反复调用 |

STBY 是整颗 TB6612FNG 共用的使能脚，固定为 PC13，初始化后长时间保持高电平。

# 按键（keyboard）

`Core/Src/keyboard.c` / `keyboard.h`，一个 `SingleBtn` 对应一个按键，引脚仍需外部先做 GPIO 初始化。

```c
typedef struct
{
    GPIO_TypeDef *area;   // 按键所在端口
    uint16_t      key;    // 按键引脚
    bool          status; // 消抖后的状态：按下为 true
    bool          flag;   // 预留的单次触发标志（当前未使用）
} SingleBtn;
```

| 函数 | 作用 |
|--|--|
| `void SingleBtn_Init(SingleBtn *self, GPIO_TypeDef *port, uint16_t key)` | 绑定端口/引脚，并读一次初值 |
| `void GetKeyboardStatus(SingleBtn *buttonArray, uint16_t length)` | 扫描 `length` 个按键，阻塞约 `KEY_DEBOUNCE_MS`（20ms）消抖，结果写回 `status` |

`GetKeyboardStatus` 是阻塞式消抖：先采样并记录状态，再在 20ms 窗口内反复采样，只要电平与记录不一致就把该键判为未按下。调用后直接读 `buttons[i].status` 即可。

# 显示（LED_iic / SSD1306）

`Core/Src/LED_iic.c` / `LED_iic.h`，软件 I2C（`SCK_Pin`=PB8、`SDA_Pin`=PB9，定义在 `main.h`）+ SSD1306，所有时序都靠 `delay_us` 忙等。

## 软件 I2C 底层

| 函数 | 作用 |
|--|--|
| `void delay_us(TIM_HandleTypeDef *htimx, unsigned short int time)` | 用定时器计数器忙等 `time` 微秒，传入的定时器需已配成 1MHz 计数（本工程用 `htim3`） |
| `void iic_initialize(void)` | 把 SCL/SDA 置为空闲电平 |
| `void iic_startsig(TIM_HandleTypeDef *htimx)` | 起始信号 |
| `void iic_stopsig(TIM_HandleTypeDef *htimx)` | 停止信号 |
| `uint8_t ACKs_judegment(TIM_HandleTypeDef *htimx)` | 读从机应答，返回 1=ACK、0=NACK |
| `uint8_t iic_transmit(TIM_HandleTypeDef *htimx, uint8_t databuffer)` | 发送一字节，返回从机 ACK 状态（1=ACK、0=NACK） |
| `uint8_t iic_receive(TIM_HandleTypeDef *htimx, _Bool askmore)` | 接收一字节；`askmore`=1 回 ACK 继续读，=0 回 NACK 结束 |

## OLED 显存与字符

显存是 `OLED_Vram[8][128]`（8 页 × 128 列），`OLED_Vram_chace` 保存上一次刷屏的快照，两者都用 `extern` 暴露。

| 函数 | 作用 |
|--|--|
| `uint8_t OLED_init(TIM_HandleTypeDef *htimx)` | 初始化 SSD1306，返回 1=成功、0=失败（地址或命令未 ACK） |
| `void OLED_SetCursor(TIM_HandleTypeDef *htimx, uint8_t Y, uint8_t X)` | 定位到第 `Y` 页（0~7）、第 `X` 列（0~127） |
| `void OLED_Clear(TIM_HandleTypeDef *htimx)` | 清屏（向整屏写 0xFF，即全亮像素，用前注意） |
| `void OLED_ClearVram(void)` | 只清显存，不刷屏 |
| `void OLED_DrawChar(uint8_t line, uint8_t col, char ch)` | 把字符画进显存，`line` 1~4、`col` 1~16 |
| `uint8_t OLED_ShowString(uint8_t line, uint8_t col, const char *str)` | 写字符串进显存，支持 `\n` 与自动换行；返回 1=成功、0=参数越界 |
| `void OLED_Refresh(TIM_HandleTypeDef *htimx)` | 把显存刷到屏上，只写与快照不同的页 |
| `void OLED_generateChace(TIM_HandleTypeDef *htimx)` | 刷屏后同步快照，避免下一轮重复刷新 |

典型用法：`OLED_ShowString(...)` 改显存 → `OLED_Refresh(&htim3)` 刷屏 → `OLED_generateChace(&htim3)` 存快照。

# 温度（Onewire_DS18B20）

`Core/Src/Onewire_DS18B20.c` / `Onewire_DS18B20.h`，单总线协议，数据脚固定为 PA3（需先配成开漏输出）。

| 函数 | 作用 |
|--|--|
| `uint8_t OneWire_init(TIM_HandleTypeDef *htimx)` | 复位并检测存在脉冲，返回 1=器件在线、0=不在线 |
| `void Onewire_transmit(TIM_HandleTypeDef *htimx, uint8_t data)` | 发送一字节，低位先发 |
| `uint8_t Onewire_read(TIM_HandleTypeDef *htimx)` | 读一字节，低位先读 |
| `void DS18B20_tempConvert(TIM_HandleTypeDef *htimx)` | 发 0xCC（跳过 ROM）+ 0x44，启动一次温度转换，并记录时间戳 |
| `float DS18B20_tempRead(TIM_HandleTypeDef *htimx)` | 发 0xCC + 0xBE 读暂存器，返回原始温度（分辨率 0.0625℃） |
| `float DS18B20_read(TIM_HandleTypeDef *htimx)` | 在 `DS18B20_tempRead` 之上加软件滤波：变化超过 ±3℃ 或超出 -30~85℃ 时沿用上次值 |

`DS18B20_launchConvertTick`（`volatile long long`）记录最近一次启动转换的 tick，`DS18B20_MAXCONVERT_TIME`=800ms 是转换所需最长等待。典型用法是「先 `DS18B20_tempConvert` 启动 → 过 800ms 后 `DS18B20_read` 取值」。
