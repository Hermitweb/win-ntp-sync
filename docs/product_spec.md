# WinTimeSync — Windows NTP 时间同步应用 产品开发文档

**版本**：v1.0.0  
**日期**：2026-06-26  
**状态**：开发中  

---

## 1. 产品概述

### 1.1 产品定位

WinTimeSync 是一款面向 Windows 用户的桌面级网络时间同步工具。通过原生 NTPv4 协议与全球标准时间服务器通信，帮助用户快速、精确地校准本地系统时间。应用以系统托盘形态常驻后台，兼顾轻量、透明与可配置性。

### 1.2 目标用户

- **普通 Windows 用户**：系统时间因主板电池耗尽、双系统切换等原因出现偏差，需要一键校准。
- **IT 运维人员**：需要确保多台 Windows 终端时间一致性，便于日志审计与故障排查。
- **开发者与测试人员**：对时间精度敏感，需要手动触发或定时自动同步的灵活机制。

### 1.3 核心价值

| 价值点 | 说明 |
|--------|------|
| 精确同步 | 基于完整 NTPv4 协议，支持往返延迟补偿，典型精度优于 50 毫秒 |
| 透明可控 | 系统托盘常驻，同步行为、偏移量、服务器状态一目了然 |
| 多服务器容灾 | 内置主备服务器列表，单点故障自动切换 |
| 低打扰 | 静默定时同步，仅在异常时通过托盘气泡提示 |
| 安全合规 | 标准 UDP/123 端口通信，不注入、不混淆，最大限度降低杀毒软件误报 |

---

## 2. 功能需求 (Functional Requirements)

### 2.1 需求清单

| ID | 需求名称 | 优先级 | 描述 |
|----|----------|--------|------|
| FR-01 | 手动同步 | P0 | 用户通过托盘菜单触发即时 NTP 同步 |
| FR-02 | 定时自动同步 | P0 | 按配置间隔自动执行同步，默认 1 小时 |
| FR-03 | 多 NTP 服务器支持 | P0 | 支持配置主服务器与多个备用服务器，依次尝试 |
| FR-04 | 服务器故障切换 | P0 | 当前服务器超时或返回异常时，自动切换至下一个 |
| FR-05 | 同步历史日志 | P0 | 记录每次同步的时间、目标服务器、偏移量、结果状态 |
| FR-06 | 系统托盘 GUI | P0 | 最小化到系统托盘，提供右键菜单与状态主窗口 |
| FR-07 | 开机自启 | P1 | 支持注册/取消 Windows 开机启动项 |
| FR-08 | 配置持久化 | P1 | 服务器列表、同步间隔等配置保存到本地文件 |
| FR-09 | 日志文件轮转 | P1 | 单日志文件大小达到阈值后自动切分，保留最近 N 份 |
| FR-10 | 气泡通知 | P1 | 同步成功/失败/时间偏差过大时弹出托盘气泡 |
| FR-11 | 关于与帮助 | P2 | 显示应用版本、协议信息、开源许可 |
| FR-12 | 网络代理支持 | P3 | 支持 HTTP/SOCKS5 代理访问外部 NTP 服务器 |

### 2.2 用户场景 (Use Cases)

**UC-01：首次启动校准**
> 用户安装后首次运行，应用请求管理员权限，自动执行一次同步，托盘气泡提示"时间已校准，偏移 -1.2s"。

**UC-02：静默后台运行**
> 用户勾选"开机启动"后关机。下次开机应用自动以管理员身份启动，每小时静默同步一次，日志正常写入，用户无感知。

**UC-03：服务器不可达**
> 主服务器 `pool.ntp.org` 因网络故障超时（默认 3s），应用自动尝试备用服务器 `time.windows.com`，切换成功后记录日志并在气泡中简要提示。

**UC-04：查看同步历史**
> 用户右键托盘图标选择"查看日志"，主窗口切换至日志标签页，展示最近 50 条同步记录，支持滚动浏览。

---

## 3. 用户界面设计

### 3.1 系统托盘图标与菜单

