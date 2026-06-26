/**
 * @file autostart.h
 * @brief 自启动模块头文件
 * @details 提供注册表 HKCU 自启动项管理功能
 */

#ifndef WINTIMESYNC_AUTOSTART_H
#define WINTIMESYNC_AUTOSTART_H

#include "common.h"

/* ============================================================================
 * 注册表路径常量
 * ============================================================================ */

#define AUTOSTART_REG_PATH \
    "Software\\Microsoft\\Windows\\CurrentVersion\\Run"

#define AUTOSTART_REG_KEY   "WinTimeSync"

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 检查是否已设置开机自启
 * @return 已设置返回 true，否则返回 false
 */
bool autostart_is_enabled(void);

/**
 * @brief 启用开机自启
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError autostart_enable(void);

/**
 * @brief 禁用开机自启
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError autostart_disable(void);

/**
 * @brief 设置开机自启状态
 * @param enable 启用或禁用
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError autostart_set(bool enable);

#endif /* WINTIMESYNC_AUTOSTART_H */