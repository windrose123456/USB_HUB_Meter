#ifndef		__INA226_H
#define	__INA226_H

#include "config.h"

/* ---- INA226 寄存器地址 ---- */
#define INA_ADDR    0x40
#define REG_CFG     0x00
#define REG_SV      0x01    /* Shunt Voltage */
#define REG_BV      0x02    /* Bus Voltage */
#define REG_PWR     0x03    /* Power */
#define REG_CUR     0x04    /* Current */
#define REG_CAL     0x05    /* Calibration */
#define REG_MFR     0xFE    /* Manufacturer ID */

/* ---- 函数声明 ---- */
void I2C_config(void);
void INA226_Init(void);
u16  INA226_ReadReg(u8 reg);
void INA226_WriteReg(u8 reg, u16 val);
void INA226_ReadAll(u8 *buf);

#endif