```
┌─────────────────────────┐
│  [图标] WinTimeSync     │
├─────────────────────────┤
│  立即同步      Ctrl+S   │
│  显示主窗口             │
│  ─────────────────────  │
│  上次同步: 2 分钟前     │
│  偏移量: -12ms          │
│  服务器: pool.ntp.org   │
│  ─────────────────────  │
│  设置...                │
│  查看日志               │
│  关于                   │
│  退出                   │
└─────────────────────────┘
```

**设计说明**：
- 图标颜色反映状态：绿色=正常，黄色=偏移量>1s，红色=上次同步失败
- 左键双击：打开主窗口
- 右键单击：弹出菜单

### 3.2 主窗口 — 状态页

```
┌─────────────────────────────────────┐
│  WinTimeSync                [_][X]  │
├─────────────────────────────────────┤
│  [状态]  [日志]  [设置]             │
├─────────────────────────────────────┤
│                                     │
│   当前系统时间: 2026-06-26 14:32:01 │
│   上次同步时间: 2026-06-26 13:32:00 │
│   参考服务器   : pool.ntp.org       │
│   时间偏移量   : -12 ms             │
│   往返延迟     : 28 ms              │
│   同步结果     : 成功               │
│                                     │
│        [ 立即同步 ]                 │
│                                     │
│   下次自动同步: 14:32:00 (约59分钟后)│
│                                     │
└─────────────────────────────────────┘
```

### 3.3 主窗口 — 设置页

```
┌─────────────────────────────────────┐
│  [状态]  [日志]  [设置]             │
├─────────────────────────────────────┤
│  NTP 服务器列表:                     │
│  ┌─────────────────────────────┐   │
│  │ 1. pool.ntp.org      [上移] │   │
│  │ 2. time.windows.com  [下移] │   │
│  │ 3. cn.pool.ntp.org   [删除] │   │
│  └─────────────────────────────┘   │
│  [ + 添加服务器 ]                    │
│                                     │
│  同步间隔: [ 60 ] 分钟              │
│  超时时长: [  3 ] 秒                │
│  最大偏移警告阈值: [ 5000 ] ms      │
│                                     │
│  [x] 开机自动启动                   │
│  [x] 启动时立即同步一次             │
│  [x] 同步失败时显示气泡通知         │
│                                     │
│        [ 保存设置 ]  [恢复默认 ]     │
└─────────────────────────────────────┘
```

---

## 4. 技术架构

### 4.1 技术栈

| 层级 | 技术 | 选型理由 |
|------|------|----------|
| 语言 | C11 | 编译产物体积小、运行效率高、Windows API 原生兼容 |
| GUI | Win32 API | 零外部依赖、系统托盘原生支持、杀毒软件误报率最低 |
| 网络 | Winsock2 | Windows 标准套接字，无需第三方库 |
| 协议 | NTPv4 (RFC 5905) | 公共服务器广泛支持，精度优于 SNTP |
| 配置 | JSON-C / 自定义 ini | JSON 表达力强；如追求零依赖则使用 ini |
| 构建 | MinGW-w64 / MSVC | MinGW 支持 Linux 交叉编译，MSVC 支持 Windows 原生开发 |

### 4.2 模块依赖图

```
                    +------------------+
                    |   main.c         |
                    |  (WinMain 入口)  |
                    +--------+---------+
                             |
        +--------------------+--------------------+
        |                    |                    |
+-------v-------+   +--------v--------+   +------v------+
| tray_icon.c   |   | sync_engine.c   |   | config.c    |
| 系统托盘 GUI  |   | 同步调度引擎    |   | 配置读写    |
+-------+-------+   +--------+--------+   +------+------+
        |                    |                    |
        |    +---------------v---------------+    |
        |    |        ntp_client.c           |    |
        |    |   NTPv4 协议 (RFC 5905)       |    |
        |    +---------------+---------------+    |
        |                    |                    |
+-------v-------+   +--------v--------+   +------v------+
| time_adjust.c |   | log.c           |   | autostart.c |
| 系统时间调整  |   | 同步历史日志    |   | 开机自启    |
+---------------+   +-----------------+   +-------------+
```

### 4.3 项目目录结构

