/**
 * @file tray_icon.c
 * @brief 系统托盘与窗口模块实现
 * @details 提供系统托盘图标、右键菜单、气泡通知、主窗口、设置窗口功能
 */

#include "tray_icon.h"
#include "log.h"
#include "sync_engine.h"
#include "config.h"
#include "resource.h"
#include "autostart.h"
#include <stdio.h>
#include <math.h>

/* ============================================================================
 * 常量定义
 * ============================================================================
 */

#define TRAY_TIP_MAX_LEN      128
#define TRAY_BALLOON_TITLE_MAX 64
#define TRAY_BALLOON_MSG_MAX   256

#define MAIN_WINDOW_WIDTH    480
#define MAIN_WINDOW_HEIGHT   380
#define SETTINGS_WIDTH       420
#define SETTINGS_HEIGHT      480
#define ABOUT_WIDTH          320
#define ABOUT_HEIGHT         220

/* 控件 ID */
#define IDC_STATIC_STATUS     1001
#define IDC_BTN_SYNC_NOW      1002
#define IDC_TAB_SETTINGS      1003
#define IDC_EDIT_INTERVAL     1004
#define IDC_EDIT_TIMEOUT      1005
#define IDC_EDIT_OFFSET       1006
#define IDC_CHK_AUTOSTART     1007
#define IDC_CHK_STARTUP_SYNC  1008
#define IDC_CHK_NOTIFY_FAIL   1009
#define IDC_CHK_NOTIFY_SUCCESS 1010
#define IDC_CHK_NOTIFY_OFFSET 1011
#define IDC_BTN_SAVE          1012
#define IDC_LIST_SERVERS      1013
#define IDC_BTN_ADD_SERVER    1014
#define IDC_BTN_DEL_SERVER    1015
#define IDC_EDIT_SERVER       1016
#define IDC_TXT_LOG           1017

/* ============================================================================
 * 内部变量
 * ============================================================================
 */

static NOTIFYICONDATAA g_nid = {0};
static HWND g_tray_hwnd = NULL;
static HMENU g_tray_menu = NULL;
static TrayIconStatus g_current_status = TRAY_ICON_NORMAL;

static HWND g_hwnd_main = NULL;
static HWND g_hwnd_settings = NULL;
static HWND g_hwnd_about = NULL;
static HWND g_hwnd_log = NULL;

/* ============================================================================
 * 内部函数声明
 * ============================================================================
 */

static HMENU create_tray_menu(void);
static LRESULT CALLBACK tray_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK main_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK settings_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK about_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK log_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void show_main_window(void);
static void show_settings_window(void);
static void show_about_window(void);
static void show_log_window(void);
static void update_main_window_status(HWND hwnd);
static void center_window(HWND hwnd);

/* ============================================================================
 * 托盘菜单
 * ============================================================================
 */

static HMENU create_tray_menu(void) {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        LOG_ERROR("创建菜单失败: %d", GetLastError());
        return NULL;
    }

    AppendMenuA(menu, MF_STRING, IDM_SYNC_NOW, "立即同步\tCtrl+S");
    AppendMenuA(menu, MF_STRING, IDM_SHOW_WINDOW, "显示主窗口");
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);

    char status_text[128];
    SyncResult result;
    sync_engine_get_last_result(&result);

    if (result.status != SYNC_STATUS_IDLE) {
        SYSTEMTIME st;
        FileTimeToSystemTime((FILETIME*)&result.sync_time, &st);
        snprintf(status_text, sizeof(status_text),
                 "上次同步: %04d-%02d-%02d %02d:%02d",
                 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
        AppendMenuA(menu, MF_STRING | MF_GRAYED, 0, status_text);

        snprintf(status_text, sizeof(status_text),
                 "偏移量: %.1f ms", result.offset_ms);
        AppendMenuA(menu, MF_STRING | MF_GRAYED, 0, status_text);

        snprintf(status_text, sizeof(status_text),
                 "服务器: %.60s", result.server);
        AppendMenuA(menu, MF_STRING | MF_GRAYED, 0, status_text);
    } else {
        AppendMenuA(menu, MF_STRING | MF_GRAYED, 0, "尚未同步");
    }

    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, IDM_SETTINGS, "设置...");
    AppendMenuA(menu, MF_STRING, IDM_VIEW_LOG, "查看日志");
    AppendMenuA(menu, MF_STRING, IDM_ABOUT, "关于");
    AppendMenuA(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(menu, MF_STRING, IDM_EXIT, "退出");

    return menu;
}

