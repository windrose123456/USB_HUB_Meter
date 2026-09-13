#include "INA226.h"
#include "STC8G_H_I2C.h"

/* ================================================================
 *  INA226 寄存器读写 (使用 STC8G_H_I2C 库函数 I2C_WriteNbyte / I2C_ReadNbyte)
 *
 *  INA226 写协议: Start + Addr_W + RegAddr + Data[2] + Stop
 *  INA226 读协议: Start + Addr_W + RegAddr + ReStart + Addr_R + Data[2] + Stop
 *
 *  I2C_WriteNbyte(dev_addr, mem_addr, *p, number)
 *    → Start + dev_addr(W) + mem_addr + *p[number] + Stop
 *
 *  I2C_ReadNbyte(dev_addr, mem_addr, *p, number)
 *    → Start + dev_addr(W) + mem_addr + Start + dev_addr(R) + *p[number] + Stop
 * ================================================================ */

// INA226 写寄存器 (2字节数据)
void INA226_WriteReg(u8 reg, u16 val)
{
	u8 buf[2];
	buf[0] = (u8)(val >> 8);       // 高字节
	buf[1] = (u8)(val & 0xFF);     // 低字节
	I2C_WriteNbyte(INA_ADDR << 1, reg, buf, 2);
}

// INA226 读寄存器 (2字节)
u16 INA226_ReadReg(u8 reg)
{
	u8 buf[2];
	I2C_ReadNbyte(INA_ADDR << 1, reg, buf, 2);
	return ((u16)buf[0] << 8) | buf[1];
}

/* ================================================================
 *  INA226 初始化
 * ================================================================ */

void INA226_Init(void)
{
	u16 id = INA226_ReadReg(REG_MFR);
	printf("INA226 ID = 0x%x\n", id);
	if (id != 0x5449) {
		printf("INA226 init fail!\r\n");
		return;  // 未检测到 INA226
	}

	// 配置: 4次平均, 1.1ms转换时间, 连续测量模式
	// 转换+IIC读取时间：1.1 * 4 + 1.1 * 4 + 2，大约10ms可以读取一次
	INA226_WriteReg(REG_CFG, 0x0327);

	// 
	INA226_WriteReg(REG_CAL, 0x0800);
}

/* ================================================================
 *  INA226 数据读取
 * ================================================================ */

// 读取所有寄存器值 (10字节, 用于协议响应)
void INA226_ReadAll(u8 *buf)
{
	u16 raw;

	raw = INA226_ReadReg(REG_SV);
	buf[0] = (u8)(raw >> 8); buf[1] = (u8)raw;

	raw = INA226_ReadReg(REG_BV);
	buf[2] = (u8)(raw >> 8); buf[3] = (u8)raw;

	raw = INA226_ReadReg(REG_PWR);
	buf[4] = (u8)(raw >> 8); buf[5] = (u8)raw;

	raw = INA226_ReadReg(REG_CUR);
	buf[6] = (u8)(raw >> 8); buf[7] = (u8)raw;

//	raw = INA226_ReadReg(REG_MFR);
//	buf[8] = (u8)(raw >> 8); buf[9] = (u8)raw;
}
