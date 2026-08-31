@echo off
echo ========================================
echo   Merge Firmware: Bootloader + App
echo ========================================
echo.

powershell -ExecutionPolicy Bypass -NoProfile -File "%~dp0merge_hex.ps1"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [OK] combined.hex generated, ready to flash
) else (
    echo.
    echo [FAIL] Merge failed, check errors above
)
echo.
