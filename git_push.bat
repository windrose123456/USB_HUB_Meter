@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ==============================
echo  Git Quick Save
echo ==============================

:: 获取当前分支
for /f %%i in ('git branch --show-current 2^>nul') do set "branch=%%i"

:: 获取时间戳
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value 2^>nul') do set "dt=%%I"
set "tag=%dt:~0,4%%dt:~4,2%%dt:~6,2%_%dt:~8,2%%dt:~10,2%%dt:~12,2%"

echo 当前分支: %branch%
echo.

:: 询问提交信息
set "msg="
set /p msg="输入提交信息（直接回车使用时间戳）: "

:: 没输入就用时间戳
if "%msg%"=="" set "msg=auto save %tag%"

echo.
echo 提交信息: %msg%
echo.

:: 生成临时脚本
set "script=%TEMP%\git_quick_save.cmd"
>"%script%" (
    echo chcp 65001 ^>nul
    echo cd /d "%~dp0"
    echo git status
    echo git add .
    echo git commit -m "%msg%"
    echo git push
    echo pause
)

:: 在 git-cmd 新窗口中执行
start "Git Quick Save" git-cmd "%script%"

echo 已打开 Git 窗口，正在执行...
timeout /t 2 >nul