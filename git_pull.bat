@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ==============================
echo  Git Quick Save
echo ==============================

:: 获取当前分支
for /f %%i in ('git branch --show-current 2^>nul') do set "branch=%%i"

echo 当前分支: %branch%
echo.

:: 生成临时脚本
set "script=%TEMP%\git_quick_save.cmd"
>"%script%" (
    echo chcp 65001 ^>nul
    echo cd /d "%~dp0"
    echo git pull
    echo pause
)

:: 在 git-cmd 新窗口中执行
start "Git Quick Save" git-cmd "%script%"

echo 已打开 Git 窗口，正在执行...
timeout /t 2 >nul