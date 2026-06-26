/**
 * @file common.h
 * @brief 公共类型定义、常量、错误码、宏
 * @details WinTimeSync 项目的通用定义，供所有模块使用
 */

#ifndef WINTIMESYNC_COMMON_H
#define WINTIMESYNC_COMMON_H

/* 必须先包含 winsock2.h 再包含 windows.h，避免警告 */
#include <winsock2.h>
#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

/* ============================================================================
 * 版本信息
 * ============================================================================ */

#define APP_NAME            "WinTimeSync"
#define APP_VERSION         "1.0.0"
#define APP_VERSION_NUM     1,0,0,0

/* ============================================================================
 * 路径与文件配置
 * ============================================================================ */

#define MAX_PATH_LEN        260
#define MAX_SERVER_LEN      256
#define MAX_SERVERS         10
#define CONFIG_FILE_NAME    "config.ini"
#define LOG_DIR_NAME        "logs"

/* ============================================================================
 * 同步配置默认值
 * ============================================================================ */

#define DEFAULT_SYNC_INTERVAL_MINUTES    60
#define DEFAULT_TIMEOUT_SECONDS           3
#define DEFAULT_MAX_OFFSET_WARNING_MS  5000
#define MAX_LOG_FILE_SIZE_MB              1
#define MAX_LOG_FILES                     5

/* ============================================================================
 * NTP 协议常量（RFC 5905）
 * ============================================================================ */

#define NTP_PORT               123
#define NTP_PACKET_SIZE        48
#define NTP_VERSION            4
#define NTP_MODE_CLIENT        3
#define NTP_MODE_SERVER        4
#define NTP_STRATUM_UNSPEC    16

/* NTP 纪元（1900-01-01）到 Windows 纪元（1601-01-01）的差值
 * 计算：109207 天 * 24小时 * 3600秒 * 10000000 (100ns) */
#define NTP_TO_FILETIME_OFFSET 116444736000000000ULL

/* ============================================================================
 * 错误码定义
 * ============================================================================ */

typedef enum {
    WTS_SUCCESS = 0,

    /* 配置相关错误 (100-199) */
    WTS_ERR_CONFIG_NOT_FOUND = 100,
    WTS_ERR_CONFIG_PARSE,
    WTS_ERR_CONFIG_WRITE,
    WTS_ERR_CONFIG_INVALID,

    /* 网络相关错误 (200-299) */
    WTS_ERR_SOCKET_INIT = 200,
    WTS_ERR_SOCKET_CREATE,
    WTS_ERR_SOCKET_CONNECT,
    WTS_ERR_SOCKET_SEND,
    WTS_ERR_SOCKET_RECV,
    WTS_ERR_SOCKET_TIMEOUT,
    WTS_ERR_DNS_RESOLVE,

    /* NTP 协议相关错误 (300-399) */
    WTS_ERR_NTP_INVALID_PACKET = 300,
    WTS_ERR_NTP_INVALID_VERSION,
    WTS_ERR_NTP_INVALID_MODE,
    WTS_ERR_NTP_KISS_OF_DEATH,
    WTS_ERR_NTP_INVALID_STRATUM,

    /* 时间调整相关错误 (400-499) */
    WTS_ERR_TIME_NO_PRIVILEGE = 400,
    WTS_ERR_TIME_ADJUST_FAILED,

    /* 日志相关错误 (500-599) */
    WTS_ERR_LOG_OPEN = 500,
    WTS_ERR_LOG_WRITE,
    WTS_ERR_LOG_ROTATE,

    /* 系统相关错误 (600-699) */
    WTS_ERR_REGISTRY = 600,
    WTS_ERR_AUTOSTART,

    /* 通用错误 (900-999) */
    WTS_ERR_UNKNOWN = 900,
    WTS_ERR_NULL_PARAM,
    WTS_ERR_BUFFER_TOO_SMALL,
    WTS_ERR_NOT_INITIALIZED,
} WtsError;

/* ============================================================================
 * 同步状态枚举
 * ============================================================================ */

typedef enum {
    SYNC_STATUS_IDLE,           /* 空闲状态 */
    SYNC_STATUS_IN_PROGRESS,     /* 同步中 */
    SYNC_STATUS_SUCCESS,         /* 成功 */
    SYNC_STATUS_FAILURE,         /* 失败 */
    SYNC_STATUS_WARNING,         /* 警告（偏移量过大等） */
} SyncStatus;

/* ============================================================================
 * 服务器状态枚举
 * ============================================================================ */

typedef enum {
    SERVER_STATUS_OK,            /* 正常 */
    SERVER_STATUS_TIMEOUT,       /* 超时 */
    SERVER_STATUS_ERROR,         /* 错误 */
    SERVER_STATUS_KOD,           /* Kiss-o'-Death */
    SERVER_STATUS_BACKOFF,       /* 退避中 */
} ServerStatus;

/* ============================================================================
 * 数据结构定义
 * ============================================================================ */

/**
 * @brief NTP 时间戳结构（64 位）
 */
typedef struct {
    uint32_t seconds;      /* 自 1900-01-01 以来的秒数 */
    uint32_t fraction;      /* 秒的小数部分 */
} NtpTimestamp;

/**
 * @brief NTP 报文结构（RFC 5905）
 */
