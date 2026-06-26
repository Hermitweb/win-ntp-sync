# WinTimeSync

<div align="center">

**Windows NTP 时间同步工具** - 轻量级托盘后台程序

[![GitHub release](https://img.shields.io/github/v/release/Hermitweb/win-ntp-sync?include_prereleases)](https://github.com/Hermitweb/win-ntp-sync/releases)
[![License](https://img.shields.io/github/license/Hermitweb/win-ntp-sync)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%207%2F8%2F10%2F11-brightgreen)]()
[![Language](https://img.shields.io/badge/language-C11-blue)]()
[![Build](https://img.shields.io/badge/build-passing-success)]()

</div>

## ✨ 特性

- 🚀 **轻量级单文件**：~340KB，无运行时依赖
- 🕐 **NTPv4 协议**：严格遵循 [RFC 5905](https://datatracker.ietf.org/doc/html/rfc5905)
- 🔄 **多服务器容灾**：默认 7 个国内外公共 NTP 服务器
- 🛡️ **UAC 提权**：manifest + 运行时检测双保险
- 🎨 **多状态托盘**：正常（绿）/ 警告（黄）/ 错误（红）/ 同步中（蓝）
- 📊 **完整 GUI**：主窗口、设置窗口、日志查看器、关于
- 🔔 **气泡通知**：同步成功/失败/偏移过大时弹出
- 📝 **日志系统**：每日滚动 + 大小限制 + 文件数限制
- ⚙️ **配置持久化**：INI 格式存于 `%APPDATA%\WinTimeSync\`
- 🚀 **开机自启**：可选，集成任务计划程序

## 📸 界面预览

### 托盘菜单
- 🟢 立即同步
- 📊 显示主窗口（当前时间、偏移、延迟）
- ⚙️ 设置（同步间隔、超时、阈值、通知）
- 📋 查看日志
- ℹ️ 关于
- ❌ 退出

## 🚀 快速开始

### 下载

从 [Releases](https://github.com/Hermitweb/win-ntp-sync/releases) 下载最新版本：

- `WinTimeSync-v1.0.0-x64.zip` - 64 位完整发布包
- `WinTimeSync-v1.0.0-x86.zip` - 32 位完整发布包

### 安装

1. 解压 ZIP
2. **右键 → 以管理员身份运行** `WinTimeSync.exe`
3. 接受 UAC 提权弹窗
4. 应用自动驻留系统托盘

或运行 `install.bat` 自动配置开机自启。

## 🛠️ 编译

### Linux/macOS（交叉编译）

```bash
# 64 位
make rebuild CROSS=x86_64-w64-mingw32-

# 32 位
make rebuild CROSS=i686-w64-mingw32-
```

### Windows

```bash
# 需要 MinGW-w64
make rebuild
```

详见 [docs/USAGE.md](docs/USAGE.md)

## 📚 文档

- [产品需求文档](docs/product_spec.md) - 完整的产品规格说明
- [使用说明](docs/USAGE.md) - 详细使用指南
- [更新日志](CHANGELOG.md) - 版本变更历史

## 🤝 贡献

欢迎提交 PR 和 Issue！

## 📜 许可证

[MIT License](LICENSE)

## 🙏 致谢

- [RFC 5905 - NTPv4](https://datatracker.ietf.org/doc/html/rfc5905)
- [NTP Pool Project](https://www.pool.ntp.org/)
- MinGW-w64 项目组
- 所有贡献者

---

⭐ 如果这个项目对你有帮助，请给个 Star！
