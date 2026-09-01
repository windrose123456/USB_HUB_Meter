@echo off
chcp 65001 >nul
echo 正在复制固件和上位机文件...

:: 复制上位机整个输出目录（包含所有依赖）
echo 复制上位机程序...
xcopy /Y /E /I "..\USB_HUB_Meter_Host\bin\Debug\net8.0-windows\*" ".\Host\"
if %errorlevel% equ 0 (echo [OK] 上位机已复制到 Host\ 文件夹) else (echo [FAIL] 复制上位机失败)

:: 复制主固件 HEX
copy /Y "..\USB_HUB_Meter\output\USB_HUB_Meter.hex" ".\"
if %errorlevel% equ 0 (echo [OK] USB_HUB_Meter.hex) else (echo [FAIL] 复制主固件失败)

:: 复制 Bootloader HEX
copy /Y "..\bootloader\Objects\Bootloader.hex" ".\"
if %errorlevel% equ 0 (echo [OK] Bootloader.hex) else (echo [FAIL] 复制 Bootloader HEX 失败)

:: 创建快捷方式（放置在 Merged_Firmware 根目录）
echo 正在创建快捷方式...
if exist "USB_HUB_Meter_Host.lnk" del /Q "USB_HUB_Meter_Host.lnk"
set "SCRIPT_DIR=%~dp0"
powershell -Command "$dir = '%SCRIPT_DIR%'.TrimEnd('\'); $exe = Join-Path $dir 'Host\USB_HUB_Meter_Host.exe'; $WshShell = New-Object -comObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut((Join-Path $dir 'USB_HUB_Meter_Host.lnk')); $Shortcut.TargetPath = $exe; $Shortcut.WorkingDirectory = (Join-Path $dir 'Host'); $Shortcut.Save()"
if %errorlevel% equ 0 (echo [OK] 快捷方式已创建) else (echo [FAIL] 创建快捷方式失败)

echo 全部完成！双击 USB_HUB_Meter_Host.lnk 即可运行上位机。
pause