```
win-ntp-sync/
├── src/
│   ├── main.c              // WinMain、消息泵、全局初始化
│   ├── tray_icon.c/.h      // 托盘图标、菜单、气泡通知
│   ├── ntp_client.c/.h     // NTPv4 报文构造/解析、时间戳转换
│   ├── sync_engine.c/.h    // 定时器、多服务器轮询、状态机
│   ├── time_adjust.c/.h    // SetSystemTime、权限检查
│   ├── config.c/.h         // 配置文件读写、默认值
│   ├── log.c/.h            // 文件日志、轮转、格式化
│   ├── autostart.c/.h      // 注册表自启项管理
│   └── resource.rc         // 图标、版本信息、UAC manifest
├── include/
│   └── common.h            // 类型定义、常量、错误码、宏
├── docs/
│   └── product_spec.md     // 本文件
├── build/
│   └── Makefile            // MinGW-w64 交叉编译脚本
└── assets/
    └── icon.ico            // 应用图标（多分辨率）
```

---

## 5. NTP 协议交互设计

### 5.1 NTPv4 报文格式 (48 bytes)

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|LI | VN  |Mode |    Stratum     |     Poll      |  Precision   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Root Delay                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Root Dispersion                       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Reference ID                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                     Reference Timestamp (64)                  |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                      Origin Timestamp (64)                    |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                      Receive Timestamp (64)                   |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                      Transmit Timestamp (64)                  |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

**客户端发送字段设置**：
- `LI` = 0 (no warning)
- `VN` = 4 (NTPv4)
- `Mode` = 3 (client)
- `Transmit Timestamp` = 当前客户端时间 (NTP 时间戳格式)

### 5.2 交互时序

```
Client                                    NTP Server
   |                                          |
   | --- UDP/SNTP Request (port 123) -------> |
   |    T1: Originate Timestamp               |
   |                                          |
   | <--- UDP/SNTP Response (port 123) ------ |
   |    T2: Receive Timestamp (server)        |
   |    T3: Transmit Timestamp (server)       |
   |    T4: Arrival Timestamp (client)        |
   |                                          |
   |  计算:                                   |
   |    delay  = (T4 - T1) - (T3 - T2)        |
   |    offset = ((T2 - T1) + (T3 - T4)) / 2  |
```

### 5.3 时间戳转换

NTP 时间戳为 64-bit 定点数：
- 高 32-bit：自 1900-01-01 00:00:00 起的秒数
- 低 32-bit：秒的小数部分 (1/2^32 秒)

与 Windows `FILETIME`（自 1601-01-01 起 100ns 间隔）转换公式：

```c
// NTP 纪元 (1900) 到 Windows 纪元 (1601) 的差值 = 109207 days
#define NTP_TO_FILETIME_OFFSET 116444736000000000ULL

filetime = (ntp_seconds * 10000000ULL) + NTP_TO_FILETIME_OFFSET
         + (ntp_fraction * 10000000ULL / 4294967296ULL);
```

### 5.4 Kiss-o'-Death (KoD) 处理

当服务器返回 `stratum = 0` 时，`Reference ID` 为 KoD 字符串（如 `DENY`、`RSTR`）。

**处理策略**：
1. 解析 KoD 代码，记录到日志
2. 对该服务器启动退避计时器（默认 60 秒），期间不再向其发送请求
3. 立即尝试下一个备用服务器

---

## 6. 配置文件格式

**文件路径**：`%APPDATA%\WinTimeSync\config.ini`

```ini
[app]
version=1.0.0

[sync]
interval_minutes=60
timeout_seconds=3
max_offset_warning_ms=5000
sync_on_startup=1

[servers]
count=3
server_1=pool.ntp.org
server_2=time.windows.com
server_3=cn.pool.ntp.org

[log]
max_file_size_mb=1
max_files=5
path=logs

[notify]
on_success=0
on_failure=1
on_large_offset=1

[autostart]
enabled=0
```

**配置加载规则**：
1. 启动时读取配置文件，若不存在则使用硬编码默认值并创建默认配置
2. 用户在设置页点击"保存"后即时写回文件
3. 写回前校验服务器地址格式（域名或 IPv4/IPv6），无效则拒绝保存并提示

---

## 7. 错误处理与边界情况

