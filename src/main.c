/**
 * @file main.c
 * @brief WinTimeSync 主入口
 * @details WinMain 入口、消息泵、全局初始化、运行时提权
 */

#include "common.h"
#include "log.h"
#include "config.h"
#include "sync_engine.h"
#include "tray_icon.h"
#include "autostart.h"
#include "time_adjust.h"
#include "resource.h"
#include <shellapi.h>

/* ============================================================================
 * 常量定义
 * ============================================================================ */

/* 消息循环超时时间（毫秒） */
#define MSG_LOOP_TIMEOUT  100

/* ============================================================================
 * 内部函数
 * ============================================================================ */

/**
 * @brief 检查当前进程是否有管理员权限
 * @return 有管理员权限返回 true
 */
static bool is_admin(void) {
    BOOL is_elevated = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(elevation);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
            is_elevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }

    return is_elevated == TRUE;
}

/**
 * @brief 以管理员身份重新启动程序
 * @return 返回新进程的退出码，如果用户取消则返回 -1
 */
static int restart_as_admin(void) {
    char path[MAX_PATH_LEN];
    GetModuleFileNameA(NULL, path, sizeof(path));

    SHELLEXECUTEINFOA sei;
    memset(&sei, 0, sizeof(sei));
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = path;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExA(&sei)) {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED) {
            return -1;
        }
        return -2;
    }

    WaitForSingleObject(sei.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(sei.hProcess, &exit_code);
    CloseHandle(sei.hProcess);

    return (int)exit_code;
}

/**
 * @brief 应用初始化
 * @return 成功返回 true，失败返回 false
 */
static bool app_initialize(void) {
    /* 初始化同步引擎 */
    if (!sync_engine_init()) {
        MessageBoxA(NULL, "同步引擎初始化失败", APP_NAME, MB_ICONERROR);
        return false;
    }

    /* 创建托盘窗口 */
    HWND tray_hwnd = tray_icon_create_window();
    if (!tray_hwnd) {
        MessageBoxA(NULL, "创建托盘窗口失败", APP_NAME, MB_ICONERROR);
        return false;
    }

    /* 初始化托盘图标 */
    if (!tray_icon_init(tray_hwnd)) {
        MessageBoxA(NULL, "初始化托盘图标失败", APP_NAME, MB_ICONERROR);
        return false;
    }

    /* 检查权限 */
    if (!g_app.has_privilege) {
        tray_icon_show_balloon("权限警告",
                               "需要管理员权限以修改系统时间",
                               NIIF_WARNING);
    }

    /* 如果配置了启动时同步，执行一次同步 */
    if (g_app.config.sync_on_startup) {
        LOG_INFO("启动时同步");
        sync_engine_do_sync();
    }

    /* 启动定时同步 */
    int interval_ms = MIN_TO_MS(g_app.config.sync_interval_minutes);
    sync_engine_start_timer(interval_ms);

    LOG_INFO("%s v%s 启动成功", APP_NAME, APP_VERSION);
    return true;
}

/**
 * @brief 应用清理
 */
static void app_cleanup(void) {
    LOG_INFO("应用清理开始");

    /* 移除托盘图标 */
    tray_icon_remove();

    /* 清理同步引擎 */
    sync_engine_cleanup();

    LOG_INFO("应用清理完成");
}

/**
 * @brief 消息循环
 * @return 退出码
 */
static int message_loop(void) {
    MSG msg;
    BOOL ret;

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
        if (ret == -1) {
            /* 错误 */
            LOG_ERROR("消息循环错误");
            break;
        }

        /* 处理消息 */
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

/* ============================================================================
 * WinMain 入口
 * ============================================================================ */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    /* 检查是否已有管理员权限 */
    if (!is_admin()) {
        /* 没有管理员权限，请求提权 */
        int result = restart_as_admin();
        if (result >= 0) {
            /* 用户同意提权，返回新进程的退出码 */
            return result;
        } else if (result == -1) {
            /* 用户取消提权，继续以普通用户身份运行 */
            LOG_WARNING("用户拒绝管理员权限请求，将以普通用户身份运行");
        } else {
            /* 提权失败 */
            LOG_ERROR("提权失败，错误码: %d", GetLastError());
        }
    }

    /* 初始化应用 */
    if (!app_initialize()) {
        app_cleanup();
        return 1;
    }

    /* 进入消息循环 */
    int exit_code = message_loop();

    /* 清理应用 */
    app_cleanup();

    return exit_code;
}