#pragma pack(push, 1)
typedef struct {
    uint8_t  li_vn_mode;      /* LI(2) | VN(3) | Mode(3) */
    uint8_t  stratum;         /* 层级 */
    uint8_t  poll;            /* 轮询间隔 */
    int8_t   precision;       /* 精度 */
    uint32_t root_delay;      /* 根延迟 */
    uint32_t root_dispersion; /* 根离差 */
    uint32_t reference_id;    /* 参考 ID */

    NtpTimestamp ref_time;    /* 参考时间戳 */
    NtpTimestamp orig_time;   /* 发起时间戳 */
    NtpTimestamp recv_time;   /* 接收时间戳 */
    NtpTimestamp xmit_time;   /* 发送时间戳 */
} NtpPacket;
#pragma pack(pop)

/**
 * @brief 同步结果结构
 */
typedef struct {
    SyncStatus status;                    /* 同步状态 */
    char server[MAX_SERVER_LEN];          /* 服务器地址 */
    double offset_ms;                     /* 时间偏移量（毫秒） */
    double delay_ms;                      /* 往返延迟（毫秒） */
    FILETIME sync_time;                   /* 同步完成时间 */
    WtsError error_code;                  /* 错误码 */
    char error_msg[256];                  /* 错误消息 */
} SyncResult;

/**
 * @brief NTP 服务器配置结构
 */
typedef struct {
    char address[MAX_SERVER_LEN];         /* 服务器地址 */
    ServerStatus status;                  /* 当前状态 */
    DWORD last_error_time;                /* 上次错误时间 */
    DWORD backoff_until;                  /* 退避结束时间 */
} NtpServer;

/**
 * @brief 应用配置结构
 */
typedef struct {
    /* 同步配置 */
    int sync_interval_minutes;            /* 同步间隔（分钟） */
    int timeout_seconds;                  /* 超时时间（秒） */
    int max_offset_warning_ms;            /* 最大偏移警告阈值（毫秒） */
    bool sync_on_startup;                 /* 启动时同步 */

    /* 服务器列表 */
    NtpServer servers[MAX_SERVERS];       /* 服务器列表 */
    int server_count;                     /* 服务器数量 */

    /* 日志配置 */
    int max_log_file_size_mb;             /* 单个日志文件最大大小 */
    int max_log_files;                    /* 保留日志文件数量 */
    char log_path[MAX_PATH_LEN];          /* 日志路径 */

    /* 通知配置 */
    bool notify_on_success;               /* 成功时通知 */
    bool notify_on_failure;               /* 失败时通知 */
    bool notify_on_large_offset;         /* 偏移过大时通知 */

    /* 自启动配置 */
    bool autostart_enabled;               /* 开机自启 */
} AppConfig;

/**
 * @brief 全局应用状态结构
 */
typedef struct {
    AppConfig config;                     /* 配置 */
    SyncResult last_sync;                 /* 上次同步结果 */
    SyncStatus current_status;            /* 当前同步状态 */
    int current_server_index;             /* 当前使用的服务器索引 */
    HWND hwnd_main;                       /* 主窗口句柄 */
    HWND hwnd_tray;                       /* 托盘消息窗口句柄 */
    bool is_initialized;                  /* 是否已初始化 */
    bool has_privilege;                   /* 是否有时间调整权限 */
} AppContext;

/* ============================================================================
 * 全局变量声明
 * ============================================================================ */

extern AppContext g_app;

/* ============================================================================
 * 宏定义
 * ============================================================================ */

/* 数组长度 */
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))

/* 安全字符串复制 */
#define STR_COPY(dest, src)  do { \
    strncpy_s(dest, sizeof(dest), src, _TRUNCATE); \
} while(0)

/* 调试日志（仅在 Debug 模式） */
#ifdef _DEBUG
#define DEBUG_PRINT(fmt, ...) \
    fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif

/* 最小/最大值 */
#define WTS_MIN(a, b)       ((a) < (b) ? (a) : (b))
#define WTS_MAX(a, b)       ((a) > (b) ? (a) : (b))

/* 时间转换辅助 */
#define MS_TO_SEC(ms)       ((ms) / 1000)
#define SEC_TO_MS(sec)      ((sec) * 1000)
#define MIN_TO_MS(min)      ((min) * 60 * 1000)
#define MIN_TO_SEC(min)     ((min) * 60)

/* ============================================================================
 * 工具函数声明
 * ============================================================================ */

/**
 * @brief 获取应用数据目录路径
 * @param path 输出路径缓冲区
 * @param size 缓冲区大小
 * @return 成功返回 true，失败返回 false
 */
bool get_appdata_path(char* path, size_t size);

/**
 * @brief 获取配置文件完整路径
 * @param path 输出路径缓冲区
 * @param size 缓冲区大小
 * @return 成功返回 true，失败返回 false
 */
bool get_config_path(char* path, size_t size);

/**
 * @brief 获取日志目录完整路径
 * @param path 输出路径缓冲区
 * @param size 缓冲区大小
 * @return 成功返回 true，失败返回 false
 */
bool get_log_path(char* path, size_t size);

/**
 * @brief 获取错误消息字符串
 * @param error 错误码
 * @return 错误消息字符串
 */
const char* get_error_message(WtsError error);

#endif /* WINTIMESYNC_COMMON_H */