namespace USB_HUB_Meter_Host;

/// <summary>
/// 通信协议：帧构建/解析/校验
/// 帧格式: [HEAD1][HEAD2][LEN][CMD][DATA...][CHECKSUM]
/// 所有常量从 ProtocolConfig 读取
/// </summary>
class Protocol
{
    readonly ProtocolConfig _cfg;

    public Protocol(ProtocolConfig config) => _cfg = config;

    // 便捷访问
    public byte Head1 => _cfg.Head1;
    public byte Head2 => _cfg.Head2;
    public byte RespOffset => _cfg.RespOffset;
    public AppCommands Cmd => _cfg.Commands;
    public BlCommands Bl => _cfg.Bootloader;

    /// <summary>
    /// 构建应用层帧: [HEAD1][HEAD2][LEN][CMD][DATA...][CHK]
    /// </summary>
    public byte[] BuildPacket(byte cmd, byte[]? data)
    {
        int dataLen = data?.Length ?? 0;
        byte[] pkt = new byte[5 + dataLen];
        pkt[0] = Head1;
        pkt[1] = Head2;
        pkt[2] = (byte)dataLen;
        pkt[3] = cmd;
        if (data != null)
            Array.Copy(data, 0, pkt, 4, dataLen);

        byte chk = 0;
        for (int i = 0; i < pkt.Length - 1; i++)
            chk ^= pkt[i];
        pkt[pkt.Length - 1] = chk;
        return pkt;
    }

    /// <summary>
    /// 构建 Bootloader 包: [CMD][DATA...] (无帧头, 无校验)
    /// </summary>
    public byte[] BuildBlPacket(byte cmd, byte[]? data)
    {
        int dataLen = data?.Length ?? 0;
        byte[] pkt = new byte[1 + dataLen];
        pkt[0] = cmd;
        if (data != null)
            Array.Copy(data, 0, pkt, 1, dataLen);
        return pkt;
    }

    /// <summary>
    /// 校验接收的帧数据, 返回 payload (去掉帧头和校验)
    /// 校验失败返回 null
    /// </summary>
    public byte[]? ValidatePacket(byte[] buf, int count)
    {
        if (count < 5) return null;
        if (buf[0] != Head1 || buf[1] != Head2) return null;

        int dataLen = buf[2];
        if (count < 5 + dataLen) return null;

        byte chk = 0;
        for (int i = 0; i < 4 + dataLen; i++)
            chk ^= buf[i];
        if (chk != buf[4 + dataLen]) return null;

        byte[] payload = new byte[dataLen];
        if (dataLen > 0)
            Array.Copy(buf, 5, payload, 0, dataLen);
        return payload;
    }

    /// <summary>
    /// 根据命令码获取命令名称（用于调试日志）
    /// </summary>
    public string GetCmdName(byte cmd)
    {
        byte baseCmd = (byte)(cmd & 0x7F);
        bool isResponse = (cmd & 0x80) != 0;
        string suffix = isResponse ? "(Resp)" : "";

        // 匹配应用层命令
        if (baseCmd == Cmd.GetData)  return "GET_DATA" + suffix;
        if (baseCmd == Cmd.SetLed)   return "SET_LED" + suffix;
        if (baseCmd == Cmd.ResetHub) return "RESET_HUB" + suffix;
        if (baseCmd == Cmd.GetInfo)  return "GET_INFO" + suffix;
        if (baseCmd == Cmd.EnterIap) return "ENTER_IAP" + suffix;

        // 匹配 Bootloader 命令
        if (baseCmd == Bl.BlInfo)   return "BL_INFO" + suffix;
        if (baseCmd == Bl.BlErase)  return "BL_ERASE" + suffix;
        if (baseCmd == Bl.BlWrite)  return "BL_WRITE" + suffix;
        if (baseCmd == Bl.BlReboot) return "BL_REBOOT" + suffix;

        return $"CMD_0x{cmd:X2}";
    }
}
