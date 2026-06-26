@echo off
REM ============================================================================
REM WinTimeSync 安装脚本（Windows 批处理）
REM 用法：以管理员身份运行 install.bat
REM ============================================================================

setlocal enabledelayedexpansion

echo ===============================================
echo   WinTimeSync 安装脚本
echo ===============================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 请以管理员身份运行此脚本！
    pause
    exit /b 1
)

REM 设置安装目录
set "INSTALL_DIR=%ProgramFiles%\WinTimeSync"
set "EXE_NAME=WinTimeSync.exe"

REM 创建安装目录
echo [信息] 创建安装目录: %INSTALL_DIR%
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

REM 复制文件
echo [信息] 复制程序文件...
copy /Y "%~dp0%EXE_NAME%" "%INSTALL_DIR%\" >nul
if errorlevel 1 (
    echo [错误] 复制失败
    pause
    exit /b 1
)

REM 创建应用数据目录
set "APPDATA_DIR=%APPDATA%\WinTimeSync"
set "LOG_DIR=%APPDATA_DIR%\logs"
echo [信息] 创建应用数据目录: %APPDATA_DIR%
if not exist "%APPDATA_DIR%" mkdir "%APPDATA_DIR%"
if not exist "%LOG_DIR%" mkdir "%LOG_DIR%"

REM 复制配置示例（如果不存在）
if not exist "%APPDATA_DIR%\config.ini" (
    if exist "%~dp0config.example.ini" (
        echo [信息] 初始化配置文件
        copy /Y "%~dp0config.example.ini" "%APPDATA_DIR%\config.ini" >nul
    )
)

REM 添加到 PATH（可选）
REM setx PATH "%PATH%;%INSTALL_DIR%" /M

REM 创建计划任务实现开机自启（以 SYSTEM 身份）
echo [信息] 创建开机自启动计划任务...
schtasks /create /tn "WinTimeSync" /tr "\"%INSTALL_DIR%\%EXE_NAME%\"" /sc onlogon /rl highest /f >nul 2>&1
if errorlevel 1 (
    echo [警告] 创建计划任务失败，请检查系统权限
) else (
    echo [成功] 已创建开机自启动任务
)

REM 创建桌面快捷方式（可选）
set "DESKTOP=%USERPROFILE%\Desktop"
set "SHORTCUT=%DESKTOP%\WinTimeSync.lnk"
if exist "%DESKTOP%" (
    echo [信息] 创建桌面快捷方式...
    powershell -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%SHORTCUT%'); $s.TargetPath = '%INSTALL_DIR%\%EXE_NAME%'; $s.WorkingDirectory = '%INSTALL_DIR%'; $s.IconLocation = '%INSTALL_DIR%\%EXE_NAME%,0'; $s.Description = 'Windows NTP Time Sync'; $s.Save()" 2>nul
)

echo.
echo ===============================================
echo   安装完成！
echo ===============================================
echo   安装目录: %INSTALL_DIR%
echo   配置目录: %APPDATA_DIR%
echo   日志目录: %LOG_DIR%
echo   开机自启: 已启用
echo.
echo   立即启动 WinTimeSync...
echo ===============================================

REM 立即启动
start "" "%INSTALL_DIR%\%EXE_NAME%"

timeout /t 3 /nobreak >nul
exit /b 0
