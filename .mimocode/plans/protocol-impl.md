# 协议实现计划 (procotol.c/h)

## 目标
在 `procotol.c` 中实现帧解析逻辑，并在 `main.c` 中接入，用于协议调试。INA226 功能暂不实现，使用占位数据回复。

## 当前状态
- `procotol.c/h` 已存在，但 `parse_packet()` 为空壳（直接返回 0）
- `send_resp()` 和 `process_cmd()` 已实现，命令处理用占位数据
- 串口收发已调通：ISR 将数据存入 `RX1_Buffer[64]`，Timer0 提供 3.5 字符超时，`uart_rx_timeout_flag` 标志帧接收完成
- `main.c` 当前是简单回显测试（`Process_UART_Frame()`），需要改为调用协议处理
- `parse_packet` 和 `process_cmd` 都是 `static`，需要在头文件中声明

## 协议规范（来自 IAP.md）
```
帧格式: HEAD1(0xAA) HEAD2(0x55) LEN(1B) CMD(1B) DATA[0..N] CHECKSUM(1B)
校验和 = HEAD1 ⊕ HEAD2 ⊕ LEN ⊕ CMD ⊕ DATA[0] ⊕ ... ⊕ DATA[N]
响应 CMD = 请求 CMD | 0x80
```

## 修改内容

### 1. `inc/procotol.h`
- 添加 `#include "config.h"` 引入类型定义
- 添加函数声明：`unsigned char parse_packet(unsigned char *cmd, unsigned char *data, unsigned char *len);`
- 添加函数声明：`void protocol_process(void);`
- 保留已有的宏定义

### 2. `src/procotol.c`
- 移除 `parse_packet()` 和 `process_cmd()` 的 `static`
- 添加 `#include "STC8G_H_UART.h"` 引入 `RX1_Buffer` / `COM1` 访问
- 添加 `#include "procotol.h"`
- 实现 `parse_packet()` 为状态机：
  - 状态 0：等待 HEAD1 (0xAA)
  - 状态 1：等待 HEAD2 (0x55)
  - 状态 2：读取 LEN，校验 ≤ MAX_DATA
  - 状态 3：读取 CMD
  - 状态 4：读取 DATA[0..LEN-1]
  - 状态 5：读取 CHECKSUM，校验通过则返回 1
- 新增 `protocol_process()` 函数：
  - 调用 `parse_packet()` 从 `RX1_Buffer` 中解析一帧
  - 调用 `process_cmd()` 处理命令
  - 处理完毕后重置 `COM1.RX_Cnt`

### 3. `src/main.c`
- 添加 `#include "procotol.h"`
- 主循环中将 `Process_UART_Frame()` 回显调用替换为 `protocol_process()`
- 暂时保留 `Process_UART_Frame()` 函数（后续可删除）

## 修改文件清单
- `inc/procotol.h` — 添加 include 和函数声明
- `src/procotol.c` — 实现 parse_packet + protocol_process，移除 static
- `src/main.c` — 主循环中调用 protocol_process

## 验证方法
- Keil 编译，烧录到 STC8G1K08A
- 用串口工具或 C# 上位机发送以下测试帧：
  - `AA 55 00 04 04`（GET_INFO，无数据）→ 期望回复 `AA 55 04 84 00 01 00 08 01 [chk]`
  - `AA 55 00 05 05`（ECHO，无数据）→ 期望回复 `AA 55 01 85 00 [chk]`
  - `AA 55 02 05 41 42`（ECHO "AB"）→ 期望回复 `AA 55 03 85 00 41 42 [chk]`
  - `AA 55 00 01 01`（GET_DATA）→ 期望回复 `AA 55 0B 81 00 FF...FF [chk]`（占位数据）
  - `AA 55 01 02 01 03`（SET_LED ON，错误校验和）→ 期望无回复或 NAK
