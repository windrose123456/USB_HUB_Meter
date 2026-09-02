using System.Drawing;
using System.Windows.Forms;

namespace USB_HUB_Meter_Host
{
    partial class Form1
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        private void InitializeComponent()
        {
            components = new System.ComponentModel.Container();

            // ===== 窗口属性 =====
            AutoScaleMode = AutoScaleMode.Dpi;
            AutoScaleDimensions = new SizeF(96F, 96F);
            ClientSize = new Size(900, 620);
            MinimumSize = new Size(750, 500);
            Text = "USB HUB Power Monitor";
            BackColor = Theme.BgWindow;
            ForeColor = Theme.TextMain;
            Font = Theme.FontNormal;
            StartPosition = FormStartPosition.CenterScreen;
            FormBorderStyle = FormBorderStyle.Sizable;

            // ===== 顶部工具栏 =====
            pnlToolbar = new Panel
            {
                Dock = DockStyle.Top,
                Height = 48,
                BackColor = Theme.BgPanel,
                Padding = new Padding(8, 0, 8, 0),
            };

            cmbPorts = new ComboBox
            {
                Location = new Point(10, 11),
                Size = new Size(120, 25),
                DropDownWidth = 245,
                DropDownStyle = ComboBoxStyle.DropDownList,
                BackColor = Theme.BgInput,
                ForeColor = Theme.TextMain,
                FlatStyle = FlatStyle.Flat,
                Font = Theme.FontNormal,
            };

            btnRefresh = MakeToolBtn("刷新", 138, DoRefresh);
            btnConnect = MakeToolBtn("连接", 200, DoConnect);

            lblStatus = new Label
            {
                Text = "未连接",
                AutoSize = true,
                Location = new Point(275, 15),
                ForeColor = Theme.Disconnected,
                Font = Theme.FontNormal,
            };

            btnLED = MakeToolBtn("LED", 460, DoToggleLED);
            btnLED.Size = new Size(60, 28);

            btnReset = MakeToolBtn("复位HUB", 528, DoResetHUB);
            btnReset.Size = new Size(80, 28);

            btnFwExpand = MakeToolBtn("固件更新 ▸", 720, DoToggleFirmwarePanel);
            btnFwExpand.Size = new Size(100, 28);

            btnDebugExpand = MakeToolBtn("串口终端 ▸", 830, DoToggleDebugPanel);
            btnDebugExpand.Size = new Size(100, 28);

            pnlToolbar.Controls.AddRange(new Control[] {
                cmbPorts, btnRefresh, btnConnect, lblStatus,
                btnLED, btnReset, btnFwExpand, btnDebugExpand
            });

            // ===== 图表上方：实时数值显示 =====
            pnlValues = new Panel
            {
                Dock = DockStyle.Top,
                Height = 60,
                BackColor = Theme.BgWindow,
                Padding = new Padding(20, 8, 20, 0),
            };

            lblVoltageTitle = MakeValueTitle("电压", 20, Theme.VoltageColor);
            lblVoltage = MakeValueDisplay("— V", 20, Theme.VoltageColor);

            lblCurrentTitle = MakeValueTitle("电流", 280, Theme.CurrentColor);
            lblCurrent = MakeValueDisplay("— A", 280, Theme.CurrentColor);

            lblPowerTitle = MakeValueTitle("功率", 540, Theme.PowerColor);
            lblPower = MakeValueDisplay("— W", 540, Theme.PowerColor);

            pnlValues.Controls.AddRange(new Control[] {
                lblVoltageTitle, lblVoltage,
                lblCurrentTitle, lblCurrent,
                lblPowerTitle, lblPower
            });

            // ===== 图表区域 =====
            pnlChart = new Panel
            {
                Dock = DockStyle.Fill,
                BackColor = Theme.BgChart,
                Padding = new Padding(4),
            };

            formsPlot = new ScottPlot.FormsPlot
            {
                Dock = DockStyle.Fill,
                BackColor = Theme.BgChart,
            };

            pnlChart.Controls.Add(formsPlot);

            // ===== 图表控制栏 =====
            pnlChartControls = new Panel
            {
                Dock = DockStyle.Top,
                Height = 36,
                BackColor = Theme.BgPanel,
                Padding = new Padding(12, 6, 12, 6),
            };

            chkAuto = new CheckBox
            {
                Text = "自动刷新",
                AutoSize = true,
                Location = new Point(12, 8),
                ForeColor = Theme.TextMain,
                Font = Theme.FontNormal,
                Checked = false,
            };

            lblInterval = new Label
            {
                Text = "间隔",
                AutoSize = true,
                Location = new Point(110, 10),
                ForeColor = Theme.TextDim,
                Font = Theme.FontNormal,
            };

            txtInterval = new TextBox
            {
                Text = "500",
                Location = new Point(155, 7),
                Size = new Size(60, 23),
                BackColor = Theme.BgInput,
                ForeColor = Theme.TextMain,
                BorderStyle = BorderStyle.FixedSingle,
                Font = Theme.FontNormal,
                MaxLength = 5,
            };
            txtInterval.KeyPress += txtInterval_KeyPress;

            lblIntervalMs = new Label
            {
                Text = "ms",
                AutoSize = true,
                Location = new Point(230, 10),
                ForeColor = Theme.TextDim,
                Font = Theme.FontNormal,
            };

            lblMaxPoints = new Label
            {
                Text = "显示点数",
                AutoSize = true,
                Location = new Point(275, 10),
                ForeColor = Theme.TextDim,
                Font = Theme.FontNormal,
            };

            cboMaxPoints = new ComboBox
            {
                Location = new Point(350, 6),
                Size = new Size(70, 23),
                DropDownStyle = ComboBoxStyle.DropDownList,
                BackColor = Theme.BgInput,
                ForeColor = Theme.TextMain,
                FlatStyle = FlatStyle.Flat,
                Font = Theme.FontNormal,
            };
            cboMaxPoints.Items.AddRange(new object[] { "100", "200", "500", "1000" });
            cboMaxPoints.SelectedIndex = 1;

            btnReadOnce = MakeToolBtn("读取一次", 460, DoReadOnce);
            btnReadOnce.Location = new Point(460, 4);
            btnReadOnce.Size = new Size(90, 28);

            pnlChartControls.Controls.AddRange(new Control[] {
                chkAuto, lblInterval, txtInterval, lblIntervalMs,
                lblMaxPoints, cboMaxPoints, btnReadOnce
            });

            // ===== 固件更新面板 =====
            pnlFirmware = new Panel
            {
                Dock = DockStyle.Bottom,
                Height = 200,
                BackColor = Theme.BgPanel,
                Padding = new Padding(12, 8, 12, 8),
                Visible = false,
            };

            var lblFwTitle = new Label
            {
                Text = "固件更新",
                Location = new Point(12, 6),
                AutoSize = true,
                ForeColor = Theme.TextMain,
                Font = Theme.FontTitle,
            };

            var lblFwFile = new Label
            {
                Text = "固件文件:",
                Location = new Point(12, 38),
                AutoSize = true,
                ForeColor = Theme.TextDim,
                Font = Theme.FontNormal,
            };

            txtFwPath = new TextBox
            {
                Location = new Point(88, 35),
                Size = new Size(550, 23),
                BackColor = Theme.BgInput,
                ForeColor = Theme.TextMain,
                BorderStyle = BorderStyle.FixedSingle,
                Font = Theme.FontNormal,
            };

            btnBrowse = MakeToolBtn("浏览...", 640, DoBrowseFirmware);
            btnBrowse.Location = new Point(640, 34);
            btnBrowse.Size = new Size(70, 28);

            btnUpdate = MakeToolBtn("开始更新", 12, DoStartUpdate);
            btnUpdate.Location = new Point(12, 66);
            btnUpdate.Size = new Size(100, 28);
            btnUpdate.BackColor = Theme.Connected;

            progressBar = new ProgressBar
            {
                Location = new Point(124, 69),
                Size = new Size(500, 22),
                Style = ProgressBarStyle.Continuous,
            };

            lblFwStatus = new Label
            {
                Text = "",
                Location = new Point(634, 70),
                AutoSize = true,
                ForeColor = Theme.TextDim,
                Font = Theme.FontSmall,
            };

            rtbLog = new RichTextBox
            {
                Location = new Point(12, 100),
                Size = new Size(856, 88),
                ReadOnly = true,
                BackColor = Color.FromArgb(18, 18, 26),
                ForeColor = Theme.CurrentColor,
                Font = Theme.FontLog,
                BorderStyle = BorderStyle.FixedSingle,
                ScrollBars = RichTextBoxScrollBars.Vertical,
            };

            pnlFirmware.Controls.AddRange(new Control[] {
                lblFwTitle, lblFwFile, txtFwPath, btnBrowse,
                btnUpdate, progressBar, lblFwStatus, rtbLog
            });

            // ===== 串口终端面板 =====
            pnlDebug = new Panel
            {
                Dock = DockStyle.Bottom,
                Height = 180,
                BackColor = Theme.BgPanel,
                Padding = new Padding(12, 8, 12, 8),
                Visible = false,
            };

            var lblDebugTitle = new Label
            {
                Text = "串口终端",
                Location = new Point(12, 6),
                AutoSize = true,
                ForeColor = Theme.TextMain,
                Font = Theme.FontTitle,
            };

            chkLogEnable = new CheckBox
            {
                Text = "启用日志",
                AutoSize = true,
                Location = new Point(120, 6),
                ForeColor = Theme.TextMain,
                Font = Theme.FontNormal,
                Checked = true,
            };

            btnClearLog = MakeToolBtn("清空", 220, DoClearDebugLog);
            btnClearLog.Location = new Point(220, 2);
            btnClearLog.Size = new Size(70, 28);

            // 终端显示框 (左侧 3/4)
            rtbDebug = new RichTextBox
            {
                Location = new Point(12, 30),
                Size = new Size(636, 138),
                ReadOnly = true,
                BackColor = Color.FromArgb(18, 18, 26),
                ForeColor = Theme.TextDim,
                Font = Theme.FontLog,
                BorderStyle = BorderStyle.FixedSingle,
                ScrollBars = RichTextBoxScrollBars.Vertical,
            };

            // 右侧指令输入区域 (6个输入框+发送按钮，竖排)
            txtCmdInputs = new TextBox[6];
            btnSendCmds = new Button[6];

            for (int i = 0; i < 6; i++)
            {
                int y = 30 + i * 25;  // 从y=30开始，每个高25px

                txtCmdInputs[i] = new TextBox
                {
                    Location = new Point(658, y),
                    Size = new Size(120, 23),
                    BackColor = Theme.BgInput,
                    ForeColor = Theme.TextMain,
                    BorderStyle = BorderStyle.FixedSingle,
                    Font = Theme.FontLog,
                    CharacterCasing = CharacterCasing.Upper,
                    Tag = i,
                };
                txtCmdInputs[i].KeyDown += TxtCmdInput_KeyDown;

                btnSendCmds[i] = new Button
                {
                    Text = "发送",
                    Location = new Point(784, y),
                    Size = new Size(56, 23),
                    FlatStyle = FlatStyle.Flat,
                    BackColor = Theme.Connected,
                    ForeColor = Color.White,
                    Font = Theme.FontSmall,
                    Cursor = Cursors.Hand,
                    Tag = i,
                };
                btnSendCmds[i].Click += DoSendCmdByIndex;
            }

            pnlDebug.Controls.AddRange(new Control[] {
                lblDebugTitle, chkLogEnable, btnClearLog, rtbDebug
            });
            pnlDebug.Controls.AddRange(txtCmdInputs);
            pnlDebug.Controls.AddRange(btnSendCmds);

            // ===== 主布局 (Dock 顺序: Top先填充, Bottom后填充, Fill填剩余) =====
            // Dock 排列顺序: Bottom控件先加入, Top后加入, Fill最后
            Controls.Add(pnlChart);           // Fill - 最后加入，填充剩余空间
            Controls.Add(pnlDebug);           // Bottom - 调试日志
            Controls.Add(pnlFirmware);        // Bottom
            Controls.Add(pnlChartControls);   // Top (在pnlChart的Dock Top)
            Controls.Add(pnlValues);          // Top
            Controls.Add(pnlToolbar);         // Top - 第一个加入Dock.Top

            // 绑定事件 (按钮事件由 MakeToolBtn 绑定, 此处绑定其他控件)
            chkAuto.CheckedChanged += chkAuto_CheckedChanged;
            cboMaxPoints.SelectedIndexChanged += cboMaxPoints_SelectedIndexChanged;
        }