/* ============================================================================
 * 图标资源
 * ============================================================================
 */

static int get_icon_resource_id(TrayIconStatus status) {
    switch (status) {
        case TRAY_ICON_NORMAL:  return IDI_ICON_NORMAL;
        case TRAY_ICON_WARNING: return IDI_ICON_WARNING;
        case TRAY_ICON_ERROR:   return IDI_ICON_ERROR;
        case TRAY_ICON_SYNCING: return IDI_ICON_SYNCING;
        default:                return IDI_ICON_NORMAL;
    }
}

/* ============================================================================
 * 窗口居中
 * ============================================================================
 */

static void center_window(HWND hwnd) {
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hwnd, NULL, (sw - w) / 2, (sh - h) / 2, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER);
}

/* ============================================================================
 * 托盘窗口过程
 * ============================================================================
 */

static LRESULT CALLBACK tray_window_proc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAY_ICON:
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP: {
                    POINT pt;
                    GetCursorPos(&pt);
                    tray_icon_show_menu(hwnd, pt.x, pt.y);
                    break;
                }
                case WM_LBUTTONDBLCLK:
                    show_main_window();
                    break;
                case NIN_BALLOONUSERCLICK:
                    show_main_window();
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_SYNC_NOW:
                    sync_engine_do_sync();
                    break;
                case IDM_SHOW_WINDOW:
                    show_main_window();
                    break;
                case IDM_SETTINGS:
                    show_settings_window();
                    break;
                case IDM_VIEW_LOG:
                    show_log_window();
                    break;
                case IDM_ABOUT:
                    show_about_window();
                    break;
                case IDM_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            break;

        case WM_TIMER:
            sync_engine_on_timer(wParam);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    return 0;
}

/* ============================================================================
 * 主窗口过程
 * ============================================================================
 */

static LRESULT CALLBACK main_window_proc(HWND hwnd, UINT msg,
                                          WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HFONT hfont = GetStockObject(DEFAULT_GUI_FONT);

            CreateWindowA("STATIC", "WinTimeSync - 时间同步状态",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         20, 15, 440, 25, hwnd, (HMENU)IDC_STATIC_STATUS, NULL, NULL);
            SendDlgItemMessage(hwnd, IDC_STATIC_STATUS, WM_SETFONT, (WPARAM)hfont, TRUE);

            char info[2048];
            SyncResult result;
            sync_engine_get_last_result(&result);

            SYSTEMTIME st_now, st_sync;
            GetLocalTime(&st_now);

            if (result.status != SYNC_STATUS_IDLE) {
                FILETIME ft;
                FileTimeToLocalFileTime((FILETIME*)&result.sync_time, &ft);
                FileTimeToSystemTime(&ft, &st_sync);
                snprintf(info, sizeof(info),
                    "当前系统时间: %04d-%02d-%02d %02d:%02d:%02d\r\n\r\n"
                    "上次同步时间: %04d-%02d-%02d %02d:%02d:%02d\r\n"
                    "参考服务器  : %s\r\n"
                    "时间偏移量  : %.1f ms\r\n"
                    "往返延迟    : %.1f ms\r\n"
                    "同步结果    : %s\r\n"
                    "\r\n"
                    "下次自动同步: 约 %d 分钟后\r\n"
                    "\r\n"
                    "权限状态    : %s",
                    st_now.wYear, st_now.wMonth, st_now.wDay,
                    st_now.wHour, st_now.wMinute, st_now.wSecond,
                    st_sync.wYear, st_sync.wMonth, st_sync.wDay,
                    st_sync.wHour, st_sync.wMinute, st_sync.wSecond,
                    result.server,
                    result.offset_ms,
                    result.delay_ms,
                    result.status == SYNC_STATUS_SUCCESS ? "成功" :
                    result.status == SYNC_STATUS_WARNING ? "警告" :
                    result.status == SYNC_STATUS_FAILURE ? "失败" : "未知",
                    g_app.config.sync_interval_minutes,
                    g_app.has_privilege ? "已获得管理员权限" : "无管理员权限（无法修改时间）"
                );
            } else {
                snprintf(info, sizeof(info),
                    "当前系统时间: %04d-%02d-%02d %02d:%02d:%02d\r\n\r\n"
                    "尚未执行同步\r\n"
                    "点击下方按钮立即同步\r\n"
                    "\r\n"
                    "权限状态: %s",
                    st_now.wYear, st_now.wMonth, st_now.wDay,
                    st_now.wHour, st_now.wMinute, st_now.wSecond,
                    g_app.has_privilege ? "已获得管理员权限" : "无管理员权限（无法修改时间）"
                );
            }

            CreateWindowA("STATIC", info,
                         WS_VISIBLE | WS_CHILD | SS_LEFT,
                         30, 55, 420, 200, hwnd, NULL, NULL, NULL);

            CreateWindowA("BUTTON", "立即同步",
                         WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         180, 270, 120, 35, hwnd, (HMENU)IDC_BTN_SYNC_NOW, NULL, NULL);

            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BTN_SYNC_NOW) {
                sync_engine_do_sync();
                update_main_window_status(hwnd);
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            g_hwnd_main = NULL;
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    return 0;
}

