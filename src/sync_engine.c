/**
 * @file sync_engine.c
 * @brief 同步引擎模块实现
 * @details 提供定时器、多服务器轮询、故障切换、同步状态管理功能
 */

#include "sync_engine.h"
#include "log.h"
#include "ntp_client.h"
#include "time_adjust.h"
#include "config.h"
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * 全局变量
 * ============================================================================ */

AppContext g_app = {0};

/* ============================================================================
 * 内部函数
 * ============================================================================ */

/**
 * @brief 选择下一个可用的服务器
 * @details 轮询服务器列表，跳过处于退避状态的服务器
 * @return 成功返回服务器索引，失败返回 -1
 */
static int select_next_server(void) {
    int start_index = g_app.current_server_index;

    /* 轮询所有服务器 */
    for (int i = 0; i < g_app.config.server_count; i++) {
        int index = (start_index + i) % g_app.config.server_count;

        /* 检查是否处于退避状态 */
        if (sync_engine_is_server_in_backoff(index)) {
            LOG_DEBUG("服务器 %d (%s) 处于退避状态，跳过",
                     index, g_app.config.servers[index].address);
            continue;
        }

        return index;
    }

    /* 所有服务器都在退避中 */
    LOG_WARNING("所有服务器均处于退避状态");
    return -1;
}

/**
 * @brief 标记服务器错误
 * @param server_index 服务器索引
 * @param error 错误码
 */
static void mark_server_error(int server_index, WtsError error) {
    if (server_index < 0 || server_index >= g_app.config.server_count) {
        return;
    }

    NtpServer* server = &g_app.config.servers[server_index];
    server->last_error_time = GetTickCount();

    /* 根据错误类型设置退避时间 */
    if (error == WTS_ERR_NTP_KISS_OF_DEATH) {
        server->status = SERVER_STATUS_KOD;
        sync_engine_mark_server_backoff(server_index, KOD_BACKOFF_SECONDS);
        LOG_WARNING("服务器 %s 收到 KoD，退避 %d 秒",
                   server->address, KOD_BACKOFF_SECONDS);
    } else {
        server->status = SERVER_STATUS_ERROR;
        sync_engine_mark_server_backoff(server_index, ERROR_BACKOFF_SECONDS);
        LOG_WARNING("服务器 %s 错误，退避 %d 秒",
                   server->address, ERROR_BACKOFF_SECONDS);
    }
}

/**
 * @brief 标记服务器正常
 * @param server_index 服务器索引
 */
static void mark_server_ok(int server_index) {
    if (server_index < 0 || server_index >= g_app.config.server_count) {
        return;
    }

    NtpServer* server = &g_app.config.servers[server_index];
    server->status = SERVER_STATUS_OK;
    sync_engine_clear_server_backoff(server_index);
}

/**
 * @brief 单次同步尝试
 * @param server_index 服务器索引
 * @param result 输出同步结果
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
static WtsError do_single_sync(int server_index, SyncResult* result) {
    if (server_index < 0 || server_index >= g_app.config.server_count) {
        return WTS_ERR_CONFIG_INVALID;
    }

    NtpServer* server = &g_app.config.servers[server_index];
    int timeout_ms = g_app.config.timeout_seconds * 1000;

    /* 执行 NTP 同步 */
    WtsError err = ntp_sync(server->address, timeout_ms, result);

    if (err == WTS_SUCCESS) {
        /* 成功 */
        mark_server_ok(server_index);
        g_app.current_server_index = server_index;
        return WTS_SUCCESS;
    }

    /* 失败，标记服务器错误 */
    mark_server_error(server_index, err);
    result->status = SYNC_STATUS_FAILURE;
    result->error_code = err;

    /* 尝试下一个服务器 */
    int next_server = select_next_server();
    if (next_server >= 0 && next_server != server_index) {
        LOG_INFO("切换到备用服务器: %s",
                g_app.config.servers[next_server].address);
        return do_single_sync(next_server, result);
    }

    /* 所有服务器都失败 */
    LOG_ERROR("所有服务器同步失败");
    return err;
}

/* ============================================================================
 * 公共函数
 * ============================================================================ */

bool sync_engine_init(void) {
    /* 初始化全局状态 */
    memset(&g_app, 0, sizeof(AppContext));
    g_app.current_status = SYNC_STATUS_IDLE;
    g_app.current_server_index = 0;
    g_app.is_initialized = false;

    /* 加载配置 */
    WtsError err = config_load(&g_app.config);
    if (err != WTS_SUCCESS) {
        LOG_ERROR("加载配置失败: %d", err);
        return false;
    }

    /* 初始化日志 */
    if (!log_init(g_app.config.log_path,
                  g_app.config.max_log_file_size_mb,
                  g_app.config.max_log_files)) {
        LOG_WARNING("日志初始化失败，继续运行");
    }

    /* 初始化 Winsock */
    err = ntp_init_winsock();
    if (err != WTS_SUCCESS) {
        LOG_ERROR("Winsock 初始化失败: %d", err);
        return false;
    }

    /* 检查权限 */
    g_app.has_privilege = time_check_privilege();
    if (!g_app.has_privilege) {
        LOG_WARNING("未启用系统时间权限，尝试启用");
        /* 尝试启用权限（管理员进程可成功） */
        if (time_enable_privilege()) {
            g_app.has_privilege = true;
            LOG_INFO("已成功启用系统时间调整权限");
        } else {
            LOG_WARNING("无法启用系统时间调整权限，需要管理员权限");
        }
    }

    g_app.is_initialized = true;
    LOG_INFO("同步引擎初始化完成");
    return true;
}

