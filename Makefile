# WinTimeSync Makefile
# MinGW-w64 交叉编译脚本

# ============================================================================
# 编译器配置
# ============================================================================

# 交叉编译工具链前缀（Linux 下使用 MinGW-w64）
CROSS ?= x86_64-w64-mingw32-

CC = $(CROSS)gcc
CXX = $(CROSS)g++
RC = $(CROSS)windres
LD = $(CROSS)gcc

# ============================================================================
# 项目配置
# ============================================================================

PROJECT_NAME = WinTimeSync
VERSION = 1.0.0

# 源代码目录
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
ASSETS_DIR = $(CURDIR)/assets

# 输出目录
OUTPUT_DIR = $(BUILD_DIR)/output

# ============================================================================
# 编译选项
# ============================================================================

# C 标准
C_STD = -std=c11

# 警告级别
WARNINGS = -Wall -Wextra -Wno-format-truncation

# 优化级别
OPTIMIZE = -O2

# 定义
DEFINES = -D_WIN32_WINNT=0x0601

# 包含路径
INCLUDES = -I$(INCLUDE_DIR) -I$(SRC_DIR)

# 编译标志
CFLAGS = $(C_STD) $(WARNINGS) $(OPTIMIZE) -fexec-charset=GBK $(DEFINES) $(INCLUDES) -mwindows

# 链接库
LIBS = -luser32 -lshell32 -lws2_32 -ladvapi32 -lcomctl32

# ============================================================================
# 源文件
# ============================================================================

# C 源文件
C_SRCS = \
	$(SRC_DIR)/common.c \
	$(SRC_DIR)/log.c \
	$(SRC_DIR)/config.c \
	$(SRC_DIR)/ntp_client.c \
	$(SRC_DIR)/time_adjust.c \
	$(SRC_DIR)/sync_engine.c \
	$(SRC_DIR)/tray_icon.c \
	$(SRC_DIR)/autostart.c \
	$(SRC_DIR)/main.c

# 资源文件
RC_SRCS = $(SRC_DIR)/resource.rc

# 目标文件
C_OBJS = $(C_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
RC_OBJS = $(RC_SRCS:$(SRC_DIR)/%.rc=$(BUILD_DIR)/%.o)

# 最终可执行文件
TARGET = $(OUTPUT_DIR)/$(PROJECT_NAME).exe

# ============================================================================
# 目标规则
# ============================================================================

.PHONY: all clean rebuild dirs

all: dirs $(TARGET)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OUTPUT_DIR)

# 链接
$(TARGET): $(C_OBJS) $(RC_OBJS)
	$(LD) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $@"
	@echo "Manifest 已嵌入 exe 内部，无需外部 manifest 文件"

# 编译 C 源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# 编译资源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.rc $(wildcard $(ASSETS_DIR)/*.xml $(ASSETS_DIR)/*.ico)
	@mkdir -p $(BUILD_DIR)
	$(RC) -I "$(ASSETS_DIR)" -I "$(SRC_DIR)" -i "$<" -o "$@"

# ============================================================================
# 清理规则
# ============================================================================

clean:
	rm -rf $(BUILD_DIR)/output
	rm -rf $(BUILD_DIR)/*.o
	@echo "Clean complete"

rebuild: clean all

# ============================================================================
# 测试规则
# ============================================================================

.PHONY: test

test: all
	@echo "Testing compilation..."
	@if [ -f $(TARGET) ]; then \
		echo "Executable created successfully"; \
		file $(TARGET); \
		ls -lh $(TARGET); \
	else \
		echo "Build failed"; \
		exit 1; \
	fi

# ============================================================================
# 打包规则
# ============================================================================

.PHONY: package

PACKAGE_NAME = $(PROJECT_NAME)-$(VERSION)-x64

package: all
	@echo "Creating package..."
	@mkdir -p $(BUILD_DIR)/$(PACKAGE_NAME)
	@cp $(TARGET) $(BUILD_DIR)/$(PACKAGE_NAME)/
	@cp $(ASSETS_DIR)/app.manifest $(BUILD_DIR)/$(PACKAGE_NAME)/
	@cd $(BUILD_DIR) && tar -czf $(PACKAGE_NAME).tar.gz $(PACKAGE_NAME)
	@echo "Package created: $(BUILD_DIR)/$(PACKAGE_NAME).tar.gz"

# ============================================================================
# 辅助信息
# ============================================================================

.PHONY: info

info:
	@echo "Project: $(PROJECT_NAME)"
	@echo "Version: $(VERSION)"
	@echo "Compiler: $(CC)"
	@echo "Cross prefix: $(CROSS)"
	@echo "Target: $(TARGET)"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the project"
	@echo "  make clean        - Clean build files"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make test         - Build and test"
	@echo "  make package      - Create distribution package"
	@echo ""
	@echo "Cross-compilation example:"
	@echo "  make CROSS=x86_64-w64-mingw32-"