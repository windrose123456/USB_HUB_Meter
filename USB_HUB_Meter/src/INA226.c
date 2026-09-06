#include "INA226.h"
#include "STC8G_H_Delay.h"

/* ================================================================
 *  硬件 I2C 底层操作
 * ================================================================ */

// 等待 I2C 操作完成 (SI 标志)
static void i2c_wait_si(void) {
    while (!(I2CMSST & 0x40));  // SI=0 表示空闲
    I2CMSST &= ~0x40;          // 清除 SI
}

// I2C 起始信号
static void i2c_start(void) {
    I2CMSCR = 0x01;  // START
    i2c_wait_si();
}

// I2C 重复起始
static void i2c_restart(void) {
    I2CMSCR = 0x01;  // START (重复起始与起始命令相同)
    i2c_wait_si();
}

// I2C 停止信号
static void i2c_stop(void) {
    I2CMSCR = 0x06;  // STOP
    i2c_wait_si();
}

// I2C 发送一字节
static void i2c_write_byte(u8 dat) {
    I2CTXD = dat;
    I2CMSCR = 0x02;  // SEND
    i2c_wait_si();
}

// I2C 读取一字节 + 应答
static u8 i2c_read_byte(u8 ack) {
    I2CMSCR = ack ? 0x04 : 0x05;  // READ_ACK / READ_NAK
    i2c_wait_si();
    return I2CRXD;
}

/* ================================================================
 *  I2C 配置
 * ================================================================ */

void I2C_config(void)
{
    // I2C 引脚: P3.2(SCL) P3.3(SDA) 设为准双向上拉
    P3M1 &= ~0x0C;  // P3.2, P3.3 = 准双向
    P3M0 &= ~0x0C;

    // 使能扩展寄存器访问
    P_SW2 |= 0x80;

    // I2C 主机模式, 速率 FOSC/24 ≈ 460kHz
    // I2CCFG: bit7=ENI2C, bit6=MSSL, bit5:4=MSpeed(00=FOSC/24)
    I2CCFG = 0xE0;
}

/* ================================================================
 *  INA226 寄存器读写
 * ================================================================ */

// INA226 写寄存器 (2字节数据)
void INA226_WriteReg(u8 reg, u16 val)
{
    i2c_start();
    i2c_write_byte(INA_ADDR << 1);  // 地址 + W
    i2c_write_byte(reg);
    i2c_write_byte((u8)(val >> 8));
    i2c_write_byte((u8)(val & 0xFF));
    i2c_stop();
}

// INA226 读寄存器 (2字节)
u16 INA226_ReadReg(u8 reg)
{
    u16 val;
    i2c_start();
    i2c_write_byte(INA_ADDR << 1);  // 地址 + W
    i2c_write_byte(reg);
    i2c_restart();                   // 重复起始
    i2c_write_byte((INA_ADDR << 1) | 1);  // 地址 + R
    val = (u16)i2c_read_byte(1) << 8;     // 读高字节 + ACK
    val |= i2c_read_byte(0);               // 读低字节 + NAK
    i2c_stop();
    return val;
}

/* ================================================================
 *  INA226 初始化
 * ================================================================ */

void INA226_Init(void)
{
    u16 id = INA226_ReadReg(REG_MFR);
    if (id != 0x5449) return;  // 未检测到 INA226

    // 配置: 1次平均, 1.1ms转换时间, 连续测量模式
    INA226_WriteReg(REG_CFG, 0x0247);

    // 校准: Rshunt=10mΩ, MaxI=3.2A → Cal = trunc(0.00512 / (Current_LSB × Rshunt))
    // Current_LSB = 3.2A / 32768 ≈ 0.0001A
    // Cal = trunc(0.00512 / (0.0001 × 0.01)) = 5120 = 0x1400
    INA226_WriteReg(REG_CAL, 0x1400);
}

/* ================================================================
 *  INA226 数据读取
 * ================================================================ */

// 读取所有寄存器值 (10字节, 用于协议响应)
void INA226_ReadAll(u8 *buf)
{
    u16 raw;

    raw = INA226_ReadReg(REG_BV);
    buf[0] = (u8)(raw >> 8); buf[1] = (u8)raw;

    raw = INA226_ReadReg(REG_SV);
    buf[2] = (u8)(raw >> 8); buf[3] = (u8)raw;

    raw = INA226_ReadReg(REG_CUR);
    buf[4] = (u8)(raw >> 8); buf[5] = (u8)raw;

    raw = INA226_ReadReg(REG_PWR);
    buf[6] = (u8)(raw >> 8); buf[7] = (u8)raw;

    raw = INA226_ReadReg(REG_MFR);
    buf[8] = (u8)(raw >> 8); buf[9] = (u8)raw;
}
