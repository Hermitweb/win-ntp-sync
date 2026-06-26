@echo off
REM ============================================================================
REM WinTimeSync 卸载脚本（Windows 批处理）
REM 用法：以管理员身份运行 uninstall.bat
REM ============================================================================

setlocal enabledelayedexpansion

echo ===============================================
echo   WinTimeSync 卸载脚本
echo ===============================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [错误] 请以管理员身份运行此脚本！
    pause
    exit /b 1
)

set "INSTALL_DIR=%ProgramFiles%\WinTimeSync"
set "APPDATA_DIR=%APPDATA%\WinTimeSync"

REM 关闭运行中的应用
echo [信息] 关闭运行中的应用...
taskkill /f /im WinTimeSync.exe >nul 2>&1
timeout /t 2 /nobreak >nul

REM 删除计划任务
echo [信息] 移除开机自启任务...
schtasks /delete /tn "WinTimeSync" /f >nul 2>&1
if errorlevel 1 (
    echo [信息] 未找到计划任务
) else (
    echo [成功] 已移除计划任务
)

REM 删除桌面快捷方式
set "DESKTOP=%USERPROFILE%\Desktop"
if exist "%DESKTOP%\WinTimeSync.lnk" (
    echo [信息] 删除桌面快捷方式...
    del /f /q "%DESKTOP%\WinTimeSync.lnk" >nul 2>&1
)

REM 询问是否删除用户数据
set /p "DELETE_DATA=是否同时删除用户配置和日志? (Y/N): "
if /i "!DELETE_DATA!"=="Y" (
    if exist "%APPDATA_DIR%" (
        echo [信息] 删除配置和日志...
        rd /s /q "%APPDATA_DIR%"
    )
) else (
    echo [信息] 保留配置和日志
)

REM 删除安装目录
if exist "%INSTALL_DIR%" (
    echo [信息] 删除安装目录...
    rd /s /q "%INSTALL_DIR%"
)

echo.
echo ===============================================
echo   卸载完成！
echo ===============================================
timeout /t 3 /nobreak >nul
exit /b 0