static void update_main_window_status(HWND hwnd) {
    (void)hwnd;  /* 保留以备未来使用 */
    /* 简单方式：关闭并重新打开以刷新内容 */
    if (g_hwnd_main) {
        DestroyWindow(g_hwnd_main);
        g_hwnd_main = NULL;
    }
    show_main_window();
}

/* ============================================================================
 * 设置窗口过程
 * ============================================================================
 */

static LRESULT CALLBACK settings_window_proc(HWND hwnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            int y = 15;

            CreateWindowA("GROUPBOX", "同步设置",
                         WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                         15, y, 380, 100, hwnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "同步间隔（分钟）:",
                         WS_VISIBLE | WS_CHILD | SS_RIGHT,
                         25, y + 25, 120, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                         150, y + 22, 80, 24, hwnd, (HMENU)IDC_EDIT_INTERVAL, NULL, NULL);
            SetDlgItemInt(hwnd, IDC_EDIT_INTERVAL, g_app.config.sync_interval_minutes, FALSE);

            CreateWindowA("STATIC", "超时时间（秒）:",
                         WS_VISIBLE | WS_CHILD | SS_RIGHT,
                         25, y + 55, 120, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                         150, y + 52, 80, 24, hwnd, (HMENU)IDC_EDIT_TIMEOUT, NULL, NULL);
            SetDlgItemInt(hwnd, IDC_EDIT_TIMEOUT, g_app.config.timeout_seconds, FALSE);

            CreateWindowA("STATIC", "偏移警告阈值（ms）:",
                         WS_VISIBLE | WS_CHILD | SS_RIGHT,
                         25, y + 85, 120, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
                         150, y + 82, 80, 24, hwnd, (HMENU)IDC_EDIT_OFFSET, NULL, NULL);
            SetDlgItemInt(hwnd, IDC_EDIT_OFFSET, g_app.config.max_offset_warning_ms, FALSE);

            y += 120;

            CreateWindowA("GROUPBOX", "通知设置",
                         WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                         15, y, 380, 85, hwnd, NULL, NULL, NULL);

            CreateWindowA("BUTTON", "启动时自动同步",
                         WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                         25, y + 22, 160, 20, hwnd, (HMENU)IDC_CHK_STARTUP_SYNC, NULL, NULL);
            CheckDlgButton(hwnd, IDC_CHK_STARTUP_SYNC,
                           g_app.config.sync_on_startup ? BST_CHECKED : BST_UNCHECKED);

            CreateWindowA("BUTTON", "同步成功时通知",
                         WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                         25, y + 48, 160, 20, hwnd, (HMENU)IDC_CHK_NOTIFY_SUCCESS, NULL, NULL);
            CheckDlgButton(hwnd, IDC_CHK_NOTIFY_SUCCESS,
                           g_app.config.notify_on_success ? BST_CHECKED : BST_UNCHECKED);

            CreateWindowA("BUTTON", "同步失败时通知",
                         WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                         200, y + 22, 160, 20, hwnd, (HMENU)IDC_CHK_NOTIFY_FAIL, NULL, NULL);
            CheckDlgButton(hwnd, IDC_CHK_NOTIFY_FAIL,
                           g_app.config.notify_on_failure ? BST_CHECKED : BST_UNCHECKED);

            CreateWindowA("BUTTON", "偏移过大时通知",
                         WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                         200, y + 48, 160, 20, hwnd, (HMENU)IDC_CHK_NOTIFY_OFFSET, NULL, NULL);
            CheckDlgButton(hwnd, IDC_CHK_NOTIFY_OFFSET,
                           g_app.config.notify_on_large_offset ? BST_CHECKED : BST_UNCHECKED);

            y += 105;

            CreateWindowA("GROUPBOX", "开机启动",
                         WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
                         15, y, 380, 50, hwnd, NULL, NULL, NULL);

            CreateWindowA("BUTTON", "开机自动启动 WinTimeSync",
                         WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
                         25, y + 20, 200, 20, hwnd, (HMENU)IDC_CHK_AUTOSTART, NULL, NULL);

            bool autostart = autostart_is_enabled();
            CheckDlgButton(hwnd, IDC_CHK_AUTOSTART,
                           autostart ? BST_CHECKED : BST_UNCHECKED);

            y += 70;

            CreateWindowA("BUTTON", "保存设置",
                         WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         120, y, 100, 32, hwnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);

            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_BTN_SAVE) {
                BOOL trans;
                g_app.config.sync_interval_minutes = GetDlgItemInt(hwnd, IDC_EDIT_INTERVAL, &trans, FALSE);
                if (!trans || g_app.config.sync_interval_minutes < 1) {
                    MessageBoxA(hwnd, "同步间隔必须大于 0 分钟", "设置错误", MB_ICONERROR);
                    break;
                }

                g_app.config.timeout_seconds = GetDlgItemInt(hwnd, IDC_EDIT_TIMEOUT, &trans, FALSE);
                if (!trans || g_app.config.timeout_seconds < 1) {
                    MessageBoxA(hwnd, "超时时间必须大于 0 秒", "设置错误", MB_ICONERROR);
                    break;
                }

                g_app.config.max_offset_warning_ms = GetDlgItemInt(hwnd, IDC_EDIT_OFFSET, &trans, FALSE);

                g_app.config.sync_on_startup = IsDlgButtonChecked(hwnd, IDC_CHK_STARTUP_SYNC) == BST_CHECKED;
                g_app.config.notify_on_success = IsDlgButtonChecked(hwnd, IDC_CHK_NOTIFY_SUCCESS) == BST_CHECKED;
                g_app.config.notify_on_failure = IsDlgButtonChecked(hwnd, IDC_CHK_NOTIFY_FAIL) == BST_CHECKED;
                g_app.config.notify_on_large_offset = IsDlgButtonChecked(hwnd, IDC_CHK_NOTIFY_OFFSET) == BST_CHECKED;

                bool want_autostart = IsDlgButtonChecked(hwnd, IDC_CHK_AUTOSTART) == BST_CHECKED;
                if (want_autostart != autostart_is_enabled()) {
                    if (want_autostart) {
                        autostart_enable();
                    } else {
                        autostart_disable();
                    }
                }

                WtsError err = config_save(&g_app.config);
                if (err == WTS_SUCCESS) {
                    sync_engine_stop_timer();
                    sync_engine_start_timer(g_app.config.sync_interval_minutes * 60 * 1000);
                    MessageBoxA(hwnd, "设置已保存", "成功", MB_ICONINFORMATION);
                } else {
                    MessageBoxA(hwnd, "保存配置失败", "错误", MB_ICONERROR);
                }
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            g_hwnd_settings = NULL;
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    return 0;
}