| 场景 | 行为 | 用户反馈 |
|------|------|----------|
| 无管理员权限启动 | 尝试获取 `SE_SYSTEMTIME_NAME` 权限失败 | 托盘气泡 + 主窗口显示红色警告条："需要管理员权限以修改系统时间" |
| 所有服务器均不可达 | 日志记录全部超时，同步失败 | 气泡通知"时间同步失败：所有服务器无响应" |
| 偏移量 > 阈值 (默认 5s) | 仍执行 `SetSystemTime`，但标记为警告 | 气泡通知"时间偏差过大 (xx s)，已校准" |
| 网络断开 | UDP 发送失败或超时 | 静默失败，记录日志，等待下次定时触发 |
| 配置文件损坏 | JSON/ini 解析失败 | 重命名为 `config.ini.bak`，重建默认配置，气泡提示"配置已恢复默认" |
| 日志目录无写入权限 | `fopen` 失败 | 降级为不记录文件日志，主窗口状态栏显示"日志写入失败" |
| 系统时间被其他程序修改 | 无冲突，下次同步正常覆盖 | 无 |
| NTP 返回 stratum=16 (未同步) | 视为无效响应，切换服务器 | 记录日志 |

---

## 8. 安全设计

### 8.1 权限模型

- **必需权限**：`SE_SYSTEMTIME_NAME`（调整系统时间）
- **获取方式**：应用启动时通过 UAC 请求管理员权限
- **实现方式**：在 `resource.rc` 中嵌入 `requireAdministrator` 执行级别 manifest

```rc
1 RT_MANIFEST "app.manifest"
```

```xml
<!-- app.manifest -->
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <trustInfo xmlns="urn:schemas-microsoft-com:asm.v2">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level="requireAdministrator" uiAccess="false"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
</assembly>
```

### 8.2 杀毒软件兼容性策略

| 风险点 | 缓解措施 |
|--------|----------|
| 修改系统时间被视为可疑行为 | 通过标准 UAC 弹窗明确请求权限，不尝试绕过 |
| 网络通信被拦截 | 仅使用标准 UDP/123 端口，向知名公共服务器通信 |
| 系统托盘程序被 heuristic 检测 | 代码清晰无混淆、无加壳、无远程代码加载 |
| 注册表自启被标记为木马 | 仅写入 `HKCU\...\Run`，用户可通过设置页显式开关 |
| 缺少版本信息导致低信誉 | 资源文件内嵌完整 `VS_VERSION_INFO`（公司名、版本号、原始文件名） |
| 数字签名缺失 | 文档建议发布时使用代码签名证书（如 Let's Encrypt/商业证书） |

### 8.3 网络安全

- 不收集用户任何个人信息
- NTP 为无状态 UDP 协议，不传输敏感数据
- 不向非用户配置的服务器发送任何报文
- 代理支持（FR-12）如实现，仅作为可选功能，默认关闭

---

## 9. 测试计划

### 9.1 单元测试

针对无 GUI 依赖的核心模块编写独立测试：

| 模块 | 测试项 |
|------|--------|
| `ntp_client` | 时间戳转换精度（NTP <-> FILETIME <-> Unix）、报文构造/解析正确性、KoD 识别 |
| `time_adjust` | 权限检查逻辑、模拟 `SetSystemTime` 参数校验 |
| `config` | 默认值加载、自定义值读写、格式错误恢复 |
| `log` | 文件创建、内容格式、大小阈值切分、文件数上限 |
| `sync_engine` | 服务器列表轮询顺序、超时处理、状态机转换 |

**测试框架**：无外部依赖，使用自定义轻量断言宏，在 Linux 下用 GCC 编译运行（平台无关逻辑）。

### 9.2 集成测试

| 场景 | 验证内容 |
|------|----------|
| 本地回环测试 | 使用 `ntpdate` 或自定义假 NTP 服务器监听 123 端口，验证端到端报文收发与时间计算 |
| 公共服务器测试 | 连接 `pool.ntp.org`，验证实际同步精度（与 `w32tm` 结果对比） |
| 故障切换测试 | 配置一个无效 IP + 一个有效服务器，验证超时后自动切换 |
| 权限降级测试 | 以普通用户身份运行，验证权限检查失败后的 UI 状态 |

### 9.3 Windows 实机测试

| 环境 | 测试项 |
|------|--------|
| Windows 10 21H2 | 安装、启动、托盘图标、菜单、主窗口、同步、日志 |
| Windows 11 23H2 | 同上，重点验证高 DPI 下图标清晰度 |
| 带 360/火绒/Windows Defender | 确认无报毒、无拦截，首次启动 UAC 弹窗正常 |
| 低权限账户 | 确认管理员权限请求弹窗出现，拒绝后应用提示清晰 |

