namespace USB_HUB_Meter_Host;

/// <summary>
/// INA226 计量芯片数据结构
/// 换算参数从 AppConfig.Ina226 读取
/// </summary>
struct Ina226Data
{
    // 原始寄存器值
    public ushort RawBusVoltage;
    public short  RawShuntVoltage;
    public short  RawCurrent;
    public ushort RawPower;
    public ushort ManufacturerId;

    /// <summary>
    /// 总线电压 (V)
    /// </summary>
    public readonly double GetBusVoltage(Ina226Config cfg)
        => RawBusVoltage * cfg.VoltageLSB / 1000.0;

    /// <summary>
    /// 分流电压 (mV)
    /// </summary>
    public readonly double GetShuntVoltage(Ina226Config cfg)
        => RawShuntVoltage * cfg.ShuntVoltageLSB / 1000.0;

    /// <summary>
    /// 电流 (A)
    /// </summary>
    public readonly double GetCurrent(Ina226Config cfg)
        => RawCurrent * cfg.CurrentLSB;

    /// <summary>
    /// 功率 (W)
    /// </summary>
    public readonly double GetPower(Ina226Config cfg)
        => RawPower * cfg.PowerMultiplier * cfg.CurrentLSB;

    /// <summary>
    /// 从 10 字节响应数据解析
    /// </summary>
    public static Ina226Data Parse(byte[] resp)
    {
        return new Ina226Data
        {
            RawBusVoltage    = (ushort)((resp[0] << 8) | resp[1]),
            RawShuntVoltage  = (short)((resp[2] << 8) | resp[3]),
            RawCurrent       = (short)((resp[4] << 8) | resp[5]),
            RawPower         = (ushort)((resp[6] << 8) | resp[7]),
            ManufacturerId   = (ushort)((resp[8] << 8) | resp[9]),
        };
    }
}