/* ============================================================================
 * 关于窗口过程
 * ============================================================================
 */

static LRESULT CALLBACK about_window_proc(HWND hwnd, UINT msg,
                                           WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            CreateWindowA("STATIC", "WinTimeSync",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         0, 20, 320, 30, hwnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "版本 " APP_VERSION,
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         0, 55, 320, 20, hwnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "Windows NTP 时间同步工具",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         0, 80, 320, 20, hwnd, NULL, NULL, NULL);

            CreateWindowA("STATIC", "基于 NTPv4 (RFC 5905)",
                         WS_VISIBLE | WS_CHILD | SS_CENTER,
                         0, 105, 320, 20, hwnd, NULL, NULL, NULL);

            CreateWindowA("BUTTON", "确定",
                         WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                         120, 150, 80, 30, hwnd, (HMENU)IDOK, NULL, NULL);
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                DestroyWindow(hwnd);
                g_hwnd_about = NULL;
            }
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            g_hwnd_about = NULL;
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    return 0;
}

/* ============================================================================
 * 日志窗口过程
 * ============================================================================
 */

static LRESULT CALLBACK log_window_proc(HWND hwnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            /* 创建只读多行文本框显示日志 */
            HWND hEdit = CreateWindowA("EDIT", "",
                         WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                         10, 10, 560, 340, hwnd, (HMENU)IDC_TXT_LOG, NULL, NULL);

            /* 读取日志文件 */
            char log_path[MAX_PATH_LEN];
            get_log_path(log_path, sizeof(log_path));
            char pattern[MAX_PATH_LEN + 32];
            snprintf(pattern, sizeof(pattern), "%s\\sync*.log", log_path);

            WIN32_FIND_DATAA fdata;
            HANDLE hFind = FindFirstFileA(pattern, &fdata);
            char latest_file[MAX_PATH_LEN] = "";
            FILETIME latest_time = {0, 0};

            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    char fullpath[MAX_PATH_LEN + 32];
                    snprintf(fullpath, sizeof(fullpath), "%s\\%s", log_path, fdata.cFileName);
                    if (CompareFileTime(&fdata.ftLastWriteTime, &latest_time) > 0) {
                        latest_time = fdata.ftLastWriteTime;
                        strncpy_s(latest_file, sizeof(latest_file), fullpath, _TRUNCATE);
                    }
                } while (FindNextFileA(hFind, &fdata));
                FindClose(hFind);
            }

            if (strlen(latest_file) > 0) {
                FILE* fp = NULL;
                fopen_s(&fp, latest_file, "r");
                if (fp) {
                    char buffer[4096];
                    size_t n = fread(buffer, 1, sizeof(buffer) - 1, fp);
                    buffer[n] = '\0';
                    SetWindowTextA(hEdit, buffer);
                    fclose(fp);
                } else {
                    SetWindowTextA(hEdit, "无法打开日志文件");
                }
            } else {
                SetWindowTextA(hEdit, "暂无日志记录");
            }

            break;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            g_hwnd_log = NULL;
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    return 0;
}

