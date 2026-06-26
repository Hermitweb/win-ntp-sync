/**
 * @file time_adjust.h
 * @brief 时间调整模块头文件
 * @details 提供 SetSystemTime API 封装、权限检查功能
 */

#ifndef WINTIMESYNC_TIME_ADJUST_H
#define WINTIMESYNC_TIME_ADJUST_H

#include "common.h"

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 检查是否有 SE_SYSTEMTIME_NAME 权限
 * @return 有权限返回 true，无权限返回 false
 */
bool time_check_privilege(void);

/**
 * @brief 启用 SE_SYSTEMTIME_NAME 权限
 * @return 成功返回 true，失败返回 false
 */
bool time_enable_privilege(void);

/**
 * @brief 调整系统时间
 * @param filetime 新的文件时间
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError time_adjust(const FILETIME* filetime);

/**
 * @brief 根据 NTP 同步结果调整系统时间
 * @param sync_result NTP 同步结果
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError time_adjust_from_sync(const SyncResult* sync_result);

/**
 * @brief 获取当前系统时间
 * @param filetime 输出文件时间
 */
void time_get_current(FILETIME* filetime);

/**
 * @brief FILETIME 转换为 SYSTEMTIME
 * @param filetime FILETIME 时间戳
 * @param systemtime 输出 SYSTEMTIME
 */
void filetime_to_systemtime(const FILETIME* filetime, SYSTEMTIME* systemtime);

/**
 * @brief SYSTEMTIME 转换为 FILETIME
 * @param systemtime SYSTEMTIME 时间
 * @param filetime 输出 FILETIME
 */
void systemtime_to_filetime(const SYSTEMTIME* systemtime, FILETIME* filetime);

#endif /* WINTIMESYNC_TIME_ADJUST_H */