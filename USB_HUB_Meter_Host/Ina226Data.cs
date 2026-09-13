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
    public bool   CalibrationValid;

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
    /// 从 8 字节响应数据解析 (SV, BV, PWR, CUR)
    /// </summary>
    public static Ina226Data Parse(byte[] resp)
    {
        var data = new Ina226Data
        {
            RawShuntVoltage  = (short)((resp[0] << 8) | resp[1]),
            RawBusVoltage    = (ushort)((resp[2] << 8) | resp[3]),
            RawPower         = (ushort)((resp[4] << 8) | resp[5]),
            RawCurrent       = (short)((resp[6] << 8) | resp[7]),
        };
        data.ValidateCalibration();
        return data;
    }

    /// <summary>
    /// 验证校准关系: CUR == SV×CAL/2048, PWR == CUR×BV/20000
    /// </summary>
    public void ValidateCalibration()
    {
        const int CAL = 2048;
        int expectedCur = (int)RawShuntVoltage * CAL / 2048;
        int expectedPwr = (int)RawCurrent * RawBusVoltage / 20000;
        // 功率允许±1容差，因为整数除法截断与四舍五入的差异
        CalibrationValid = (expectedCur == RawCurrent)
            && (Math.Abs(expectedPwr - (int)RawPower) <= 1);
    }
}
