# WinTimeSync

> Windows NTP 时间同步工具 · 绿色单文件 · 后台托盘运行

[![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%207%2F8%2F10%2F11-brightgreen.svg)]()
[![License](https://img.shields.io/badge/license-MIT-orange.svg)]()
[![Build](https://img.shields.io/badge/build-passing-success.svg)]()

## 📖 项目简介

**WinTimeSync** 是一款用 C 语言（Win32 API）开发的轻量级 Windows 时间同步工具。

它以后台托盘程序的形式常驻运行，按设定间隔自动从公网 NTP 服务器获取精确时间，并校正本地系统时钟。适用于：

- 💼 服务器、工作站、虚拟机需要保持精准时间的场景
- 🕐 Windows 时间服务（w32time）失效或精度不足时的替代方案
- 🌐 内外网隔离环境需要走自定义 NTP 服务器列表的场景
- 🔧 系统维护、自动化运维、跨主机协同等对时间一致性敏感的工作

**核心优势**：
- 🚀 纯 C 实现，无运行时依赖，单个 EXE 文件
- 🎨 100KB 量级极小体积，不占资源
- 🛡️ 自动管理员提权（UAC Manifest + 运行时检测双保险）
- 🔄 多服务器容灾，支持 KoD 退避策略
- 📊 完整日志记录与配置持久化

## ✨ 主要特性

| 功能 | 说明 |
|------|------|
| 自动同步 | 设定时间间隔（默认 60 分钟）自动同步 |
| 手动同步 | 托盘菜单 / 主界面按钮立即触发 |
| 多 NTP 服务器 | 默认 7 个国内外公共服务器，可自定义 |
| 故障切换 | 单服务器失败自动尝试下一个 |
| 系统托盘 | 绿色（正常） / 黄色（警告） / 红色（错误） / 蓝色（同步中） |
| 气泡通知 | 同步成功/失败/偏移过大时弹出系统通知 |
| 开机自启 | 可选，注册表 Run 键管理 |
| 时间校正 | 调用 Win32 `SetSystemTime` 直接修改系统时间 |
| 日志记录 | 每日滚动日志，最大保留 5 个文件 |
| 配置持久化 | INI 格式存于 `%APPDATA%\WinTimeSync\config.ini` |
| 状态查看 | 主窗口展示当前时间、上次同步、偏移量、延迟 |
| 协议标准 | 严格遵循 NTPv4 (RFC 5905) |

## 🖥️ 系统要求

| 项 | 要求 |
|----|------|
| 操作系统 | Windows 7 SP1 / 8 / 8.1 / 10 / 11 / Server 2008R2+ |
| 架构 | x86 (32-bit) / x64 (64-bit) |
| 权限 | 需要管理员权限（用于修改系统时间） |
| 网络 | 能访问 NTP 服务器（默认走 UDP 123 端口） |
| 运行时 | 无（纯静态链接，单文件即可运行） |

## 🚀 快速开始

### 方式一：直接运行（推荐普通用户）

1. 从 [Releases](https://github.com/yourusername/win-ntp-sync/releases) 下载最新版本的 `WinTimeSync.exe`
2. **右键 → 以管理员身份运行**（首次需要）
3. 等待 5~10 秒，应用自动最小化到系统托盘
4. 在托盘图标上**右键**，可看到：
   - 立即同步
   - 显示主窗口
   - 设置…
   - 查看日志
   - 关于
   - 退出

### 方式二：设置开机自启动

托盘菜单 → 设置 → 勾选「开机自动启动」→ 保存。

应用会在下次开机时自动以管理员身份启动并驻留托盘。

## 🛠️ 编译构建

### 准备工具链

- **Linux/macOS**: MinGW-w64 cross-compilation toolchain
  ```bash
  # Ubuntu / Debian
  sudo apt install gcc-mingw-w64-x86-64 mingw-w64-tools

  # macOS
  brew install mingw-w64
  ```
- **Windows**: MinGW-w64 ([download](https://www.mingw-w64.org/)) 或 MSVC Build Tools

### 编译

```bash
# Linux/macOS（交叉编译为 64 位 Windows exe）
make rebuild CROSS=x86_64-w64-mingw32-

# Windows（原生编译，确保 MinGW-w64 在 PATH 中）
make rebuild
```

### 清理

```bash
make clean     # 删除构建产物
make rebuild   # 完整重新构建
```

### 生成图标

```bash
python3 scripts/generate_icons.py
```

依赖：Pillow（`pip install Pillow`）

## ⚙️ 配置说明

配置文件路径：`%APPDATA%\WinTimeSync\config.ini`

示例：

```ini
[Sync]
IntervalMinutes=60
TimeoutSeconds=3
MaxOffsetWarningMs=5000
SyncOnStartup=false

[Servers]
Server1=time.windows.com
Server2=pool.ntp.org
Server3=cn.pool.ntp.org
Server4=time.cloudflare.com
Server5=time1.aliyun.com
Server6=ntp.aliyun.com
Server7=time.asia.apple.com

[Log]
MaxFileSizeMB=1
MaxFiles=5
Path=C:\Users\<User>\AppData\Roaming\WinTimeSync\logs

[Notification]
OnSuccess=true
OnFailure=true
OnLargeOffset=true

[Startup]
AutostartEnabled=false
```

### 默认 NTP 服务器

| 服务器 | 位置 | 备注 |
|--------|------|------|
| `time.windows.com` | 全球 | Windows 默认 |
| `pool.ntp.org` | 全球 | NTP Pool Project |
| `cn.pool.ntp.org` | 中国 | NTP Pool CN 节点 |
| `time.cloudflare.com` | 全球 | Cloudflare |
| `time1.aliyun.com` | 中国 | 阿里云 |
| `ntp.aliyun.com` | 中国 | 阿里云备用 |
| `time.asia.apple.com` | 亚洲 | Apple |

## 🔧 技术架构

### 模块划分

```
WinTimeSync
├── main.c           // 入口、UAC 提权、消息循环
├── common.c/h       // 公共类型、错误码、路径工具
├── config.c/h       // INI 配置文件读写
├── log.c/h          // 日志系统
├── ntp_client.c/h   // NTPv4 协议实现
├── time_adjust.c/h  // 系统时间调整
├── sync_engine.c/h  // 同步引擎（定时器、容灾）
├── tray_icon.c/h    // 托盘 GUI、菜单
├── autostart.c/h    // 注册表自启动管理
└── resource.rc      // 图标、版本、Manifest
```

### NTP 协议实现

严格遵循 [RFC 5905](https://datatracker.ietf.org/doc/html/rfc5905)：

```
        NTP 报文（48 字节）
   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |LI | VN  |Mode |    Stratum    |     Poll      |   Precision    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                          Root Delay                           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                       Root Dispersion                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Reference Identifier                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                   Reference Timestamp (64)                    |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                   Originate Timestamp (64)                    |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                    Receive Timestamp (64)                     |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  |                    Transmit Timestamp (64)                    |
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

时间偏移量计算：
```
offset = ((T2 - T1) + (T3 - T4)) / 2
delay  = (T4 - T1) - (T3 - T2)
```

### 同步流程

```
        ┌──────────────┐
        │   启动应用    │
        └──────┬───────┘
               │
       ┌───────▼────────┐
       │  加载配置/日志  │
       └───────┬────────┘
               │
       ┌───────▼────────┐
       │  检查管理员权限 │
       └───────┬────────┘
               │
       ┌───────▼────────┐
       │  创建托盘 + UI  │
       └───────┬────────┘
               │
       ┌───────▼────────┐
    ┌─►│  等待定时器触发 │
    │  └───────┬────────┘
    │          │
    │  ┌───────▼────────┐
    │  │  选择 NTP 服务器│ ◄──── 故障切换
    │  └───────┬────────┘
    │          │
    │  ┌───────▼────────┐
    │  │ 发送 NTP 请求   │
    │  └───────┬────────┘
    │          │
    │  ┌───────▼────────┐
    │  │ 接收 NTP 响应   │
    │  └───────┬────────┘
    │          │
    │  ┌───────▼────────┐
    │  │ 计算偏移/延迟   │
    │  └───────┬────────┘
    │          │
    │  ┌───────▼────────┐
    │  │ 调整系统时间    │
    │  └───────┬────────┘
    │          │
    │  ┌───────▼────────┐
    │  │  记录日志/通知  │
    │  └───────┬────────┘
    │          │
    │          └────► (返回等待)
    │
    └─── (循环)
```

## 🐛 故障排除

### 1. 启动时没有 UAC 提权弹窗

**现象**：双击 EXE 后直接进入应用，没有权限弹窗

**原因**：Windows UAC 设置为"最低档（Never notify）"时，管理员账号下不会弹窗

**解决**：
- 设置 → 控制面板 → 用户账户 → 更改用户账户控制设置 → 调到默认级别
- 或在 EXE 上右键 → 以管理员身份运行

### 2. 同步总是失败

**排查步骤**：
1. 打开菜单 → "查看日志"，检查错误信息
2. 测试网络：命令行 `ping pool.ntp.org`
3. 检查防火墙是否放行 UDP 123 出站
4. 切换其他 NTP 服务器测试

### 3. 主界面显示"无管理员权限"

**解决**：
- 完全退出后重新以管理员身份启动
- 关闭杀毒软件后重试（部分杀软会拦截 AdjustTokenPrivileges）

### 4. 菜单中文乱码

如果出现乱码，请确认是从官方渠道下载的最新版本。源码默认使用 GBK 编码编译（适配中文 Windows）。

## 🤝 参与贡献

欢迎提交 PR、Issue。请遵循以下规范：

- C 代码风格：Linux kernel K&R 风格，4 空格缩进
- 提交前确保 `make rebuild` 无警告
- 重要功能变更请更新 docs/

## 📜 许可证

本项目使用 [MIT License](LICENSE) 开源。

## 🙏 致谢

- [RFC 5905 - NTPv4](https://datatracker.ietf.org/doc/html/rfc5905)
- [NTP Pool Project](https://www.pool.ntp.org/)
- MinGW-w64 项目组
- 所有贡献者

## 📞 联系方式

- 项目主页：https://github.com/yourusername/win-ntp-sync
- 问题反馈：https://github.com/yourusername/win-ntp-sync/issues
- 邮箱：support@example.com

---

**免责声明**：本软件按"原样"提供，不保证适用于任何特定用途。使用前请确保符合当地法律法规。
