/**
 * @file log.h
 * @brief 日志模块头文件
 * @details 提供文件日志、滚动轮转、格式化输出功能
 */

#ifndef WINTIMESYNC_LOG_H
#define WINTIMESYNC_LOG_H

#include "common.h"

/* ============================================================================
 * 日志级别定义
 * ============================================================================ */

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
} LogLevel;

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 初始化日志系统
 * @param log_path 日志目录路径，若为 NULL 则使用默认路径
 * @param max_file_size_mb 单个日志文件最大大小（MB）
 * @param max_files 保留日志文件数量
 * @return 成功返回 true，失败返回 false
 */
bool log_init(const char* log_path, int max_file_size_mb, int max_files);

/**
 * @brief 关闭日志系统
 */
void log_cleanup(void);

/**
 * @brief 写入日志
 * @param level 日志级别
 * @param file 源文件名
 * @param line 行号
 * @param func 函数名
 * @param fmt 格式化字符串
 * @param ... 可变参数
 */
void log_write(LogLevel level, const char* file, int line, const char* func,
               const char* fmt, ...);

/**
 * @brief 记录同步结果日志
 * @param result 同步结果
 */
void log_sync_result(const SyncResult* result);

/**
 * @brief 强制执行日志轮转
 * @return 成功返回 true，失败返回 false
 */
bool log_rotate(void);

/* ============================================================================
 * 便捷宏
 * ============================================================================ */

#define LOG_DEBUG(fmt, ...) \
    log_write(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    log_write(LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_WARNING(fmt, ...) \
    log_write(LOG_LEVEL_WARNING, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    log_write(LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

#endif /* WINTIMESYNC_LOG_H */