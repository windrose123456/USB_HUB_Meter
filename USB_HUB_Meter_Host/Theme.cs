using System.Drawing;

namespace USB_HUB_Meter_Host;

static class Theme
{
    // 窗口/面板背景
    public static readonly Color BgWindow  = Color.FromArgb(30, 30, 46);
    public static readonly Color BgPanel   = Color.FromArgb(42, 42, 62);
    public static readonly Color BgChart   = Color.FromArgb(22, 33, 62);
    public static readonly Color BgInput   = Color.FromArgb(50, 50, 66);

    // 文字
    public static readonly Color TextMain  = Color.FromArgb(224, 224, 232);
    public static readonly Color TextDim   = Color.FromArgb(140, 140, 148);
    public static readonly Color TextMuted = Color.FromArgb(100, 100, 108);

    // 状态
    public static readonly Color Connected = Color.FromArgb(46, 204, 113);
    public static readonly Color Disconnected = Color.FromArgb(127, 140, 141);
    public static readonly Color Error     = Color.FromArgb(231, 76, 60);

    // 按钮
    public static readonly Color BtnBg     = Color.FromArgb(58, 58, 92);
    public static readonly Color BtnHover  = Color.FromArgb(74, 74, 108);
    public static readonly Color BtnBorder = Color.FromArgb(80, 80, 100);

    // 曲线颜色
    public static readonly Color VoltageColor = Color.FromArgb(255, 107, 107);  // 珊瑚红
    public static readonly Color CurrentColor = Color.FromArgb(78, 205, 196);   // 青绿
    public static readonly Color PowerColor   = Color.FromArgb(255, 230, 109);  // 明黄

    // 字体
    public static readonly Font FontTitle  = new("Segoe UI", 11f, FontStyle.Bold);
    public static readonly Font FontValue  = new("Consolas", 18f, FontStyle.Bold);
    public static readonly Font FontNormal = new("Segoe UI", 9f);
    public static readonly Font FontSmall  = new("Segoe UI", 8f);
    public static readonly Font FontLog    = new("Consolas", 8.5f);
}
