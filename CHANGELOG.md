# 更新日志

所有显著的功能变更都会记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [1.0.0] - 2026-06-26

### 新增
- 首次公开发布
- NTPv4 协议实现 (RFC 5905)
- 系统托盘 + 右键菜单（立即同步、设置、日志、退出）
- 主窗口：显示当前时间、同步结果、偏移量、延迟
- 设置窗口：同步间隔、超时、偏移警告阈值、通知开关、自启动开关
- 多 NTP 服务器配置（默认 7 个国内外公共服务器）
- 故障自动切换 + KoD 退避策略
- 系统时间调整（`SetSystemTime` API + 权限管理）
- 日志系统（每日滚动 + 大小限制 + 文件数限制）
- 配置文件持久化（INI 格式）
- UAC 提权（manifest + 运行时检测双保险）
- 开机自启（注册表 Run 键）
- 多状态托盘图标（正常/警告/错误/同步中）
- 完整版本信息和文件元数据

### 安全
- 嵌入 UAC manifest 声明 `requireAdministrator`
- 使用 `strncpy_s` 等安全字符串函数
- 完整的 NULL 参数检查
- snprintf 防止缓冲区溢出

### 文档
- 完整 README（含编译、配置、架构、API 说明）
- 产品需求文档 (docs/product_spec.md)
- LICENSE (MIT)
- CHANGELOG

### 已知问题
- 暂无
