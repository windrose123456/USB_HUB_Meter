# IAP 固件更新调试记录

## 2026-09-01 调试进展

### 系统架构

```
PC (上位机) ──UART──> STC8G1K08A ──I2C──> INA226
                          │
                     ┌────┴────┐
                     │ App     │ 0x0000-0x1BFF (7KB)
                     │ Boot    │ 0x1C00-0x1FFF (1KB)
                     │ Flag    │ 0x1F00 (IAP标志)
                     └─────────┘
```

### 固件更新流程

```
PC                          MCU(App)                 MCU(Boot)
 │  ENTER_IAP(0x10)     →   [回复ACK]
 │                              [写标志0xA5→复位]
 │                         ─────┤
 │  等待 1.5s                   │
 │  BL_INFO(0x05)          →   │  [ACK + 芯片信息]
 │  BL_ERASE ×N           →   │  [ACK] × N
 │  BL_WRITE ×N           →   │  [ACK] × N
 │  BL_REBOOT(0x04)       →   │  [ACK]
 │                              │  [清标志→跳转APP]
 │                         ←────┤
 │  设备重新枚举                 新固件开始运行
```

### 已验证通过的步骤 ✓

| 步骤 | 状态 | 证据 |
|------|------|------|
| Application 收到 ENTER_IAP | ✓ | 原始数据: `C:10` |
| 发送 ACK 响应 | ✓ | 原始数据: `AA-55-01-90-00-6E` |
| 擦除标志页 (0x1F00) | ✓ | 原始数据: `E` |
| 写入标志 0xA5 | ✓ | 原始数据: `W` |
| 读回验证标志 | ✓ | 原始数据: `V:A5` (写入成功) |
| 触发复位 | ✓ | 原始数据: `R` |
| Bootloader 在 0x1C00 | ✓ | STC-ISP 确认有代码 |

### 当前问题

**复位后 MCU 启动 Application，而不是 Bootloader**

- 复位后输出 `STC8G1K08 UART1 Test Programme`（Application 启动信息）
- 预期应该从 0x1C00 启动 Bootloader，进入 IAP 模式
- Bootloader 确认在 0x1C00 地址有代码

### Application 复位代码

```c
// procotol.c CMD_ENTER_IAP 处理
IAP_CONTR = 0xE0;  /* IAPEN=1, SWBS=1, SWRST=1 */
```

尝试过的值：
- `0x60` (SWBS=1, SWRST=1, IAPEN=0) → 失败
- `0xE0` (SWBS=1, SWRST=1, IAPEN=1) → 失败

### Bootloader 复位代码

```c
// bootloader.c jump_app
IAP_CONTR = 0x20;  /* SWBS=0, SWRST=1, 跳转到Application */
```

### 可能原因

1. **IAP_CONTR 寄存器 bit 定义问题** — STC8G1K08A 的 IAP_CONTR 可能与预期不同
2. **SWBS 位不生效** — 可能需要通过其他寄存器或机制控制启动源
3. **复位向量问题** — 0x0000 的复位向量可能覆盖 SWBS 行为
4. **IAP 配置问题** — STC-ISP 中的 IAP 大小设置可能影响启动行为

### 明天调试方向

1. **查 datasheet** — 确认 STC8G1K08A 的 IAP_CONTR 寄存器 bit 定义
2. **读回验证** — 复位前读回 IAP_CONTR 值，确认写入生效
3. **检查选项位** — 用 STC-ISP 查看芯片选项位配置（特别是 IAP 大小设置）
4. **简化测试** — 写一个最简单的测试程序，只做 IAP_CONTR 复位，看是否能跳转到 Bootloader

### 相关文件

- `USB_HUB_Meter/src/procotol.c` — Application CMD_ENTER_IAP 处理
- `bootloader/src/bootloader.c` — Bootloader 主程序
- `Merged_Firmware/merge_hex.ps1` — 合并固件脚本
- `USB_HUB_Meter_Host/FirmwareUpdater.cs` — 上位机固件更新器

### 编译/烧录流程

```bash
# 1. Keil 编译 Application
# 2. 合并固件
cd Merged_Firmware
merge.bat
# 3. STC-ISP 烧录 combined.hex
# 4. 上位机测试固件更新
```

### 命令码参考

| 命令 | 值 | 说明 |
|------|-----|------|
| CMD_GET_DATA | 0x01 | 读取 INA226 数据 |
| CMD_SET_LED | 0x02 | 控制 LED |
| CMD_RESET_HUB | 0x03 | 复位 HUB |
| CMD_GET_INFO | 0x04 | 获取设备信息 |
| CMD_ENTER_IAP | 0x10 | 进入 IAP 模式 |
| BL_INFO | 0x05 | Bootloader 信息 |
| BL_ERASE | 0x01 | 擦除 Flash 页 |
| BL_WRITE | 0x02 | 写入 Flash |
| BL_REBOOT | 0x04 | 复位到应用 |
| BL_ACK | 0x06 | Bootloader 确认 |
| BL_NAK | 0x15 | Bootloader 拒绝 |
