/**
 * @file sync_engine.h
 * @brief 同步引擎模块头文件
 * @details 提供定时器、多服务器轮询、故障切换、同步状态管理功能
 */

#ifndef WINTIMESYNC_SYNC_ENGINE_H
#define WINTIMESYNC_SYNC_ENGINE_H

#include "common.h"

/* ============================================================================
 * 定时器 ID 定义
 * ============================================================================ */

#define TIMER_ID_SYNC       1001
#define TIMER_ID_UPDATE_UI  1002

/* ============================================================================
 * 退避时间常量
 * ============================================================================ */

#define KOD_BACKOFF_SECONDS  60  /* Kiss-o'-Death 后退避时间 */
#define ERROR_BACKOFF_SECONDS 30 /* 错误后退避时间 */

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 初始化同步引擎
 * @return 成功返回 true，失败返回 false
 */
bool sync_engine_init(void);

/**
 * @brief 清理同步引擎
 */
void sync_engine_cleanup(void);

/**
 * @brief 启动定时同步
 * @param interval_ms 同步间隔（毫秒）
 */
void sync_engine_start_timer(int interval_ms);

/**
 * @brief 停止定时同步
 */
void sync_engine_stop_timer(void);

/**
 * @brief 执行单次同步
 * @details 尝试所有服务器直到成功或全部失败
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError sync_engine_do_sync(void);

/**
 * @brief 处理定时器消息
 * @param timer_id 定时器 ID
 */
void sync_engine_on_timer(UINT_PTR timer_id);

/**
 * @brief 获取当前同步状态
 * @return 同步状态
 */
SyncStatus sync_engine_get_status(void);

/**
 * @brief 获取上次同步结果
 * @param result 输出同步结果
 */
void sync_engine_get_last_result(SyncResult* result);

/**
 * @brief 检查服务器是否处于退避状态
 * @param server_index 服务器索引
 * @return 处于退避状态返回 true，否则返回 false
 */
bool sync_engine_is_server_in_backoff(int server_index);

/**
 * @brief 标记服务器为退避状态
 * @param server_index 服务器索引
 * @param seconds 退避时间（秒）
 */
void sync_engine_mark_server_backoff(int server_index, int seconds);

/**
 * @brief 清除服务器退避状态
 * @param server_index 服务器索引
 */
void sync_engine_clear_server_backoff(int server_index);

#endif /* WINTIMESYNC_SYNC_ENGINE_H */