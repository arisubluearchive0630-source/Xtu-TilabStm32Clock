#include "main.h"
#include "LED_iic.h"
#include "OLED_font.h"
#define IIC_SCK_PIN GPIO_PIN_8
#define IIC_SCK_PIN_AREA GPIOB
#define IIC_SDA_PIN GPIO_PIN_9
#define IIC_SDA_PIN_AREA GPIOB
#define SCL_H() HAL_GPIO_WritePin(IIC_SCK_PIN_AREA, IIC_SCK_PIN, GPIO_PIN_SET)
#define SCL_L() HAL_GPIO_WritePin(IIC_SCK_PIN_AREA, IIC_SCK_PIN, GPIO_PIN_RESET)
#define SDA_H() HAL_GPIO_WritePin(IIC_SDA_PIN_AREA, IIC_SDA_PIN, GPIO_PIN_SET)
#define SDA_L() HAL_GPIO_WritePin(IIC_SDA_PIN_AREA, IIC_SDA_PIN, GPIO_PIN_RESET)
#define OLED_ADDRESS 0x78
#define IIC_WRITE 0x00
#define IIC_READ 0x01
void delay_us(TIM_HandleTypeDef *htimx, unsigned short int time)
{
    unsigned short int currentTicker = __HAL_TIM_GetCounter(htimx);
    while ((uint16_t)(__HAL_TIM_GetCounter(htimx) - currentTicker) < time)
        ;
}
// 返回0则nack 返回1则ack
uint8_t ACKs_judegment(TIM_HandleTypeDef *htimx)
{
    SCL_L();
    SDA_H(); // 释放sda总线
    delay_us(htimx, 5);
    SCL_H();            // 打一下scl拍子
    delay_us(htimx, 2); // 从机反应时间
    uint8_t ack = (HAL_GPIO_ReadPin(IIC_SDA_PIN_AREA, IIC_SDA_PIN) == GPIO_PIN_RESET);
    delay_us(htimx, 3);
    SCL_L();
    return ack;
}

void iic_initialize(void)
{
    HAL_GPIO_WritePin(IIC_SCK_PIN_AREA, IIC_SCK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IIC_SDA_PIN_AREA, IIC_SDA_PIN, GPIO_PIN_SET);
    // 在初始化时sda，sck均拉高
}
/// @brief 由单片机向指定的iic通信设备发送读请求
void iic_startsig(TIM_HandleTypeDef *htimx)
{
    // 向iic总线发送start信号 SCK-set SDA-fall high more than 4.7us low more than 4us
    HAL_GPIO_WritePin(IIC_SCK_PIN_AREA, IIC_SCK_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IIC_SDA_PIN_AREA, IIC_SDA_PIN, GPIO_PIN_SET);
    delay_us(htimx, 5);
    HAL_GPIO_WritePin(IIC_SDA_PIN_AREA, IIC_SDA_PIN, GPIO_PIN_RESET);
    delay_us(htimx, 5);
    HAL_GPIO_WritePin(IIC_SCK_PIN_AREA, IIC_SCK_PIN, GPIO_PIN_RESET);
}
void iic_stopsig(TIM_HandleTypeDef *htimx)
{
    // 向iic总线发送stop信号 SCK-set sda-raise 4,4.7
    SCL_L();
    SDA_L();
    delay_us(htimx, 5); /* 先在 SCL 低时把 SDA 拉低 */
    SCL_H();
    delay_us(htimx, 5);
    SDA_H();
    delay_us(htimx, 5); /* SCL 高时 SDA 上升 = STOP */
}
// @brief 向从机输入八位数据
uint8_t iic_transmit(TIM_HandleTypeDef *htimx, uint8_t databuffer)
{
    for (int i = 0; i < 8; i++)
    {
        uint8_t pin_status = (databuffer & 0x80); // 0x80=10000000 if highest digit is 1 then 0x80 / 0 then 0
        databuffer <<= 1;                         // 取下一位做提取
        // SCL置为低时切换数据
        SCL_L();
        if (pin_status)
        {
            SDA_H();
        }
        else
        {
            SDA_L();
        }
        delay_us(htimx, 5); // scl最低抖动时间设置为5us，之后就都按这个时间，标准模式
        SCL_H();
        delay_us(htimx, 5);
        SCL_L();
    }
    return ACKs_judegment(htimx);
}
uint8_t iic_receive(TIM_HandleTypeDef *htimx, _Bool askmore)
{
    SCL_L();
    delay_us(htimx, 5);
    SDA_H();
    uint8_t readbuffer = 0x00;
    for (int i = 0; i < 8; i++)
    {
        SCL_L();
        delay_us(htimx, 5);
        SCL_H();
        delay_us(htimx, 2);
        readbuffer <<= 1;
        readbuffer |= HAL_GPIO_ReadPin(IIC_SDA_PIN_AREA, IIC_SDA_PIN);
        delay_us(htimx, 3);
    }
    SCL_L();
    if (askmore)
        SDA_L();
    else
        SDA_H(); /* ACK / NACK */
    delay_us(htimx, 5);
    SCL_H();
    delay_us(htimx, 5);
    SCL_L();
    SDA_H();
    delay_us(htimx, 5); /* 收尾：时钟低、数据释放 */
    return readbuffer;
}
void OLED_SetCursor(TIM_HandleTypeDef *htimx, uint8_t Y, uint8_t X)
{
    iic_startsig(htimx);
    iic_transmit(htimx, OLED_ADDRESS);
    iic_transmit(htimx, 0x00);
    iic_transmit(htimx, 0xB0 | Y);                 // 设置Y位置
    iic_transmit(htimx, 0x10 | ((X & 0xF0) >> 4)); // 设置X位置高4位
    iic_transmit(htimx, 0x00 | (X & 0x0F));        // 设置X位置低4位
    iic_stopsig(htimx);
}
/**
 * @brief  OLED清屏
 * @param  无
 * @retval 无
 */
