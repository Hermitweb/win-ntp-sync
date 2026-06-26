/**
 * @file log.c
 * @brief 日志模块实现
 * @details 提供文件日志、滚动轮转、格式化输出功能
 */

#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <direct.h>

/* ============================================================================
 * 内部常量
 * ============================================================================ */

#define LOG_FILENAME_PREFIX "wintimesync"
#define LOG_FILENAME_EXT    ".log"
#define LOG_FILENAME_FORMAT "%s_%04d%02d%02d.log"

/* ============================================================================
 * 内部结构
 * ============================================================================ */

typedef struct {
    char log_dir[MAX_PATH_LEN];        /* 日志目录路径 */
    char current_file[MAX_PATH_LEN];  /* 当前日志文件路径 */
    FILE* fp;                          /* 当前日志文件句柄 */
    int max_file_size;                 /* 最大文件大小（字节） */
    int max_files;                     /* 保留文件数量 */
    bool initialized;                  /* 是否已初始化 */
    CRITICAL_SECTION lock;             /* 线程锁 */
} LogContext;

/* ============================================================================
 * 全局变量
 * ============================================================================ */

static LogContext g_log = {0};

/* ============================================================================
 * 日志级别字符串
 * ============================================================================ */

static const char* level_strings[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR"
};

/* ============================================================================
 * 内部函数
 * ============================================================================ */

/**
 * @brief 获取当前日期
 * @param year 输出年份
 * @param month 输出月份
 * @param day 输出日期
 */
static void get_current_date(int* year, int* month, int* day) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    *year = st.wYear;
    *month = st.wMonth;
    *day = st.wDay;
}

/**
 * @brief 获取当前时间戳字符串
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 */
static void get_timestamp(char* buffer, size_t size) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(buffer, size, "%04u-%02u-%02u %02u:%02u:%02u.%03u",
             (unsigned)st.wYear, (unsigned)st.wMonth, (unsigned)st.wDay,
             (unsigned)st.wHour, (unsigned)st.wMinute, (unsigned)st.wSecond,
             (unsigned)st.wMilliseconds);
}

/**
 * @brief 检查并创建目录
 * @param path 目录路径
 * @return 成功返回 true，失败返回 false
 */
static bool ensure_directory(const char* path) {
    struct _stat st;
    if (_stat(path, &st) == 0) {
        return (st.st_mode & _S_IFDIR) != 0;
    }
    return _mkdir(path) == 0 || errno == EEXIST;
}

/**
 * @brief 获取文件大小
 * @param filepath 文件路径
 * @return 文件大小（字节），失败返回 -1
 */