        #endregion

        // ===== UI 辅助方法 =====

        private static Button MakeToolBtn(string text, int x, EventHandler handler)
        {
            var btn = new Button
            {
                Text = text,
                Location = new Point(x, 10),
                Size = new Size(55, 28),
                FlatStyle = FlatStyle.Flat,
                BackColor = Theme.BtnBg,
                ForeColor = Theme.TextMain,
                Font = Theme.FontNormal,
                Cursor = Cursors.Hand,
            };
            btn.FlatAppearance.BorderColor = Theme.BtnBorder;
            btn.FlatAppearance.MouseOverBackColor = Theme.BtnHover;
            btn.Click += handler;
            return btn;
        }

        private static Label MakeValueTitle(string text, int x, Color color)
        {
            return new Label
            {
                Text = text,
                Location = new Point(x, 6),
                AutoSize = true,
                ForeColor = color,
                Font = new Font("Segoe UI", 8f, FontStyle.Bold),
            };
        }

        private static Label MakeValueDisplay(string init, int x, Color color)
        {
            return new Label
            {
                Text = init,
                Location = new Point(x, 24),
                AutoSize = true,
                ForeColor = color,
                Font = Theme.FontValue,
            };
        }

        // ===== 控件声明 =====

        // 顶部工具栏
        private Panel pnlToolbar;
        private ComboBox cmbPorts;
        private Button btnRefresh, btnConnect;
        private Label lblStatus;
        private Button btnLED, btnReset;
        private Button btnFwExpand;

        // 实时数值
        private Panel pnlValues;
        private Label lblVoltageTitle, lblVoltage;
        private Label lblCurrentTitle, lblCurrent;
        private Label lblPowerTitle, lblPower;

        // 图表
        private Panel pnlChart;
        private ScottPlot.FormsPlot formsPlot;

        // 图表控制
        private Panel pnlChartControls;
        private CheckBox chkAuto;
        private Label lblInterval;
        private TextBox txtInterval;
        private Label lblIntervalMs;
        private Label lblMaxPoints;
        private ComboBox cboMaxPoints;
        private Button btnReadOnce;

        // 固件更新
        private Panel pnlFirmware;
        private TextBox txtFwPath;
        private Button btnBrowse, btnUpdate;
        private ProgressBar progressBar;
        private Label lblFwStatus;
        private RichTextBox rtbLog;

        // 串口终端
        private Panel pnlDebug;
        private Button btnDebugExpand, btnClearLog;
        private CheckBox chkLogEnable;
        private RichTextBox rtbDebug;
        private TextBox[] txtCmdInputs = new TextBox[0];
        private Button[] btnSendCmds = new Button[0];
    }
}
