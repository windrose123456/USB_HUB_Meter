namespace USB_HUB_Meter_Host
{
    internal static class Program
    {
        [STAThread]
        static void Main()
        {
            // 全局异常处理 — 防止未捕获异常触发系统蜂鸣
            Application.ThreadException += (_, e) =>
            {
                // UI线程异常: 静默处理，不弹窗不蜂鸣
                System.Diagnostics.Debug.WriteLine($"[UI Exception] {e.Exception.Message}");
            };

            AppDomain.CurrentDomain.UnhandledException += (_, e) =>
            {
                // 非UI线程异常
                var ex = e.ExceptionObject as Exception;
                System.Diagnostics.Debug.WriteLine($"[Unhandled Exception] {ex?.Message}");
            };

            Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
            ApplicationConfiguration.Initialize();
            Application.Run(new Form1());
        }
    }
}