void sync_engine_cleanup(void) {
    if (!g_app.is_initialized) {
        return;
    }

    /* 停止定时器 */
    sync_engine_stop_timer();

    /* 清理 Winsock */
    ntp_cleanup_winsock();

    /* 清理日志 */
    log_cleanup();

    g_app.is_initialized = false;
    LOG_INFO("同步引擎清理完成");
}

void sync_engine_start_timer(int interval_ms) {
    if (!g_app.hwnd_tray) {
        LOG_WARNING("托盘窗口未创建，无法启动定时器");
        return;
    }

    SetTimer(g_app.hwnd_tray, TIMER_ID_SYNC, interval_ms, NULL);
    LOG_INFO("启动定时同步，间隔 %d ms", interval_ms);
}

void sync_engine_stop_timer(void) {
    if (!g_app.hwnd_tray) {
        return;
    }

    KillTimer(g_app.hwnd_tray, TIMER_ID_SYNC);
    LOG_INFO("停止定时同步");
}

WtsError sync_engine_do_sync(void) {
    if (!g_app.is_initialized) {
        return WTS_ERR_NOT_INITIALIZED;
    }

    /* 设置同步状态 */
    g_app.current_status = SYNC_STATUS_IN_PROGRESS;

    /* 选择服务器 */
    int server_index = select_next_server();
    if (server_index < 0) {
        g_app.current_status = SYNC_STATUS_FAILURE;
        g_app.last_sync.status = SYNC_STATUS_FAILURE;
        g_app.last_sync.error_code = WTS_ERR_CONFIG_INVALID;
        snprintf(g_app.last_sync.error_msg, sizeof(g_app.last_sync.error_msg),
                 "所有服务器处于退避状态");
        return WTS_ERR_CONFIG_INVALID;
    }

    /* 执行同步 */
    SyncResult result;
    WtsError err = do_single_sync(server_index, &result);

    /* 保存结果 */
    g_app.last_sync = result;

    /* 如果成功且有权限，调整时间 */
    if (err == WTS_SUCCESS && g_app.has_privilege) {
        err = time_adjust_from_sync(&result);

        /* 检查偏移量是否过大 */
        double abs_offset_ms = fabs(result.offset_ms);
        if (abs_offset_ms > g_app.config.max_offset_warning_ms) {
            g_app.current_status = SYNC_STATUS_WARNING;
            result.status = SYNC_STATUS_WARNING;
        } else {
            g_app.current_status = SYNC_STATUS_SUCCESS;
        }
    } else if (err != WTS_SUCCESS) {
        g_app.current_status = SYNC_STATUS_FAILURE;
    } else {
        g_app.current_status = SYNC_STATUS_SUCCESS;
    }

    /* 记录日志 */
    log_sync_result(&result);

    return err;
}

void sync_engine_on_timer(UINT_PTR timer_id) {
    if (timer_id == TIMER_ID_SYNC) {
        LOG_DEBUG("定时同步触发");
        sync_engine_do_sync();
    }
}

SyncStatus sync_engine_get_status(void) {
    return g_app.current_status;
}

void sync_engine_get_last_result(SyncResult* result) {
    if (result) {
        *result = g_app.last_sync;
    }
}

bool sync_engine_is_server_in_backoff(int server_index) {
    if (server_index < 0 || server_index >= g_app.config.server_count) {
        return false;
    }

    NtpServer* server = &g_app.config.servers[server_index];
    if (server->backoff_until == 0) {
        return false;
    }

    DWORD current_time = GetTickCount();
    return current_time < server->backoff_until;
}

void sync_engine_mark_server_backoff(int server_index, int seconds) {
    if (server_index < 0 || server_index >= g_app.config.server_count) {
        return;
    }

    NtpServer* server = &g_app.config.servers[server_index];
    server->backoff_until = GetTickCount() + (seconds * 1000);
    server->status = SERVER_STATUS_BACKOFF;
}

void sync_engine_clear_server_backoff(int server_index) {
    if (server_index < 0 || server_index >= g_app.config.server_count) {
        return;
    }

    NtpServer* server = &g_app.config.servers[server_index];
    server->backoff_until = 0;
    server->status = SERVER_STATUS_OK;
}