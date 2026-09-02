using System.Diagnostics;
using System.IO.Ports;
using System.Management;

namespace USB_HUB_Meter_Host
{
    public partial class Form1 : Form
    {
        // ===== 配置 =====
        readonly AppConfig _config;
        readonly Protocol _proto;
        static readonly string ConfigPath = Path.Combine(
            AppDomain.CurrentDomain.BaseDirectory, "config.json");

        // ===== 串口 =====
        SerialPort? _port;
        bool _connected;
        byte[]? _lastRawFrame;  // 保存完整原始帧，用于响应显示

        // ===== 后台监听 =====
        Thread? _serialMonitorThread;
        CancellationTokenSource? _monitorCts;
        readonly byte[] _rxBuffer = new byte[256];
        int _rxBufferCount;
        bool _waitingForCommandResponse;
        readonly ManualResetEventSlim _responseReceived = new(false);
        DateTime _lastRxTime;  // 最后收到字节的时间，用于超时检测

        // ===== INA226 数据缓冲 =====
        readonly List<double> _voltageData = new();
        readonly List<double> _currentData = new();
        readonly List<double> _powerData = new();
        readonly List<double> _timeData = new();   // X轴: 从0开始的秒数
        DateTime _startTime;
        int _maxPoints;

        // ===== 图表信号线 =====
        ScottPlot.Plottable.ScatterPlot? _signalV, _signalA, _signalW;

        // ===== 定时器 =====
        System.Windows.Forms.Timer? _timer;

        // ===== 固件更新 =====
        FirmwareUpdater? _fwUpdater;

        // ===== LED 状态 =====
        bool _ledOn;

        // ===== 日志文件 =====
        readonly StreamWriter _logFile;

        public Form1()
        {
            _config = AppConfig.Load(ConfigPath);
            _proto = new Protocol(_config.Protocol);
            _maxPoints = _config.Chart.MaxPoints;

            // 初始化日志文件
            string logDir = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "log");
            Directory.CreateDirectory(logDir);
            string logPath = Path.Combine(logDir, $"log_{DateTime.Now:yyyyMMdd_HHmmss}.txt");
            _logFile = new StreamWriter(logPath, append: true, encoding: System.Text.Encoding.UTF8) { AutoFlush = true };
            _logFile.WriteLine($"===== 会话开始 {DateTime.Now:yyyy-MM-dd HH:mm:ss} =====");
            _logFile.WriteLine($"[LOG] 日志路径: {logPath}");

            InitializeComponent();
            ApplyTheme();
            InitChart();
            InitTimer();
            ApplyConfigToControls();
            LoadCmdInputs();
        }

        // ================================================================
        //  初始化
        // ================================================================

        void ApplyTheme()
        {
            BackColor = Theme.BgWindow;
            ForeColor = Theme.TextMain;
            pnlToolbar.BackColor = Theme.BgPanel;
            pnlValues.BackColor = Theme.BgWindow;
            pnlChart.BackColor = Theme.BgChart;
            pnlChartControls.BackColor = Theme.BgPanel;
            pnlFirmware.BackColor = Theme.BgPanel;

            cmbPorts.BackColor = Theme.BgInput;
            cmbPorts.ForeColor = Theme.TextMain;
            txtInterval.BackColor = Theme.BgInput;
            txtInterval.ForeColor = Theme.TextMain;
            cboMaxPoints.BackColor = Theme.BgInput;
            cboMaxPoints.ForeColor = Theme.TextMain;
            txtFwPath.BackColor = Theme.BgInput;
            txtFwPath.ForeColor = Theme.TextMain;

            rtbLog.BackColor = Color.FromArgb(18, 18, 26);
            rtbLog.ForeColor = Theme.CurrentColor;
            rtbDebug.BackColor = Color.FromArgb(18, 18, 26);
            rtbDebug.ForeColor = Theme.TextDim;
        }

