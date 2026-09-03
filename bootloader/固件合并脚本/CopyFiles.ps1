# CopyFiles.ps1
# 用途：将上位机程序、主固件 HEX、Bootloader HEX 复制到当前目录，
#       并为上位机创建快捷方式（方便直接运行）

Write-Host "正在复制固件和上位机文件..." -ForegroundColor Cyan

# ---------- 路径定义（基于脚本所在目录，确保绝对路径） ----------
$scriptDir      = $PSScriptRoot
$sourceHost     = Join-Path $scriptDir "..\USB_HUB_Meter_Host\bin\Debug\net8.0-windows"
$destHost       = Join-Path $scriptDir "Host"
$sourceMainHex  = Join-Path $scriptDir "..\USB_HUB_Meter\output\USB_HUB_Meter.hex"
$destMainHex    = Join-Path $scriptDir "USB_HUB_Meter.hex"
$sourceBootHex  = Join-Path $scriptDir "..\bootloader\Objects\Bootloader.hex"
$destBootHex    = Join-Path $scriptDir "Bootloader.hex"
$shortcutPath   = Join-Path $scriptDir "USB_HUB_Meter_Host.lnk"
$targetExe      = Join-Path $scriptDir "Host\USB_HUB_Meter_Host.exe"   # 快捷方式指向的绝对路径

# ---------- 1. 复制上位机整个发布目录 ----------
Write-Host "复制上位机程序..." -ForegroundColor Yellow
if (Test-Path $destHost) {
    Remove-Item -Recurse -Force $destHost
}
Copy-Item -Path $sourceHost -Destination $destHost -Recurse -Force
if ($?) {
    Write-Host "[OK] 上位机已复制到 Host\ 文件夹" -ForegroundColor Green
} else {
    Write-Host "[FAIL] 复制上位机失败" -ForegroundColor Red
    exit 1
}

# ---------- 2. 复制主固件 HEX ----------
Write-Host "复制主固件 HEX..." -ForegroundColor Yellow
Copy-Item -Path $sourceMainHex -Destination $destMainHex -Force
if ($?) {
    Write-Host "[OK] USB_HUB_Meter.hex" -ForegroundColor Green
} else {
    Write-Host "[FAIL] 复制主固件 HEX 失败" -ForegroundColor Red
}

# ---------- 3. 复制 Bootloader HEX ----------
Write-Host "复制 Bootloader HEX..." -ForegroundColor Yellow
Copy-Item -Path $sourceBootHex -Destination $destBootHex -Force
if ($?) {
    Write-Host "[OK] Bootloader.hex" -ForegroundColor Green
} else {
    Write-Host "[FAIL] 复制 Bootloader HEX 失败" -ForegroundColor Red
}

# ---------- 4. 创建快捷方式 ----------
Write-Host "创建快捷方式..." -ForegroundColor Yellow
if (Test-Path $shortcutPath) {
    Remove-Item -Force $shortcutPath
}
# 使用 WScript.Shell 创建 .lnk 快捷方式
$WshShell = New-Object -comObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut($shortcutPath)
$Shortcut.TargetPath = $targetExe          # 绝对路径，从任何位置双击均可找到 exe
$Shortcut.WorkingDirectory = $destHost     # 启动时工作目录设为 Host
$Shortcut.Save()
if ($?) {
    Write-Host "[OK] 快捷方式已创建：$shortcutPath" -ForegroundColor Green
} else {
    Write-Host "[FAIL] 创建快捷方式失败" -ForegroundColor Red
}

Write-Host "`n全部完成！双击 USB_HUB_Meter_Host.lnk 即可运行上位机。" -ForegroundColor Cyan
pause