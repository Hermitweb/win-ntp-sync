/**
 * @file tray_icon.h
 * @brief 系统托盘模块头文件
 * @details 提供系统托盘图标、右键菜单、气泡通知功能
 */

#ifndef WINTIMESYNC_TRAY_ICON_H
#define WINTIMESYNC_TRAY_ICON_H

#include "common.h"

/* ============================================================================
 * 消息定义
 * ============================================================================ */

#define WM_TRAY_ICON           (WM_USER + 100)
#define WM_TRAY_MENU_SYNC      (WM_USER + 101)
#define WM_TRAY_MENU_SHOW      (WM_USER + 102)
#define WM_TRAY_MENU_SETTINGS  (WM_USER + 103)
#define WM_TRAY_MENU_LOG       (WM_USER + 104)
#define WM_TRAY_MENU_ABOUT     (WM_USER + 105)
#define WM_TRAY_MENU_EXIT      (WM_USER + 106)

/* ============================================================================
 * 菜单项 ID
 * ============================================================================ */

#define IDM_SYNC_NOW           2001
#define IDM_SHOW_WINDOW        2002
#define IDM_SETTINGS           2003
#define IDM_VIEW_LOG           2004
#define IDM_ABOUT              2005
#define IDM_EXIT               2006

/* ============================================================================
 * 图标状态
 * ============================================================================ */

typedef enum {
    TRAY_ICON_NORMAL,    /* 正常 - 绿色 */
    TRAY_ICON_WARNING,   /* 偏移过大 - 黄色 */
    TRAY_ICON_ERROR,     /* 失败 - 红色 */
    TRAY_ICON_SYNCING,   /* 同步中 - 动画 */
} TrayIconStatus;

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 创建托盘图标窗口
 * @return 成功返回窗口句柄，失败返回 NULL
 */
HWND tray_icon_create_window(void);

/**
 * @brief 初始化托盘图标
 * @param hwnd 托盘消息窗口句柄
 * @return 成功返回 true，失败返回 false
 */
bool tray_icon_init(HWND hwnd);

/**
 * @brief 移除托盘图标
 */
void tray_icon_remove(void);

/**
 * @brief 更新托盘图标状态
 * @param status 图标状态
 */
void tray_icon_update_status(TrayIconStatus status);

/**
 * @brief 显示托盘菜单
 * @param hwnd 窗口句柄
 * @param x 菜单显示位置 x
 * @param y 菜单显示位置 y
 */
void tray_icon_show_menu(HWND hwnd, int x, int y);

/**
 * @brief 显示气球通知
 * @param title 通知标题
 * @param message 通知消息
 * @param flags 通知标志（NIIF_INFO、NIIF_WARNING、NIIF_ERROR）
 */
void tray_icon_show_balloon(const char* title, const char* message, DWORD flags);

/**
 * @brief 更新托盘图标提示文本
 * @param text 提示文本
 */
void tray_icon_update_tip(const char* text);

/**
 * @brief 根据同步状态更新托盘图标
 * @param sync_status 同步状态
 * @param result 同步结果
 */
void tray_icon_update_from_sync(SyncStatus sync_status, const SyncResult* result);

#endif /* WINTIMESYNC_TRAY_ICON_H */