        void InitChart()
        {
            var plot = formsPlot.Plot;

            // 暗色背景 - 手动设置颜色
            plot.Style(
                figureBackground: Theme.BgChart,
                dataBackground: Theme.BgChart,
                grid: Color.FromArgb(40, 255, 255, 255),
                tick: Color.Gray,
                axisLabel: Color.LightGray,
                titleLabel: Color.White
            );

            // 坐标轴样式
            plot.XAxis.TickLabelStyle(fontSize: 8);
            plot.YAxis.TickLabelStyle(fontSize: 8);
            plot.YAxis2.TickLabelStyle(fontSize: 8);

            // 标题和标签
            plot.Title("INA226 实时数据", size: 12);
            plot.XAxis.Label("时间 (s)");
            plot.YAxis.Label("电压(V) / 电流(A)");
            plot.YAxis2.Label("功率(W)");

            // 初始化空数据
            double[] emptyX = { 0 };
            double[] emptyY = { 0 };
            _signalV = plot.AddScatter(emptyX, emptyY, color: Theme.VoltageColor);
            _signalV.Label = "电压(V)";

            _signalA = plot.AddScatter(emptyX, emptyY, color: Theme.CurrentColor);
            _signalA.Label = "电流(A)";

            _signalW = plot.AddScatter(emptyX, emptyY, color: Theme.PowerColor);
            _signalW.YAxisIndex = 1;
            _signalW.Label = "功率(W)";

            plot.Legend(true);

            // 禁用 ScottPlot 默认的鼠标缩放，保留拖动平移
            formsPlot.Configuration.Zoom = false;
            formsPlot.Configuration.Pan = true;

            // 绑定滚轮缩放
            formsPlot.MouseWheel += FormsPlot_MouseWheel;

            formsPlot.Refresh();
        }

        void InitTimer()
        {
            _timer = new System.Windows.Forms.Timer { Interval = _config.Chart.AutoRefreshInterval };
            _timer.Tick += Timer_Tick;
        }

        /// <summary>
        /// 将配置值应用到 UI 控件
        /// </summary>
        void ApplyConfigToControls()
        {
            // 刷新间隔 TextBox
            txtInterval.Text = _config.Chart.AutoRefreshInterval.ToString();

            // 最大点数 ComboBox
            cboMaxPoints.Items.Clear();
            foreach (var v in _config.Chart.MaxPointsOptions)
                cboMaxPoints.Items.Add(v.ToString());
            int mpIdx = Array.IndexOf(_config.Chart.MaxPointsOptions, _config.Chart.MaxPoints);
            cboMaxPoints.SelectedIndex = mpIdx >= 0 ? mpIdx : 0;

            // 调试日志开关
            chkLogEnable.Checked = _config.Debug.LogEnabled;
        }

        // ================================================================
        //  串口操作
        // ================================================================

        /// <summary>
        /// 串口条目，包含端口名和设备描述
        /// </summary>
        class PortInfo
        {
            public string PortName { get; set; } = "";
            public string Description { get; set; } = "";
            public override string ToString() => string.IsNullOrEmpty(Description)
                ? PortName
                : $"{PortName} - {Description}";
        }

        readonly List<PortInfo> _portInfos = new();

        void RefreshPorts()
        {
            cmbPorts.Items.Clear();
            _portInfos.Clear();

            foreach (var port in SerialPort.GetPortNames())
            {
                var info = new PortInfo { PortName = port };
                try
                {
                    using var searcher = new ManagementObjectSearcher(
                        $"SELECT * FROM Win32_PnPEntity WHERE Caption LIKE '%({port})%'");
                    foreach (var obj in searcher.Get())
                    {
                        string? caption = obj["Caption"]?.ToString();
                        if (!string.IsNullOrEmpty(caption))
                        {
                            // Caption 格式: "CH340 (COM3)" → 提取 "CH340"
                            int idx = caption.LastIndexOf('(');
                            info.Description = idx > 0 ? caption[..idx].Trim() : caption;
                        }
                        break;
                    }
                }
                catch { /* WMI 查询失败，只显示端口号 */ }

                _portInfos.Add(info);
                cmbPorts.Items.Add(info);
            }

            if (cmbPorts.Items.Count > 0)
                cmbPorts.SelectedIndex = 0;
        }

        void DoRefresh(object? s, EventArgs e) => RefreshPorts();

