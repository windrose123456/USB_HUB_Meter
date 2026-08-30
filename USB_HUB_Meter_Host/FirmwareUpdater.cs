using System.Diagnostics;
using System.IO.Ports;

namespace USB_HUB_Meter_Host;

/// <summary>
/// 固件更新器：Bootloader 通信与固件烧写
/// 流程: ENTER_IAP → 等待MCU复位 → BL_INFO握手 → 逐页擦写 → BL_REBOOT
/// </summary>
class FirmwareUpdater
{
    const int PAGE_SIZE = 512;
    const int APP_MAX = 0x1C00;   // 7KB 应用区
    const int CHUNK_SIZE = 32;    // BL_WRITE 最大 32 字节

    readonly Protocol _proto;

    public event Action<string>? LogMessage;
    public event Action<int, int>? ProgressChanged;

    public FirmwareUpdater(Protocol proto) => _proto = proto;

    public void Log(string msg) => LogMessage?.Invoke($"[{DateTime.Now:HH:mm:ss}] {msg}");

    public void SetProgress(int current, int max) => ProgressChanged?.Invoke(current, max);

    /// <summary>
    /// 执行固件更新（应在后台线程调用）
    /// </summary>
    public async Task<bool> UpdateAsync(SerialPort port, byte[] firmware, CancellationToken ct = default)
    {
        if (firmware.Length > APP_MAX)
        {
            Log($"错误: 固件大小 {firmware.Length} 超过应用区 {APP_MAX} 字节");
            return false;
        }

        // 对齐到页边界
        int pageCount = (firmware.Length + PAGE_SIZE - 1) / PAGE_SIZE;
        int totalSize = pageCount * PAGE_SIZE;
        if (totalSize > firmware.Length)
        {
            byte[] padded = new byte[totalSize];
            Array.Copy(firmware, padded, firmware.Length);
            Array.Fill(padded, (byte)0xFF, firmware.Length, totalSize - firmware.Length);
            firmware = padded;
        }

        int progressMax = pageCount * 2 + 2;  // erase + write + info + reboot
        SetProgress(0, progressMax);
        Log("===== 固件更新开始 =====");
        Log($"固件大小: {firmware.Length} bytes, {pageCount} 页");

        try
        {
            // 步骤1: 进入 IAP 模式
            Log("步骤 1/4: 进入 IAP 模式...");
            byte[] enterPkt = _proto.BuildPacket(_proto.Cmd.EnterIap, null);
            port.DiscardInBuffer();
            port.Write(enterPkt, 0, enterPkt.Length);

            // 尝试读取确认（MCU 可能已复位）
            try { ReadResponseRaw(port, 2000); }
            catch { /* MCU 复位后无响应是正常的 */ }

            Log("等待 MCU 复位进入 Bootloader...");
            await Task.Delay(1500, ct);
            port.DiscardInBuffer();
            SetProgress(1, progressMax);

            // 步骤2: 握手 Bootloader
            Log("步骤 2/4: 与 Bootloader 握手...");
            if (!SendBlCommand(port, _proto.Bl.BlInfo, null, 500))
            {
                Log("错误: Bootloader 无响应!");
                return false;
            }
            Log("Bootloader 已就绪");
            SetProgress(2, progressMax);

            // 步骤3: 逐页擦除 + 写入
            Log($"步骤 3/4: 擦写 {pageCount} 页...");
            for (int pg = 0; pg < pageCount; pg++)
            {
                ct.ThrowIfCancellationRequested();

                int addr = pg * PAGE_SIZE;

                // 擦除
                byte[] eraseParam = new byte[] { (byte)(addr >> 8), (byte)(addr & 0xFF) };
                if (!SendBlCommand(port, _proto.Bl.BlErase, eraseParam, 1000))
                {
                    Log($"擦除页 {pg} 失败 (0x{addr:X4})");
                    return false;
                }
                SetProgress(2 + pg * 2 + 1, progressMax);
                Log($"  擦除页 {pg}: 0x{addr:X4} OK");

                // 写入 (分 CHUNK_SIZE 块)
                for (int offset = 0; offset < PAGE_SIZE; offset += CHUNK_SIZE)
                {
                    ct.ThrowIfCancellationRequested();

                    int chunkLen = Math.Min(CHUNK_SIZE, PAGE_SIZE - offset);
                    byte[] wrParam = new byte[3 + chunkLen];
                    wrParam[0] = (byte)((addr + offset) >> 8);
                    wrParam[1] = (byte)((addr + offset) & 0xFF);
                    wrParam[2] = (byte)chunkLen;
                    Array.Copy(firmware, addr + offset, wrParam, 3, chunkLen);

                    if (!SendBlCommand(port, _proto.Bl.BlWrite, wrParam, 1000))
                    {
                        Log($"写入失败: 0x{addr + offset:X4}");
                        return false;
                    }
                }
                SetProgress(2 + pg * 2 + 2, progressMax);
            }

            // 步骤4: 复位到应用
            Log("步骤 4/4: 复位 MCU...");
            SendBlCommand(port, _proto.Bl.BlReboot, null, 500);
            SetProgress(progressMax, progressMax);
            Log("===== 固件更新完成! =====");
            return true;
        }
        catch (OperationCanceledException)
        {
            Log("更新已取消");
            return false;
        }
        catch (Exception ex)
        {
            Log($"异常: {ex.Message}");
            return false;
        }
    }

    /// <summary>
    /// 发送 Bootloader 命令并等待 ACK/NAK
    /// </summary>
    bool SendBlCommand(SerialPort port, byte cmd, byte[]? data, int timeoutMs)
    {
        byte[] pkt = _proto.BuildBlPacket(cmd, data);

        port.DiscardInBuffer();
        port.Write(pkt, 0, pkt.Length);

        var sw = Stopwatch.StartNew();
        while (sw.ElapsedMilliseconds < timeoutMs)
        {
            if (port.BytesToRead > 0)
            {
                byte resp = (byte)port.ReadByte();
                if (resp == _proto.Bl.BlAck) return true;
                if (resp == _proto.Bl.BlNak) return false;
            }
            else
            {
                Thread.Sleep(5);
            }
        }
        return false;
    }

    /// <summary>
    /// 读取应用层响应 (超时后返回原始字节)
    /// </summary>
    byte[]? ReadResponseRaw(SerialPort port, int timeoutMs)
    {
        var buf = new byte[64];
        int count = 0;
        var sw = Stopwatch.StartNew();

        while (sw.ElapsedMilliseconds < timeoutMs)
        {
            if (port.BytesToRead > 0)
            {
                buf[count++] = (byte)port.ReadByte();
                if (count >= 5)
                {
                    int dataLen = buf[2];
                    if (count >= 5 + dataLen) break;
                }
            }
            else
            {
                Thread.Sleep(5);
            }
        }
        return count > 0 ? buf[..count] : null;
    }
}