void OLED_Clear(TIM_HandleTypeDef *htimx)
{
    for (uint8_t j = 0; j < 8; j++)
    {
        OLED_SetCursor(htimx, j, 0); /* 命令事务：定位到第 j 页第 0 列 */

        iic_startsig(htimx); /* 数据事务：一次写满 128 列 */
        iic_transmit(htimx, OLED_ADDRESS);
        iic_transmit(htimx, 0x40); //! 数据窗口
        for (uint8_t i = 0; i < 128; i++)
        {
            iic_transmit(htimx, 0xFF);
        }
        iic_stopsig(htimx);
    }
}
uint8_t OLED_init(TIM_HandleTypeDef *htimx)
{
    HAL_Delay(100);
    iic_startsig(htimx);
    if (iic_transmit(htimx, OLED_ADDRESS) == 0)
        return 0; // 写入从机地址
    if (!iic_transmit(htimx, 0x00))
        return 0;              // 打开命令窗口
    iic_transmit(htimx, 0xAE); // 关闭显示

    iic_transmit(htimx, 0xD5); // 设置显示时钟分频比/振荡器频率
    iic_transmit(htimx, 0x80);

    iic_transmit(htimx, 0xA8); // 设置多路复用率
    iic_transmit(htimx, 0x3F);

    iic_transmit(htimx, 0xD3); // 设置显示偏移
    iic_transmit(htimx, 0x00);

    iic_transmit(htimx, 0x40); // 设置显示开始行

    iic_transmit(htimx, 0xA1); // 设置左右方向，0xA1正常 0xA0左右反置

    iic_transmit(htimx, 0xC8); // 设置上下方向，0xC8正常 0xC0上下反置

    iic_transmit(htimx, 0xDA); // 设置COM引脚硬件配置
    iic_transmit(htimx, 0x12);

    iic_transmit(htimx, 0x81); // 设置对比度控制
    iic_transmit(htimx, 0xCF);

    iic_transmit(htimx, 0xD9); // 设置预充电周期
    iic_transmit(htimx, 0xF1);

    iic_transmit(htimx, 0xDB); // 设置VCOMH取消选择级别
    iic_transmit(htimx, 0x30);

    iic_transmit(htimx, 0xA4); // 设置整个显示打开/关闭

    iic_transmit(htimx, 0xA6); // 设置正常/倒转显示

    iic_transmit(htimx, 0x8D); // 设置充电泵
    iic_transmit(htimx, 0x14);

    iic_transmit(htimx, 0xAF); // 开启显示
    iic_stopsig(htimx);
    OLED_Clear(htimx);
    return 1;
}
/* ===================== 显存（VRAM）与字符显示 ===================== */

/* 显存镜像：8 页 × 128 列 = 1024 字节 RAM */
uint8_t OLED_Vram[8][128];

/* 清空显存（不刷屏） */
void OLED_ClearVram(void)
{
    for (uint8_t j = 0; j < 8; j++)
    {
        for (uint8_t i = 0; i < 128; i++)
        {
            OLED_Vram[j][i] = 0x00;
        }
    }
}

/* 把一个 8x16 字符写进显存：line 1~4，col 1~16 */
void OLED_DrawChar(uint8_t line, uint8_t col, char ch)
{
    if (line < 1 || line > 4 || col < 1 || col > 16)
    {
        return;
    }
    if (ch < ' ' || ch > '~')
    {
        ch = ' ';
    }

    const unsigned char *g = OLED_F8x16[(unsigned char)ch - (unsigned char)' '];
    uint8_t page = (uint8_t)((line - 1) * 2);
    uint8_t x = (uint8_t)((col - 1) * 8);

    for (uint8_t i = 0; i < 8; i++)
    {
        OLED_Vram[page][x + i] = g[i];         /* 上半页 */
        OLED_Vram[page + 1][x + i] = g[i + 8]; /* 下半页 */
    }
}

/* 把字符串写进显存并自动换行
 *   line 1~4, col 1~16, 支持 '\n'
 *   列超过 16  -> 跳到下一行第 1 列
 *   行超过 4   -> 停止（不覆盖屏幕） */
uint8_t OLED_ShowString(uint8_t line, uint8_t col, const char *str)
{
    if (str == 0 || line < 1 || line > 4 || col < 1 || col > 16)
    {
        return 0;
    }

    while (*str)
    {
        if (*str == '\n')
        {
            line++;
            col = 1;
            str++;
            continue;
        }
        if (col > 16) /* 自动换行 */
        {
            col = 1;
            line++;
        }
        if (line > 4) /* 超出屏幕底部 */
        {
            break;
        }

        OLED_DrawChar(line, col, *str);
        col++;
        str++;
    }
    return 1;
}

/* 把整块显存刷到屏上：8 次事务 */
void OLED_Refresh(TIM_HandleTypeDef *htimx)
{
    for (uint8_t j = 0; j < 8; j++)
    {
        OLED_SetCursor(htimx, j, 0);
        iic_startsig(htimx);
        iic_transmit(htimx, OLED_ADDRESS);
        iic_transmit(htimx, 0x40); /* 数据窗口 */
        for (uint8_t i = 0; i < 128; i++)
        {
            iic_transmit(htimx, OLED_Vram[j][i]);
        }
        iic_stopsig(htimx);
    }
}