        void DoConnect(object? s, EventArgs e)
        {
            if (_connected)
            {
                StopSerialMonitor();
                _timer?.Stop();
                _port?.Close();
                _connected = false;
            }
            else
            {
                if (cmbPorts.SelectedItem is not PortInfo portInfo) return;
                try
                {
                    _port = new SerialPort(
                        portInfo.PortName,
                        _config.Serial.BaudRate,
                        _config.Serial.GetParity(),
                        _config.Serial.DataBits,
                        _config.Serial.GetStopBits())
                    {
                        ReadTimeout = _config.Serial.ReadTimeout,
                        WriteTimeout = _config.Serial.WriteTimeout,
                    };
                    _port.Open();
                    _connected = true;
                    StartSerialMonitor();
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"串口打开失败: {ex.Message}", "错误",
                        MessageBoxButtons.OK, MessageBoxIcon.Error);
                    return;
                }
            }
            UpdateConnectionUI();
        }

        void UpdateConnectionUI()
        {
            btnConnect.Text = _connected ? "断开" : "连接";
            lblStatus.Text = _connected ? "● 已连接" : "未连接";
            lblStatus.ForeColor = _connected ? Theme.Connected : Theme.Disconnected;
            btnLED.Enabled = _connected;
            btnReset.Enabled = _connected;
            btnReadOnce.Enabled = _connected;

            if (!_connected)
            {
                _timer?.Stop();
                chkAuto.Checked = false;
                // 清空图表数据，下次连接从0开始
                _timeData.Clear();
                _voltageData.Clear();
                _currentData.Clear();
                _powerData.Clear();
            }
        }

        // ================================================================
        //  协议: 发送/接收
        // ================================================================

        byte[]? SendCmd(byte cmd, byte[]? data, int timeoutMs = 1000)
        {
            if (!_connected || _port == null) return null;

            byte[] pkt = _proto.BuildPacket(cmd, data);
            string cmdName = _proto.GetCmdName(pkt[3]);

            // 显示发送数据
            AppendTerminal(RxTxType.Tx, pkt, cmdName);

            _port.DiscardInBuffer();
            _port.Write(pkt, 0, pkt.Length);

            // 等待响应 (后台线程会捕获数据)
            var resp = WaitForResponse(timeoutMs);

            // 显示响应数据
            if (resp != null && _lastRawFrame != null)
            {
                string respCmdName = _proto.GetCmdName(_lastRawFrame[3]);
                AppendTerminal(RxTxType.Rx, _lastRawFrame, respCmdName);
            }

            return resp;
        }

        // ================================================================
        //  INA226 数据读取与图表更新
        // ================================================================

        void DoReadOnce(object? s, EventArgs e) => ReadINA226();

        void ReadINA226()
        {
            if (!_connected) return;

            try
            {
                byte[]? r = SendCmd(_proto.Cmd.GetData, null, 500);
                if (r == null || r.Length < 10)
                {
                    lblVoltage.Text = "读取失败";
                    lblCurrent.Text = "— A";
                    lblPower.Text = "— W";
                    return;
                }

                var data = Ina226Data.Parse(r);
                var inaCfg = _config.Ina226;

                // 更新实时数值
                lblVoltage.Text = $"{data.GetBusVoltage(inaCfg):F4} V";
                lblCurrent.Text = $"{data.GetCurrent(inaCfg):F4} A";
                lblPower.Text = $"{data.GetPower(inaCfg):F4} W";

                // 追加到图表数据
                if (_timeData.Count == 0)
                    _startTime = DateTime.Now;

                double elapsed = (DateTime.Now - _startTime).TotalSeconds;
                _timeData.Add(elapsed);
                _voltageData.Add(data.GetBusVoltage(inaCfg));
                _currentData.Add(data.GetCurrent(inaCfg));
                _powerData.Add(data.GetPower(inaCfg));

                // 裁剪超过最大点数
                while (_voltageData.Count > _maxPoints)
                {
                    _timeData.RemoveAt(0);
                    _voltageData.RemoveAt(0);
                    _currentData.RemoveAt(0);
                    _powerData.RemoveAt(0);
                }

                // 更新图表
                UpdateChart();
            }
            catch (Exception ex)
            {
                lblVoltage.Text = $"异常: {ex.Message}";
            }
        }

        void UpdateChart()
        {
            if (_signalV == null || _signalA == null || _signalW == null) return;

            double[] xs = _timeData.ToArray();
            _signalV.Update(xs, _voltageData.ToArray());
            _signalA.Update(xs, _currentData.ToArray());
            _signalW.Update(xs, _powerData.ToArray());

            formsPlot.Plot.AxisAuto();
            formsPlot.Refresh();
        }

        /// <summary>
        /// Ctrl+滚轮缩放X轴, Shift+滚轮缩放Y轴, 向前放大向后缩小
        /// </summary>
        void FormsPlot_MouseWheel(object? sender, MouseEventArgs e)
        {
            var plot = formsPlot.Plot;
            var keys = Control.ModifierKeys;
            bool ctrl = (keys & Keys.Control) != 0;
            bool shift = (keys & Keys.Shift) != 0;

            if (!ctrl && !shift) return;

            // 缩放因子: 向前(正)缩小(范围扩大), 向后(负)放大(范围缩小)
            double factor = e.Delta > 0 ? 1.25 : 0.8;

            var limits = plot.GetAxisLimits();

            if (ctrl)
            {
                // 缩放 X 轴
                double xCenter = (limits.XMin + limits.XMax) / 2.0;
                double xRange = (limits.XMax - limits.XMin) * factor;
                plot.SetAxisLimitsX(xCenter - xRange / 2, xCenter + xRange / 2);
            }

            if (shift)
            {
                // 缩放 Y 轴 (左轴: 电压/电流)
                double yCenter = (limits.YMin + limits.YMax) / 2.0;
                double yRange = (limits.YMax - limits.YMin) * factor;
                plot.SetAxisLimitsY(yCenter - yRange / 2, yCenter + yRange / 2, 0);

                // 同时缩放右 Y 轴 (功率) — 需要单独获取
                // YAxis2 没有独立的 GetAxisLimits，用 SetAxisLimitsY 的 yAxisIndex 参数
                // 右轴范围通过信号线数据推算，此处简化为同步缩放
                plot.SetAxisLimitsY(yCenter - yRange / 2, yCenter + yRange / 2, 1);
            }

            formsPlot.Refresh();
        }

        void Timer_Tick(object? s, EventArgs e) => ReadINA226();

        // ================================================================
        //  串口终端 (统一显示)
        // ================================================================

        /// <summary>
        /// 数据方向枚举
        /// </summary>
        enum RxTxType { Tx, Rx }

        /// <summary>
        /// 统一终端显示方法
        /// </summary>
        void AppendTerminal(RxTxType type, byte[] data, string? cmdName = null)
        {
            if (chkLogEnable == null || !chkLogEnable.Checked) return;

            string timestamp = $"[{DateTime.Now:HH:mm:ss.fff}]";
            string hex = BitConverter.ToString(data).Replace("-", " ");
            string label = type == RxTxType.Tx ? "TX" : "RX";

            // 构建显示行
            string line = cmdName != null
                ? $"{timestamp} {label}  {cmdName}  {hex}"
                : $"{timestamp} {label}  {hex}";

            // 颜色: 绿色TX, 蓝色RX
            Color color = type == RxTxType.Tx
                ? Color.FromArgb(46, 204, 113)    // 绿色
                : Color.FromArgb(100, 149, 237);  // 蓝色

            // 写入日志文件
            _logFile.WriteLine($"[{label}] {line}");

            // 更新UI (线程安全)
            if (InvokeRequired)
            {
                BeginInvoke(() => AppendTerminal(type, data, cmdName));
                return;
            }

            rtbDebug.SelectionStart = rtbDebug.TextLength;
            rtbDebug.SelectionLength = 0;
            rtbDebug.SelectionColor = color;
            rtbDebug.AppendText(line + "\n");
            rtbDebug.SelectionColor = Theme.TextDim;
            rtbDebug.ScrollToCaret();

            // 限制行数，避免内存膨胀
            if (rtbDebug.Lines.Length > _config.Debug.MaxLines)
            {
                rtbDebug.SelectionStart = 0;
                rtbDebug.SelectionLength = rtbDebug.GetFirstCharIndexFromLine(100);
                rtbDebug.SelectedText = "";
            }
        }

        // ================================================================
        //  后台串口监听
        // ================================================================

        /// <summary>
        /// 启动后台串口监听线程
        /// </summary>
        void StartSerialMonitor()
        {
            StopSerialMonitor();

            _monitorCts = new CancellationTokenSource();
            _lastRxTime = DateTime.Now;
            _rxBufferCount = 0;
            _serialMonitorThread = new Thread(SerialMonitorLoop)
            {
                IsBackground = true,
                Name = "SerialMonitor"
            };
            _serialMonitorThread.Start(_monitorCts.Token);
        }

        /// <summary>
        /// 停止后台串口监听线程
        /// </summary>
        void StopSerialMonitor()
        {
            _monitorCts?.Cancel();
            if (_serialMonitorThread != null && _serialMonitorThread.IsAlive)
            {
                _serialMonitorThread.Join(500);
            }
            FlushRxBuffer();  // 停止时刷新残留数据
            _monitorCts?.Dispose();
            _monitorCts = null;
            _serialMonitorThread = null;
        }

        /// <summary>
        /// 后台监听循环 - 持续读取串口数据
        /// </summary>
        void SerialMonitorLoop(object? state)
        {
            var ct = (CancellationToken)state!;

            while (!ct.IsCancellationRequested && _connected && _port?.IsOpen == true)
            {
                try
                {
                    if (_port.BytesToRead > 0)
                    {
                        byte b = (byte)_port.ReadByte();
                        _lastRxTime = DateTime.Now;
                        ProcessReceivedByte(b);
                    }
                    else
                    {
                        // 检查缓冲区中的残留数据 (超时100ms，给 bootloader 响应足够时间)
                        if (_rxBufferCount > 0 && (DateTime.Now - _lastRxTime).TotalMilliseconds > 100)
                        {
                            FlushRxBuffer();
                        }
                        Thread.Sleep(1);
                    }
                }
                catch (Exception)
                {
                    break;
                }
            }
        }

        /// <summary>
        /// 刷新缓冲区中的残留数据 - 显示非协议帧数据
        /// </summary>
        void FlushRxBuffer()
        {
            if (_rxBufferCount == 0) return;

            // 如果正在等待命令响应，先通知
            if (_waitingForCommandResponse && _lastRawFrame != null)
            {
                _responseReceived.Set();
            }
            else
            {
                // 显示残留数据作为原始RX
                byte[] data = new byte[_rxBufferCount];
                Array.Copy(_rxBuffer, data, _rxBufferCount);
                AppendTerminal(RxTxType.Rx, data);
            }

            _rxBufferCount = 0;
        }

        /// <summary>
        /// 处理接收到的字节 - 尝试解析协议帧
        /// </summary>
        void ProcessReceivedByte(byte b)
        {
            // 添加到缓冲区
            if (_rxBufferCount >= _rxBuffer.Length)
            {
                // 缓冲区满，先刷新显示
                FlushRxBuffer();
            }

            _rxBuffer[_rxBufferCount++] = b;
            _lastRxTime = DateTime.Now;

            // 只有当缓冲区有足够数据且以 AA 55 开头时才尝试解析
            // 否则等待超时后由 FlushRxBuffer 显示
            if (_rxBufferCount >= 5 && _rxBuffer[0] == _proto.Head1 && _rxBuffer[1] == _proto.Head2)
            {
                int dataLen = _rxBuffer[2];
                int frameLen = 5 + dataLen;

                if (frameLen > _rxBuffer.Length)
                {
                    // 数据长度异常，刷新显示
                    FlushRxBuffer();
                    return;
                }

                if (_rxBufferCount >= frameLen)
                {
                    // 有足够数据，尝试验证
                    var payload = _proto.ValidatePacket(_rxBuffer, _rxBufferCount);
                    if (payload != null)
                    {
                        // 保存完整帧
                        _lastRawFrame = new byte[frameLen];
                        Array.Copy(_rxBuffer, _lastRawFrame, frameLen);

                        // 移除已处理的数据
                        if (_rxBufferCount > frameLen)
                        {
                            Array.Copy(_rxBuffer, frameLen, _rxBuffer, 0, _rxBufferCount - frameLen);
                            _rxBufferCount -= frameLen;
                        }
                        else
                        {
                            _rxBufferCount = 0;
                        }

                        // 如果是等待命令响应，通知等待线程
                        if (_waitingForCommandResponse)
                        {
                            _responseReceived.Set();
                        }
                        else
                        {
                            // 主动上报数据，直接显示
                            string cmdName = _proto.GetCmdName(_lastRawFrame[3]);
                            AppendTerminal(RxTxType.Rx, _lastRawFrame, cmdName);
                        }
                    }
                    else
                    {
                        // 校验失败，刷新显示
                        FlushRxBuffer();
                    }
                }
            }
        }

        /// <summary>
        /// 等待命令响应 (用于SendCmd)
        /// </summary>
        byte[]? WaitForResponse(int timeoutMs)
        {
            _responseReceived.Reset();
            _rxBufferCount = 0;
            _waitingForCommandResponse = true;

            bool gotResponse = _responseReceived.Wait(timeoutMs);

            _waitingForCommandResponse = false;

            if (gotResponse && _lastRawFrame != null)
            {
                return _proto.ValidatePacket(_lastRawFrame, _lastRawFrame.Length);
            }

            return null;
        }

        void DoToggleDebugPanel(object? s, EventArgs e)
        {
            pnlDebug.Visible = !pnlDebug.Visible;
            btnDebugExpand.Text = pnlDebug.Visible ? "串口终端 ▾" : "串口终端 ▸";
        }

        void DoClearDebugLog(object? s, EventArgs e)
        {
            rtbDebug?.Clear();
        }

        /// <summary>
        /// 根据索引发送指令
        /// </summary>
        void DoSendCmdByIndex(object? s, EventArgs e)
        {
            if (s is not Button btn || btn.Tag is not int idx) return;
            if (idx < 0 || idx >= txtCmdInputs.Length) return;

            if (!_connected || _port == null)
            {
                MessageBox.Show("请先连接设备", "提示", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            string input = txtCmdInputs[idx].Text.Trim();
            if (string.IsNullOrEmpty(input)) return;

            // 移除空格和其他分隔符
            input = input.Replace(" ", "").Replace("-", "").Replace("0x", "").Replace("0X", "");

            // 校验HEX字符
            if (input.Length % 2 != 0 || !System.Text.RegularExpressions.Regex.IsMatch(input, @"^[0-9A-Fa-f]+$"))
            {
                MessageBox.Show("请输入有效的HEX数据 (如: AA 55 01 01 01)", "格式错误",
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            try
            {
                byte[] data = Convert.FromHexString(input);
                AppendTerminal(RxTxType.Tx, data, $"CMD{idx + 1}");

                _port.DiscardInBuffer();
                _port.Write(data, 0, data.Length);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"发送失败: {ex.Message}", "错误",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        /// <summary>
        /// 输入框回车发送
        /// </summary>
        void TxtCmdInput_KeyDown(object? s, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Enter) return;
            if (s is not TextBox txt || txt.Tag is not int idx) return;

            // 触发对应发送按钮的点击
            if (idx >= 0 && idx < btnSendCmds.Length)
            {
                DoSendCmdByIndex(btnSendCmds[idx], EventArgs.Empty);
            }

            e.Handled = true;
            e.SuppressKeyPress = true;
        }

        /// <summary>
        /// 加载指令到输入框
        /// </summary>
        void LoadCmdInputs()
        {
            var cmds = _config.QuickCommand.Commands;
            for (int i = 0; i < txtCmdInputs.Length; i++)
            {
                if (i < cmds.Length)
                    txtCmdInputs[i].Text = cmds[i];
                else
                    txtCmdInputs[i].Text = "";
            }
        }

        /// <summary>
        /// 保存指令从输入框到配置
        /// </summary>
        void SaveCmdInputs()
        {
            var cmds = new string[txtCmdInputs.Length];
            for (int i = 0; i < txtCmdInputs.Length; i++)
                cmds[i] = txtCmdInputs[i].Text.Trim().ToUpper();
            _config.QuickCommand.Commands = cmds;
        }

        // ================================================================
        //  LED / HUB 控制
        // ================================================================

        void DoToggleLED(object? s, EventArgs e)
        {
            if (!_connected) return;

            _ledOn = !_ledOn;
            byte[]? r = SendCmd(_proto.Cmd.SetLed, new byte[] { (byte)(_ledOn ? 1 : 0) });
            if (r != null && r.Length >= 1)
                _ledOn = r[0] != 0;

            btnLED.Text = _ledOn ? "LED ●" : "LED ○";
            btnLED.ForeColor = _ledOn ? Theme.Connected : Theme.TextMain;
        }

        void DoResetHUB(object? s, EventArgs e)
        {
            if (!_connected) return;

            btnReset.Enabled = false;
            btnReset.Text = "复位中...";

            var t = new Thread(() =>
            {
                SendCmd(_proto.Cmd.ResetHub, null, 2000);
                BeginInvoke(() =>
                {
                    btnReset.Enabled = true;
                    btnReset.Text = "复位HUB";
                    MessageBox.Show("CH634X HUB 已复位", "完成",
                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                });
            }) { IsBackground = true };
            t.Start();
        }

        // ================================================================
        //  自动刷新控制
        // ================================================================

        void chkAuto_CheckedChanged(object? s, EventArgs e)
        {
            if (chkAuto.Checked)
            {
                if (int.TryParse(txtInterval.Text, out int interval) && interval > 0)
                    _timer!.Interval = interval;
                _timer?.Start();
            }
            else
            {
                _timer?.Stop();
            }
        }

        void txtInterval_KeyPress(object? s, KeyPressEventArgs e)
        {
            // 只允许数字和退格键
            if (!char.IsDigit(e.KeyChar) && e.KeyChar != '\b')
                e.Handled = true;
        }

        void cboMaxPoints_SelectedIndexChanged(object? s, EventArgs e)
        {
            if (cboMaxPoints.SelectedItem is string pts && int.TryParse(pts, out int max))
                _maxPoints = max;
        }

        // ================================================================
        //  固件更新
        // ================================================================

        void DoToggleFirmwarePanel(object? s, EventArgs e)
        {
            pnlFirmware.Visible = !pnlFirmware.Visible;
            btnFwExpand.Text = pnlFirmware.Visible ? "固件更新 ▾" : "固件更新 ▸";
        }

        void DoBrowseFirmware(object? s, EventArgs e)
        {
            using var dlg = new OpenFileDialog
            {
                Filter = "固件文件|*.bin;*.hex|所有文件|*.*",
            };
            if (dlg.ShowDialog() == DialogResult.OK)
                txtFwPath.Text = dlg.FileName;
        }

        async void DoStartUpdate(object? s, EventArgs e)
        {
            if (!_connected || _port == null)
            {
                MessageBox.Show("请先连接设备");
                return;
            }
            if (string.IsNullOrEmpty(txtFwPath.Text) || !File.Exists(txtFwPath.Text))
            {
                MessageBox.Show("请选择有效的固件文件");
                return;
            }

            byte[]? fwBin = LoadFirmware(txtFwPath.Text);
            if (fwBin == null || fwBin.Length == 0)
            {
                MessageBox.Show("固件文件为空或格式不支持");
                return;
            }

            btnUpdate.Enabled = false;
            btnBrowse.Enabled = false;
            rtbLog.Clear();
            progressBar.Value = 0;

            // 暂停后台监听 (固件更新器直接操作串口)
            StopSerialMonitor();

            _fwUpdater = new FirmwareUpdater(_proto);
            _fwUpdater.LogMessage += msg =>
            {
                if (InvokeRequired)
                    BeginInvoke(() => AppendLog(msg));
                else
                    AppendLog(msg);
            };
            _fwUpdater.ProgressChanged += (cur, max) =>
            {
                if (InvokeRequired)
                    BeginInvoke(() => { progressBar.Maximum = max; progressBar.Value = Math.Min(cur, max); });
                else
                {
                    progressBar.Maximum = max;
                    progressBar.Value = Math.Min(cur, max);
                }
            };
            _fwUpdater.RawLog += msg =>
            {
                // 解析固件更新日志中的 TX/RX 标记
                RxTxType type = msg.Contains("TX") ? RxTxType.Tx : RxTxType.Rx;
                // 提取 HEX 数据部分 (去掉时间戳和 TX/RX 标记)
                int hexStart = msg.IndexOf("AA");
                if (hexStart < 0) hexStart = msg.IndexOf("06"); // BL_ACK
                if (hexStart >= 0)
                {
                    string hexStr = msg[hexStart..].Replace(" ", "");
                    try
                    {
                        byte[] bytes = Convert.FromHexString(hexStr);
                        if (InvokeRequired)
                            BeginInvoke(() => AppendTerminal(type, bytes, "FW"));
                        else
                            AppendTerminal(type, bytes, "FW");
                    }
                    catch { /* 忽略解析失败 */ }
                }
            };

            bool ok = await Task.Run(() => _fwUpdater.UpdateAsync(_port, fwBin));

            // 恢复后台监听
            if (_connected && _port?.IsOpen == true)
                StartSerialMonitor();

            btnUpdate.Enabled = true;
            btnBrowse.Enabled = true;
            lblFwStatus.Text = ok ? "更新成功!" : "更新失败，请重试";
            lblFwStatus.ForeColor = ok ? Theme.Connected : Theme.Error;
        }

        static byte[]? LoadFirmware(string path)
        {
            string ext = Path.GetExtension(path).ToLowerInvariant();
            return ext switch
            {
                ".bin" => File.ReadAllBytes(path),
                ".hex" => ParseIntelHex(path),
                _ => null,
            };
        }

        static byte[] ParseIntelHex(string path)
        {
            var lines = File.ReadAllLines(path);
            var data = new List<byte>();
            int maxAddr = 0;

            foreach (var line in lines)
            {
                if (string.IsNullOrEmpty(line) || line[0] != ':') continue;

                int byteCount = Convert.ToInt32(line.Substring(1, 2), 16);
                int address = Convert.ToInt32(line.Substring(3, 4), 16);
                int recType = Convert.ToInt32(line.Substring(7, 2), 16);

                if (recType == 0x01) break;    // EOF
                if (recType != 0x00) continue; // 只处理数据记录

                while (data.Count < address + byteCount)
                    data.Add(0xFF);

                for (int i = 0; i < byteCount; i++)
                {
                    byte b = Convert.ToByte(line.Substring(9 + i * 2, 2), 16);
                    data[address + i] = b;
                    if (address + i > maxAddr) maxAddr = address + i;
                }
            }

            return data.GetRange(0, maxAddr + 1).ToArray();
        }

        void AppendLog(string msg)
        {
            _logFile.WriteLine($"[FW] {msg}");
            rtbLog.AppendText(msg + "\n");
            rtbLog.ScrollToCaret();
        }

        // ================================================================
        //  窗口事件
        // ================================================================

        protected override void OnLoad(EventArgs e)
        {
            base.OnLoad(e);

            // 应用窗口配置
            var win = _config.Window;
            ClientSize = new Size(win.Width, win.Height);
            if (win.X >= 0 && win.Y >= 0)
            {
                StartPosition = FormStartPosition.Manual;
                Location = new Point(win.X, win.Y);
            }

            RefreshPorts();
            UpdateConnectionUI();
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            StopSerialMonitor();
            _timer?.Stop();
            if (_port?.IsOpen == true) _port.Close();

            // 保存窗口位置到配置
            _config.Window.Width = ClientSize.Width;
            _config.Window.Height = ClientSize.Height;
            _config.Window.X = Location.X;
            _config.Window.Y = Location.Y;
            _config.Chart.AutoRefreshInterval = _timer?.Interval ?? 500;
            _config.Chart.MaxPoints = _maxPoints;
            _config.Debug.LogEnabled = chkLogEnable.Checked;

            // 保存指令输入框内容
            SaveCmdInputs();

            _config.Save(ConfigPath);

            _logFile.WriteLine($"===== 会话结束 {DateTime.Now:yyyy-MM-dd HH:mm:ss} =====");
            _logFile.Close();

            base.OnFormClosing(e);
        }
    }
}
