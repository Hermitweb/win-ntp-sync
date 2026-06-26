# 使用说明

## 1. 首次安装

### 1.1 系统要求
- Windows 7 SP1 或更高版本
- 管理员权限（修改系统时间必需）
- UDP 123 出站网络访问

### 1.2 安装步骤

**方式 A：直接运行（推荐）**
1. 将 `WinTimeSync.exe` 复制到任意目录（如 `C:\Program Files\WinTimeSync\`）
2. 双击运行，会弹出 UAC 提权对话框
3. 点击「是」以管理员身份运行
4. 应用启动后会自动最小化到系统托盘（屏幕右下角）

**方式 B：开机自启**
1. 首次按方式 A 启动
2. 在托盘图标上右键 → 「设置...」
3. 勾选「开机自动启动」
4. 点击「保存设置」
5. 关闭应用，下次开机时自动启动

## 2. 界面介绍

### 2.1 系统托盘图标

| 颜色 | 含义 |
|------|------|
| 🟢 绿色 | 正常 - 时间已同步且偏移在阈值内 |
| 🟡 黄色 | 警告 - 偏移过大（> 5 秒） |
| 🔴 红色 | 错误 - 同步失败 |
| 🔵 蓝色 | 同步中 - 正在与 NTP 服务器通信 |

**右键菜单**：
- 立即同步
- 显示主窗口
- 上次同步时间 / 偏移 / 服务器（只读信息）
- 设置...
- 查看日志
- 关于
- 退出

### 2.2 主窗口

显示以下信息：
- 当前系统时间
- 上次同步时间
- 时间偏移量（毫秒）
- 往返延迟（毫秒）
- 参考服务器地址
- 下次自动同步时间
- 权限状态

可执行：
- 立即同步（按钮）
- 关闭窗口（隐藏到托盘，应用继续运行）

### 2.3 设置窗口

可配置：
- **同步间隔**：1~10080 分钟（1 周）
- **超时时间**：1~60 秒
- **偏移警告阈值**：100~60000 毫秒
- **启动时自动同步**：勾选后开机立即同步一次
- **同步成功时通知**：是否弹气泡
- **同步失败时通知**：是否弹气泡
- **偏移过大时通知**：是否弹气泡
- **开机自动启动**：注册表 Run 键开关

点击「保存设置」后立即生效。

### 2.4 日志窗口

显示最新的日志文件内容（只读）。

日志文件位置：`%APPDATA%\WinTimeSync\logs\`

格式：
```
2026-06-26 14:30:00.123 [INFO] 同步开始，目标服务器: pool.ntp.org
2026-06-26 14:30:00.456 [INFO] 收到响应，偏移: +12.5 ms, 延迟: 35.2 ms
2026-06-26 14:30:00.460 [INFO] 系统时间已调整
```

## 3. 常见场景

### 3.1 时间经常偏移几十秒

**症状**：每次开机时间都不准

**原因**：可能是 BIOS 电池电量不足，或 Windows 时间服务被关闭

**解决**：
- 检查 BIOS 电池
- 在「服务」中启用「Windows Time」服务
- 或使用 WinTimeSync 的「开机自启」+ 「启动时同步」

### 3.2 同步总是失败

**排查清单**：
- [ ] 网络能访问互联网（`ping 8.8.8.8`）
- [ ] DNS 解析正常（`nslookup pool.ntp.org`）
- [ ] 防火墙放行 UDP 123 出站
- [ ] 杀毒软件未拦截

**手动测试**：
```powershell
# 测试 NTP 端口连通性
Test-NetConnection -ComputerName pool.ntp.org -Port 123
```

### 3.3 想使用内部 NTP 服务器

修改配置文件 `%APPDATA%\WinTimeSync\config.ini`：
```ini
[Servers]
Server1=ntp.yourcompany.local
Server2=time.yourcompany.local
```

### 3.4 完全卸载

1. 关闭应用（托盘 → 退出）
2. 取消开机自启（如有）：设置 → 取消勾选 → 保存
3. 删除 `%APPDATA%\WinTimeSync\` 文件夹
4. 删除 EXE 文件

## 4. 高级主题

### 4.1 通过命令行启动

WinTimeSync 当前不接收命令行参数。如需静默启动：
```powershell
# 创建计划任务
$action = New-ScheduledTaskAction -Execute "C:\Path\To\WinTimeSync.exe"
$trigger = New-ScheduledTaskTrigger -AtLogOn
$principal = New-ScheduledTaskPrincipal -UserId "SYSTEM" -LogonType ServiceAccount -RunLevel Highest
Register-ScheduledTask -TaskName "WinTimeSync" -Action $action -Trigger $trigger -Principal $principal
```

### 4.2 远程批量部署

通过组策略（GPO）或 PowerShell 推送：
```powershell
# 复制文件
Copy-Item ".\WinTimeSync.exe" -Destination "\\SERVER\C$\Program Files\WinTimeSync\"

# 远程创建计划任务
Invoke-Command -ComputerName SERVER1,SERVER2 -ScriptBlock {
    $action = New-ScheduledTaskAction -Execute "C:\Program Files\WinTimeSync\WinTimeSync.exe"
    $trigger = New-ScheduledTaskTrigger -AtStartup
    Register-ScheduledTask -TaskName "WinTimeSync" -Action $action -Trigger $trigger -RunLevel Highest -Force
}
```

### 4.3 自定义 NTP 服务器列表

编辑 `%APPDATA%\WinTimeSync\config.ini`，按优先级排列：

```ini
[Servers]
Server1=192.168.1.10    # 本地首选
Server2=192.168.1.11    # 本地备用
Server3=pool.ntp.org    # 公网兜底
```

应用按顺序尝试，前两个失败时自动切换到 `pool.ntp.org`。

## 5. 故障反馈

如遇问题请提供：
- Windows 版本
- 应用版本（关于对话框中）
- 日志文件（`%APPDATA%\WinTimeSync\logs\` 最新的一个）
- 问题截图或描述

提交至：https://github.com/yourusername/win-ntp-sync/issues