---

## 10. 构建与部署

### 10.1 开发环境

**Linux 交叉编译（推荐）**：
```bash
sudo apt install mingw-w64
make -C build CROSS=x86_64-w64-mingw32-
```

**Windows 本地编译**：
```powershell
# Visual Studio Developer Prompt
cl src\*.c /link user32.lib shell32.lib ws2_32.lib advapi32.lib
```

### 10.2 发布产物

```
WinTimeSync-1.0.0-x64/
├── WinTimeSync.exe       // 主程序（含 UAC manifest）
├── config.ini            // 默认配置（首次运行复制到 APPDATA）
├── logs/                 // 日志目录（运行时创建）
└── README.txt            // 快速使用说明
```

### 10.3 安装方式

- **便携版**：压缩包解压即用，配置与日志存于同级目录
- **安装版**（未来可选）：Inno Setup / NSIS 打包，写入 `Program Files`，配置存于 `%APPDATA%`

### 10.4 版本信息

资源文件中包含的 `VS_VERSION_INFO`：

```rc
VS_VERSION_INFO VERSIONINFO
FILEVERSION     1,0,0,0
PRODUCTVERSION  1,0,0,0
FILEFLAGSMASK   0x3fL
FILEFLAGS       0x0L
FILEOS          VOS_NT_WINDOWS32
FILETYPE        VFT_APP
FILESUBTYPE     0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904b0"
        BEGIN
            VALUE "CompanyName", "WinTimeSync Project"
            VALUE "FileDescription", "Windows NTP Time Synchronization"
            VALUE "FileVersion", "1.0.0.0"
            VALUE "InternalName", "WinTimeSync"
            VALUE "OriginalFilename", "WinTimeSync.exe"
            VALUE "ProductName", "WinTimeSync"
            VALUE "ProductVersion", "1.0.0"
        END
    END
END
```

---

## 11. 里程碑与排期

| 阶段 | 内容 | 预计工期 |
|------|------|----------|
| M1 | 产品开发文档（本文档）定稿 | 1 天 |
| M2 | 核心模块实现：NTP 客户端、时间调整、日志、配置 | 3 天 |
| M3 | GUI 实现：托盘图标、主窗口、设置页、气泡通知 | 3 天 |
| M4 | 集成：同步引擎、自动启动、定时器、故障切换 | 2 天 |
| M5 | 测试与调优：单元测试、Windows 实机测试、杀毒软件兼容性验证 | 3 天 |
| M6 | 打包发布：资源文件、图标、Makefile、README | 1 天 |

---

## 12. 附录

### 12.1 参考标准

- [RFC 5905] Network Time Protocol Version 4: Protocol and Algorithms Specification
- [RFC 4330] Simple Network Time Protocol (SNTP) Version 4 for IPv4, IPv6 and OSI
- Microsoft Docs: `SetSystemTime`, `Shell_NotifyIcon`, `NotifyIconData`

### 12.2 默认 NTP 服务器列表

| 服务器 | 地址 | 说明 |
|--------|------|------|
| NTP Pool Global | `pool.ntp.org` | 全球负载均衡 |
| Microsoft | `time.windows.com` | Windows 默认时间服务器 |
| NTP Pool China | `cn.pool.ntp.org` | 中国大陆节点 |
| Alibaba Cloud | `time.pool.aliyun.com` | 阿里云公共 NTP |
| Tencent Cloud | `time.tencentcloud.com` | 腾讯云公共 NTP |

### 12.3 术语表

| 术语 | 说明 |
|------|------|
| NTP | Network Time Protocol，网络时间协议 |
| SNTP | Simple NTP，简化版 NTP，精度较低 |
| Stratum | 时间源的层级，0=原子钟，1=直接与原子钟同步的服务器，2-15=逐级同步 |
| Offset | 本地时间与服务器时间的偏差 |
| Delay | 网络往返延迟 |
| KoD | Kiss-o'-Death，服务器要求客户端停止请求的响应包 |
| UAC | User Account Control，Windows 用户账户控制 |
