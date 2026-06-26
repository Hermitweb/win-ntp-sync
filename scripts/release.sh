#!/bin/bash
# ============================================================================
# WinTimeSync 发布脚本
# 构建可发布包，包含 EXE、文档、配置示例
# ============================================================================

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
DIST_DIR="$BUILD_DIR/dist"
RELEASE_DIR="$BUILD_DIR/release"

# 版本号
VERSION="1.0.0"
APP_NAME="WinTimeSync"
EXE_NAME="WinTimeSync.exe"
ARCH="${1:-x64}"

# 颜色
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info()  { echo -e "${BLUE}[INFO]${NC}  $1"; }
log_ok()    { echo -e "${GREEN}[ OK ]${NC}  $1"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC}  $1"; }
log_err()   { echo -e "${RED}[FAIL]${NC}  $1"; }

# ============================================================================
# 1. 准备阶段
# ============================================================================
log_info "准备发布目录..."
rm -rf "$DIST_DIR" "$RELEASE_DIR"
mkdir -p "$DIST_DIR"
mkdir -p "$RELEASE_DIR"

# ============================================================================
# 2. 选择编译器
# ============================================================================
if [[ "$ARCH" == "x64" ]]; then
    CROSS="x86_64-w64-mingw32-"
else
    CROSS="i686-w64-mingw32-"
fi

# 检查编译器
if ! command -v "${CROSS}gcc" &> /dev/null; then
    log_err "未找到编译器: ${CROSS}gcc"
    if [[ "$ARCH" == "x64" ]]; then
        log_info "请安装: sudo apt install gcc-mingw-w64-x86-64"
    else
        log_info "请安装: sudo apt install gcc-mingw-w64-i686"
    fi
    exit 1
fi
log_ok "使用编译器: ${CROSS}gcc"

# ============================================================================
# 3. 重新生成图标
# ============================================================================
log_info "检查图标资源..."
if [[ ! -f "$PROJECT_DIR/assets/icon_normal.ico" ]] || \
   [[ "$PROJECT_DIR/scripts/generate_icons.py" -nt "$PROJECT_DIR/assets/icon_normal.ico" ]]; then
    if command -v python3 &> /dev/null && python3 -c "from PIL import Image" 2>/dev/null; then
        log_info "重新生成图标..."
        python3 "$PROJECT_DIR/scripts/generate_icons.py" > /dev/null
        log_ok "图标已生成"
    else
        log_warn "未安装 Pillow，跳过图标生成（使用已有图标）"
    fi
fi

# ============================================================================
# 4. 编译 EXE
# ============================================================================
log_info "开始编译 $ARCH 版本..."
cd "$PROJECT_DIR"
make rebuild CROSS="$CROSS" 2>&1 | tail -5

if [[ ! -f "$BUILD_DIR/output/$EXE_NAME" ]]; then
    log_err "编译失败，未生成 $EXE_NAME"
    exit 1
fi

# 复制 EXE 到发布目录
cp "$BUILD_DIR/output/$EXE_NAME" "$DIST_DIR/"
log_ok "已生成: $DIST_DIR/$EXE_NAME"

# ============================================================================
# 5. 生成 ZIP 发布包
# ============================================================================
RELEASE_PKG_NAME="${APP_NAME}-v${VERSION}-${ARCH}"
RELEASE_PKG_DIR="$RELEASE_DIR/$RELEASE_PKG_NAME"
mkdir -p "$RELEASE_PKG_DIR"

# 复制主程序
cp "$DIST_DIR/$EXE_NAME" "$RELEASE_PKG_DIR/"

# 复制文档
cp "$PROJECT_DIR/README.md" "$RELEASE_PKG_DIR/"
cp "$PROJECT_DIR/LICENSE" "$RELEASE_PKG_DIR/"
cp "$PROJECT_DIR/CHANGELOG.md" "$RELEASE_PKG_DIR/"
cp "$PROJECT_DIR/docs/USAGE.md" "$RELEASE_PKG_DIR/"

# 创建配置示例文件
cat > "$RELEASE_PKG_DIR/config.example.ini" << 'EOF'
; ============================================================================
; WinTimeSync 配置文件示例
; 复制为 %APPDATA%\WinTimeSync\config.ini 即可生效
; ============================================================================

[Sync]
; 同步间隔（分钟）
IntervalMinutes=60
; 超时时间（秒）
TimeoutSeconds=3
; 偏移警告阈值（毫秒）
MaxOffsetWarningMs=5000
; 启动时自动同步
SyncOnStartup=false

[Servers]
; NTP 服务器列表（按优先级排序）
Server1=time.windows.com
Server2=pool.ntp.org
Server3=cn.pool.ntp.org
Server4=time.cloudflare.com
Server5=time1.aliyun.com
Server6=ntp.aliyun.com
Server7=time.asia.apple.com

[Log]
; 单个日志文件最大大小（MB）
MaxFileSizeMB=1
; 保留日志文件数
MaxFiles=5

[Notification]
OnSuccess=true
OnFailure=true
OnLargeOffset=true

[Startup]
AutostartEnabled=false
EOF

# 复制额外的资源（可选）
mkdir -p "$RELEASE_PKG_DIR/assets"
cp -r "$PROJECT_DIR/assets/"*.ico "$RELEASE_PKG_DIR/assets/" 2>/dev/null || true
cp -r "$PROJECT_DIR/assets/"*.png "$RELEASE_PKG_DIR/assets/" 2>/dev/null || true

# 复制批处理安装/卸载脚本
cp "$PROJECT_DIR/scripts/install.bat" "$RELEASE_PKG_DIR/" 2>/dev/null || true
cp "$PROJECT_DIR/scripts/uninstall.bat" "$RELEASE_PKG_DIR/" 2>/dev/null || true

# 验证 EXE 信息
log_info "EXE 文件信息："
file "$DIST_DIR/$EXE_NAME"

log_info "资源检查："
${CROSS}objdump -p "$DIST_DIR/$EXE_NAME" 2>/dev/null | grep -E "Manifest|Version" | head -3

# 压缩为 ZIP
cd "$RELEASE_DIR"
if command -v zip &> /dev/null; then
    zip -r "$RELEASE_PKG_NAME.zip" "$RELEASE_PKG_NAME/" > /dev/null
    log_ok "ZIP 包: $RELEASE_DIR/$RELEASE_PKG_NAME.zip"
elif command -v 7z &> /dev/null; then
    7z a "$RELEASE_PKG_NAME.zip" "$RELEASE_PKG_NAME/" > /dev/null
    log_ok "ZIP 包: $RELEASE_DIR/$RELEASE_PKG_NAME.zip"
else
    log_warn "未找到 zip 命令，跳过压缩"
    log_ok "目录包: $RELEASE_PKG_DIR/"
fi

# ============================================================================
# 6. 显示摘要
# ============================================================================
echo ""
echo "==============================================="
echo -e "${GREEN}✓ 发布完成${NC}"
echo "==============================================="
echo "版本: v$VERSION"
echo "架构: $ARCH"
echo "目录: $RELEASE_PKG_DIR"
echo "压缩包: $RELEASE_DIR/$RELEASE_PKG_NAME.zip"
echo ""
echo "包含文件："
ls -lh "$RELEASE_PKG_DIR/"
echo ""