/* ============================================================================
 * 窗口创建辅助函数
 * ============================================================================
 */

static HWND create_window_class(const char* class_name, WNDPROC proc) {
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = class_name;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            LOG_ERROR("注册窗口类失败 %s: %d", class_name, err);
            return NULL;
        }
    }
    return (HWND)1;
}

static void show_main_window(void) {
    if (g_hwnd_main) {
        SetForegroundWindow(g_hwnd_main);
        return;
    }

    create_window_class("WinTimeSyncMainClass", main_window_proc);

    g_hwnd_main = CreateWindowExA(0,
        "WinTimeSyncMainClass",
        "WinTimeSync - 状态",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        MAIN_WINDOW_WIDTH, MAIN_WINDOW_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (g_hwnd_main) {
        center_window(g_hwnd_main);
        ShowWindow(g_hwnd_main, SW_SHOW);
        UpdateWindow(g_hwnd_main);
    }
}

static void show_settings_window(void) {
    if (g_hwnd_settings) {
        SetForegroundWindow(g_hwnd_settings);
        return;
    }

    create_window_class("WinTimeSyncSettingsClass", settings_window_proc);

    g_hwnd_settings = CreateWindowExA(0,
        "WinTimeSyncSettingsClass",
        "设置",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        SETTINGS_WIDTH, SETTINGS_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (g_hwnd_settings) {
        center_window(g_hwnd_settings);
        ShowWindow(g_hwnd_settings, SW_SHOW);
        UpdateWindow(g_hwnd_settings);
    }
}

static void show_about_window(void) {
    if (g_hwnd_about) {
        SetForegroundWindow(g_hwnd_about);
        return;
    }

    create_window_class("WinTimeSyncAboutClass", about_window_proc);

    g_hwnd_about = CreateWindowExA(0,
        "WinTimeSyncAboutClass",
        "关于 WinTimeSync",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        ABOUT_WIDTH, ABOUT_HEIGHT,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (g_hwnd_about) {
        center_window(g_hwnd_about);
        ShowWindow(g_hwnd_about, SW_SHOW);
        UpdateWindow(g_hwnd_about);
    }
}

static void show_log_window(void) {
    if (g_hwnd_log) {
        SetForegroundWindow(g_hwnd_log);
        return;
    }

    create_window_class("WinTimeSyncLogClass", log_window_proc);

    g_hwnd_log = CreateWindowExA(0,
        "WinTimeSyncLogClass",
        "同步日志",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        590, 400,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    if (g_hwnd_log) {
        center_window(g_hwnd_log);
        ShowWindow(g_hwnd_log, SW_SHOW);
        UpdateWindow(g_hwnd_log);
    }
}

/* ============================================================================
 * 公共函数
 * ============================================================================
 */

HWND tray_icon_create_window(void) {
    WNDCLASSEXA wc;
    memset(&wc, 0, sizeof(WNDCLASSEXA));
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = tray_window_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "WinTimeSyncTrayClass";

    if (!RegisterClassExA(&wc)) {
        DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            LOG_ERROR("注册窗口类失败: %d", err);
            return NULL;
        }
    }

    HWND hwnd = CreateWindowExA(0,
                                "WinTimeSyncTrayClass",
                                "WinTimeSync Tray",
                                WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                200, 200,
                                NULL, NULL,
                                GetModuleHandle(NULL),
                                NULL);

    if (!hwnd) {
        LOG_ERROR("创建托盘窗口失败: %d", GetLastError());
        return NULL;
    }

    ShowWindow(hwnd, SW_HIDE);
    g_tray_hwnd = hwnd;
    g_app.hwnd_tray = hwnd;

    LOG_INFO("托盘窗口创建成功");
    return hwnd;
}

bool tray_icon_init(HWND hwnd) {
    if (!hwnd) return false;

    memset(&g_nid, 0, sizeof(NOTIFYICONDATAA));
    g_nid.cbSize = sizeof(NOTIFYICONDATAA);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY_ICON;

    g_nid.hIcon = LoadIconA(GetModuleHandle(NULL),
                            MAKEINTRESOURCEA(IDI_ICON_NORMAL));
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    }
    strncpy_s(g_nid.szTip, sizeof(g_nid.szTip), APP_NAME, _TRUNCATE);

    if (!Shell_NotifyIconA(NIM_ADD, &g_nid)) {
        LOG_ERROR("添加托盘图标失败: %d", GetLastError());
        return false;
    }

    g_tray_menu = create_tray_menu();
    if (!g_tray_menu) {
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
        return false;
    }

    LOG_INFO("托盘图标初始化成功");
    return true;
}

void tray_icon_remove(void) {
    if (g_nid.hWnd) {
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
        g_nid.hWnd = NULL;
    }
    if (g_tray_menu) { DestroyMenu(g_tray_menu); g_tray_menu = NULL; }
    if (g_tray_hwnd) { DestroyWindow(g_tray_hwnd); g_tray_hwnd = NULL; }
    if (g_hwnd_main) { DestroyWindow(g_hwnd_main); g_hwnd_main = NULL; }
    if (g_hwnd_settings) { DestroyWindow(g_hwnd_settings); g_hwnd_settings = NULL; }
    if (g_hwnd_about) { DestroyWindow(g_hwnd_about); g_hwnd_about = NULL; }
    if (g_hwnd_log) { DestroyWindow(g_hwnd_log); g_hwnd_log = NULL; }
}

void tray_icon_update_status(TrayIconStatus status) {
    if (!g_nid.hWnd) return;
    g_current_status = status;
    g_nid.hIcon = LoadIconA(GetModuleHandle(NULL),
                            MAKEINTRESOURCEA(get_icon_resource_id(status)));
    if (!g_nid.hIcon) {
        g_nid.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    }
    g_nid.uFlags = NIF_ICON;
    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

void tray_icon_show_menu(HWND hwnd, int x, int y) {
    if (!g_tray_menu) {
        g_tray_menu = create_tray_menu();
        if (!g_tray_menu) return;
    }
    SetForegroundWindow(hwnd);
    TrackPopupMenu(g_tray_menu,
                   TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                   x, y, 0, hwnd, NULL);
    DestroyMenu(g_tray_menu);
    g_tray_menu = NULL;
}

void tray_icon_show_balloon(const char* title, const char* message, DWORD flags) {
    if (!g_nid.hWnd) return;
    g_nid.uFlags = NIF_INFO;
    g_nid.uTimeout = 5000;
    strncpy_s(g_nid.szInfoTitle, sizeof(g_nid.szInfoTitle), title, _TRUNCATE);
    strncpy_s(g_nid.szInfo, sizeof(g_nid.szInfo), message, _TRUNCATE);
    g_nid.dwInfoFlags = flags;
    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
}

void tray_icon_update_tip(const char* text) {
    if (!g_nid.hWnd || !text) return;
    strncpy_s(g_nid.szTip, sizeof(g_nid.szTip), text, _TRUNCATE);
    g_nid.uFlags = NIF_TIP;
    Shell_NotifyIconA(NIM_MODIFY, &g_nid);
}

void tray_icon_update_from_sync(SyncStatus sync_status, const SyncResult* result) {
    TrayIconStatus icon_status;
    switch (sync_status) {
        case SYNC_STATUS_SUCCESS:   icon_status = TRAY_ICON_NORMAL; break;
        case SYNC_STATUS_WARNING:   icon_status = TRAY_ICON_WARNING; break;
        case SYNC_STATUS_FAILURE:   icon_status = TRAY_ICON_ERROR; break;
        case SYNC_STATUS_IN_PROGRESS: icon_status = TRAY_ICON_SYNCING; break;
        default: icon_status = TRAY_ICON_NORMAL;
    }
    tray_icon_update_status(icon_status);

    char tip[TRAY_TIP_MAX_LEN];
    if (result && result->status != SYNC_STATUS_IDLE) {
        snprintf(tip, sizeof(tip), "%s\n偏移: %.1f ms", APP_NAME, result->offset_ms);
    } else {
        strncpy_s(tip, sizeof(tip), APP_NAME, _TRUNCATE);
    }
    tray_icon_update_tip(tip);

    if (result) {
        AppConfig* config = &g_app.config;
        if (sync_status == SYNC_STATUS_SUCCESS && config->notify_on_success) {
            char msg[TRAY_BALLOON_MSG_MAX];
            snprintf(msg, sizeof(msg), "时间已校准\n偏移: %.1f ms", result->offset_ms);
            tray_icon_show_balloon("同步成功", msg, NIIF_INFO);
        } else if (sync_status == SYNC_STATUS_FAILURE && config->notify_on_failure) {
            tray_icon_show_balloon("同步失败", result->error_msg, NIIF_ERROR);
        } else if (sync_status == SYNC_STATUS_WARNING && config->notify_on_large_offset) {
            char msg[TRAY_BALLOON_MSG_MAX];
            snprintf(msg, sizeof(msg), "时间偏差过大 (%.1f ms)\n已校准", result->offset_ms);
            tray_icon_show_balloon("同步警告", msg, NIIF_WARNING);
        }
    }
}