static long get_file_size(const char* filepath) {
    struct _stat st;
    if (_stat(filepath, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/**
 * @brief 生成日志文件名
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @param year 年份
 * @param month 月份
 * @param day 日期
 */
static void generate_log_filename(char* buffer, size_t size, int year, int month, int day) {
    snprintf(buffer, size, LOG_FILENAME_FORMAT, LOG_FILENAME_PREFIX, year, month, day);
}

/**
 * @brief 比较函数，用于排序日志文件（按修改时间降序）
 */
static int compare_log_files(const void* a, const void* b) {
    /* 简化实现：按文件名字符串比较 */
    return strcmp(*(const char**)b, *(const char**)a);
}

/**
 * @brief 清理旧日志文件
 * @details 保留最新的 max_files 个日志文件
 */
static void cleanup_old_logs(void) {
    char search_pattern[MAX_PATH_LEN];
    char filepath[MAX_PATH_LEN];
    WIN32_FIND_DATAA find_data;
    HANDLE h_find;
    char** files = NULL;
    int file_count = 0;
    int capacity = 0;

    /* 构建搜索模式 */
    snprintf(search_pattern, sizeof(search_pattern), "%s\\%s*%s",
             g_log.log_dir, LOG_FILENAME_PREFIX, LOG_FILENAME_EXT);

    /* 查找所有日志文件 */
    h_find = FindFirstFileA(search_pattern, &find_data);
    if (h_find == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        if (file_count >= capacity) {
            capacity = capacity == 0 ? 10 : capacity * 2;
            char** new_files = realloc(files, capacity * sizeof(char*));
            if (!new_files) {
                FindClose(h_find);
                free(files);
                return;
            }
            files = new_files;
        }

        files[file_count] = _strdup(find_data.cFileName);
        if (files[file_count]) {
            file_count++;
        }
    } while (FindNextFileA(h_find, &find_data));

    FindClose(h_find);

    /* 如果文件数量超过限制，删除最旧的 */
    if (file_count > g_log.max_files) {
        /* 排序文件（按文件名降序，最新的在前） */
        qsort(files, file_count, sizeof(char*), compare_log_files);

        /* 删除多余的文件 */
        for (int i = g_log.max_files; i < file_count; i++) {
            snprintf(filepath, sizeof(filepath), "%s\\%s",
                     g_log.log_dir, files[i]);
            DeleteFileA(filepath);
        }
    }

    /* 释放内存 */
    for (int i = 0; i < file_count; i++) {
        free(files[i]);
    }
    free(files);
}

/**
 * @brief 执行日志轮转
 * @return 成功返回 true，失败返回 false
 */
static bool do_log_rotation(void) {
    /* 关闭当前文件 */
    if (g_log.fp) {
        fclose(g_log.fp);
        g_log.fp = NULL;
    }

    /* 清理旧日志 */
    cleanup_old_logs();

    return true;
}

/* ============================================================================
 * 公共函数
 * ============================================================================ */

bool log_init(const char* log_path, int max_file_size_mb, int max_files) {
    if (g_log.initialized) {
        return true;
    }

    /* 设置日志目录 */
    if (log_path && strlen(log_path) > 0) {
        strncpy_s(g_log.log_dir, sizeof(g_log.log_dir), log_path, _TRUNCATE);
    } else {
        /* 使用默认路径 */
        if (!get_log_path(g_log.log_dir, sizeof(g_log.log_dir))) {
            return false;
        }
    }

    /* 确保目录存在 */
    if (!ensure_directory(g_log.log_dir)) {
        return false;
    }

    /* 设置参数 */
    g_log.max_file_size = max_file_size_mb * 1024 * 1024;
    g_log.max_files = WTS_MAX(1, max_files);

    /* 初始化线程锁 */
    InitializeCriticalSection(&g_log.lock);

    g_log.initialized = true;
    return true;
}

void log_cleanup(void) {
    if (!g_log.initialized) {
        return;
    }

    EnterCriticalSection(&g_log.lock);

    if (g_log.fp) {
        fclose(g_log.fp);
        g_log.fp = NULL;
    }

    LeaveCriticalSection(&g_log.lock);
    DeleteCriticalSection(&g_log.lock);

    g_log.initialized = false;
}

void log_write(LogLevel level, const char* file, int line, const char* func,
               const char* fmt, ...) {
    if (!g_log.initialized) {
        return;
    }

    EnterCriticalSection(&g_log.lock);

    /* 获取当前日期 */
    int year, month, day;
    get_current_date(&year, &month, &day);

    /* 生成日志文件名 */
    char log_filename[MAX_PATH_LEN];
    generate_log_filename(log_filename, sizeof(log_filename), year, month, day);

    /* 构建完整路径 */
    char log_filepath[MAX_PATH_LEN];
    snprintf(log_filepath, sizeof(log_filepath), "%s\\%s",
             g_log.log_dir, log_filename);

    /* 如果文件路径改变或文件未打开，打开新文件 */
    if (!g_log.fp || strcmp(g_log.current_file, log_filepath) != 0) {
        if (g_log.fp) {
            fclose(g_log.fp);
        }
        g_log.fp = fopen(log_filepath, "a");
        if (!g_log.fp) {
            LeaveCriticalSection(&g_log.lock);
            return;
        }
        strncpy_s(g_log.current_file, sizeof(g_log.current_file),
                   log_filepath, _TRUNCATE);
    }

    /* 检查文件大小 */
    long file_size = get_file_size(log_filepath);
    if (file_size > 0 && file_size >= g_log.max_file_size) {
        do_log_rotation();
        g_log.fp = fopen(log_filepath, "a");
        if (!g_log.fp) {
            LeaveCriticalSection(&g_log.lock);
            return;
        }
    }

    /* 格式化日志消息 */
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    /* 提取文件名（不含路径） */
    const char* filename = strrchr(file, '\\');
    if (!filename) {
        filename = file;
    } else {
        filename++;
    }

    /* 写入日志头 */
    fprintf(g_log.fp, "[%s] [%s] [%s:%d %s] ",
            timestamp, level_strings[level], filename, line, func);

    /* 写入日志内容 */
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log.fp, fmt, args);
    va_end(args);

    /* 写入换行 */
    fprintf(g_log.fp, "\n");

    /* 刷新缓冲区 */
    fflush(g_log.fp);

    LeaveCriticalSection(&g_log.lock);
}

void log_sync_result(const SyncResult* result) {
    if (!result) {
        return;
    }

    const char* status_str;
    switch (result->status) {
        case SYNC_STATUS_SUCCESS:
            status_str = "成功";
            break;
        case SYNC_STATUS_FAILURE:
            status_str = "失败";
            break;
        case SYNC_STATUS_WARNING:
            status_str = "警告";
            break;
        default:
            status_str = "未知";
    }

    LOG_INFO("同步结果: %s | 服务器: %s | 偏移: %.2f ms | 延迟: %.2f ms",
             status_str, result->server, result->offset_ms, result->delay_ms);

    if (result->status == SYNC_STATUS_FAILURE && result->error_msg[0]) {
        LOG_ERROR("错误: %s", result->error_msg);
    }
}

bool log_rotate(void) {
    if (!g_log.initialized) {
        return false;
    }

    EnterCriticalSection(&g_log.lock);
    bool result = do_log_rotation();
    LeaveCriticalSection(&g_log.lock);

    return result;
}