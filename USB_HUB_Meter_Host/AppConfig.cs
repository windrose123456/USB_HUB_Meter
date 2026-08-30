using System.IO.Ports;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace USB_HUB_Meter_Host;

/// <summary>
/// 应用配置，上电时从 config.json 加载，不存在则自动生成默认值
/// </summary>
class AppConfig
{
    public SerialConfig Serial { get; set; } = new();
    public ProtocolConfig Protocol { get; set; } = new();
    public Ina226Config Ina226 { get; set; } = new();
    public ChartConfig Chart { get; set; } = new();
    public DebugConfig Debug { get; set; } = new();
    public WindowConfig Window { get; set; } = new();

    static readonly JsonSerializerOptions JsonOpts = new()
    {
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.Never,
    };

    /// <summary>
    /// 加载配置文件，失败返回默认值
    /// </summary>
    public static AppConfig Load(string path)
    {
        try
        {
            if (File.Exists(path))
            {
                string json = File.ReadAllText(path);
                return JsonSerializer.Deserialize<AppConfig>(json, JsonOpts) ?? CreateDefault();
            }
        }
        catch { }

        var cfg = CreateDefault();
        cfg.Save(path);
        return cfg;
    }

    /// <summary>
    /// 保存到文件
    /// </summary>
    public void Save(string path)
    {
        try
        {
            string json = JsonSerializer.Serialize(this, JsonOpts);
            File.WriteAllText(path, json);
        }
        catch { }
    }

    /// <summary>
    /// 默认配置
    /// </summary>
    public static AppConfig CreateDefault() => new();
}

class SerialConfig
{
    public int BaudRate { get; set; } = 115200;
    public int DataBits { get; set; } = 8;
    public string Parity { get; set; } = "None";
    public string StopBits { get; set; } = "One";
    public int ReadTimeout { get; set; } = 1000;
    public int WriteTimeout { get; set; } = 1000;

    /// <summary>
    /// 将字符串 Parity 转为 System.IO.Ports.Parity 枚举
    /// </summary>
    public Parity GetParity() => Parity?.ToLowerInvariant() switch
    {
        "odd" => System.IO.Ports.Parity.Odd,
        "even" => System.IO.Ports.Parity.Even,
        "mark" => System.IO.Ports.Parity.Mark,
        "space" => System.IO.Ports.Parity.Space,
        _ => System.IO.Ports.Parity.None,
    };

    /// <summary>
    /// 将字符串 StopBits 转为 System.IO.Ports.StopBits 枚举
    /// </summary>
    public StopBits GetStopBits() => StopBits?.ToLowerInvariant() switch
    {
        "two" => System.IO.Ports.StopBits.Two,
        "onepointfive" => System.IO.Ports.StopBits.OnePointFive,
        _ => System.IO.Ports.StopBits.One,
    };
}

class Ina226Config
{
    /// <summary>电流 LSB, A/bit (Rshunt=10mΩ, MaxI=3.2A → 0.0001)</summary>
    public double CurrentLSB { get; set; } = 0.0001;

    /// <summary>采样电阻, Ω</summary>
    public double RShunt { get; set; } = 0.01;

    /// <summary>总线电压 LSB, mV/bit</summary>
    public double VoltageLSB { get; set; } = 1.25;

    /// <summary>分流电压 LSB, µV/bit</summary>
    public double ShuntVoltageLSB { get; set; } = 2.5;

    /// <summary>功率倍率: powerLSB = Multiplier × currentLSB</summary>
    public double PowerMultiplier { get; set; } = 25.0;
}

class ChartConfig
{
    /// <summary>自动刷新间隔, ms</summary>
    public int AutoRefreshInterval { get; set; } = 500;

    /// <summary>图表最大显示点数</summary>
    public int MaxPoints { get; set; } = 200;

    /// <summary>刷新间隔可选值</summary>
    public int[] IntervalOptions { get; set; } = [200, 500, 1000, 2000];

    /// <summary>最大点数可选值</summary>
    public int[] MaxPointsOptions { get; set; } = [100, 200, 500, 1000];
}

class DebugConfig
{
    /// <summary>是否启用调试日志</summary>
    public bool LogEnabled { get; set; } = true;

    /// <summary>日志最大行数</summary>
    public int MaxLines { get; set; } = 500;
}

class WindowConfig
{
    public int Width { get; set; } = 900;
    public int Height { get; set; } = 620;

    /// <summary>-1 表示居中显示</summary>
    public int X { get; set; } = -1;
    public int Y { get; set; } = -1;
}

/// <summary>
/// 协议配置，命令码从 config.json 读取，与 MCU 固件保持一致
/// </summary>
class ProtocolConfig
{
    /// <summary>帧头字节1</summary>
    public byte Head1 { get; set; } = 0xAA;

    /// <summary>帧头字节2</summary>
    public byte Head2 { get; set; } = 0x55;

    /// <summary>响应命令偏移 (响应 = 请求 | Offset)</summary>
    public byte RespOffset { get; set; } = 0x80;

    /// <summary>应用层命令</summary>
    public AppCommands Commands { get; set; } = new();

    /// <summary>Bootloader 命令</summary>
    public BlCommands Bootloader { get; set; } = new();
}

class AppCommands
{
    public byte GetData { get; set; } = 0x01;
    public byte SetLed { get; set; } = 0x02;
    public byte ResetHub { get; set; } = 0x03;
    public byte GetInfo { get; set; } = 0x04;
    public byte EnterIap { get; set; } = 0x10;
}

class BlCommands
{
    public byte BlInfo { get; set; } = 0x05;
    public byte BlErase { get; set; } = 0x01;
    public byte BlWrite { get; set; } = 0x02;
    public byte BlReboot { get; set; } = 0x04;
    public byte BlAck { get; set; } = 0x06;
    public byte BlNak { get; set; } = 0x15;
}
