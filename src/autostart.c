/**
 * @file autostart.c
 * @brief 自启动模块实现
 * @details 提供注册表 HKCU 自启动项管理功能
 */

#include "autostart.h"
#include "log.h"

/* ============================================================================
 * 内部函数
 * ============================================================================ */

/**
 * @brief 获取当前可执行文件路径
 * @param path 输出路径缓冲区
 * @param size 缓冲区大小
 * @return 成功返回 true，失败返回 false
 */
static bool get_executable_path(char* path, size_t size) {
    if (!GetModuleFileNameA(NULL, path, size)) {
        LOG_ERROR("获取可执行文件路径失败: %d", GetLastError());
        return false;
    }
    return true;
}

/* ============================================================================
 * 公共函数
 * ============================================================================ */

bool autostart_is_enabled(void) {
    HKEY hkey;
    LONG ret;

    /* 打开注册表键 */
    ret = RegOpenKeyExA(HKEY_CURRENT_USER, AUTOSTART_REG_PATH,
                        0, KEY_QUERY_VALUE, &hkey);
    if (ret != ERROR_SUCCESS) {
        LOG_DEBUG("打开注册表键失败: %d", ret);
        return false;
    }

    /* 检查是否存在 WinTimeSync 键 */
    char value[MAX_PATH_LEN];
    DWORD value_size = sizeof(value);
    ret = RegQueryValueExA(hkey, AUTOSTART_REG_KEY, NULL, NULL,
                           (LPBYTE)value, &value_size);

    RegCloseKey(hkey);

    return ret == ERROR_SUCCESS;
}

WtsError autostart_enable(void) {
    HKEY hkey;
    LONG ret;
    char exe_path[MAX_PATH_LEN];

    /* 获取可执行文件路径 */
    if (!get_executable_path(exe_path, sizeof(exe_path))) {
        return WTS_ERR_AUTOSTART;
    }

    /* 打开或创建注册表键 */
    ret = RegOpenKeyExA(HKEY_CURRENT_USER, AUTOSTART_REG_PATH,
                        0, KEY_SET_VALUE, &hkey);
    if (ret != ERROR_SUCCESS) {
        LOG_ERROR("打开注册表键失败: %d", ret);
        return WTS_ERR_REGISTRY;
    }

    /* 设置自启动键 */
    ret = RegSetValueExA(hkey, AUTOSTART_REG_KEY, 0, REG_SZ,
                         (const BYTE*)exe_path, strlen(exe_path) + 1);
    if (ret != ERROR_SUCCESS) {
        LOG_ERROR("设置自启动键失败: %d", ret);
        RegCloseKey(hkey);
        return WTS_ERR_REGISTRY;
    }

    RegCloseKey(hkey);
    LOG_INFO("开机自启已启用: %s", exe_path);
    return WTS_SUCCESS;
}

WtsError autostart_disable(void) {
    HKEY hkey;
    LONG ret;

    /* 打开注册表键 */
    ret = RegOpenKeyExA(HKEY_CURRENT_USER, AUTOSTART_REG_PATH,
                        0, KEY_SET_VALUE, &hkey);
    if (ret != ERROR_SUCCESS) {
        LOG_ERROR("打开注册表键失败: %d", ret);
        return WTS_ERR_REGISTRY;
    }

    /* 删除自启动键 */
    ret = RegDeleteValueA(hkey, AUTOSTART_REG_KEY);
    if (ret != ERROR_SUCCESS) {
        if (ret != ERROR_FILE_NOT_FOUND) {
            LOG_ERROR("删除自启动键失败: %d", ret);
            RegCloseKey(hkey);
            return WTS_ERR_REGISTRY;
        }
        /* 键不存在，视为成功 */
    }

    RegCloseKey(hkey);
    LOG_INFO("开机自启已禁用");
    return WTS_SUCCESS;
}

WtsError autostart_set(bool enable) {
    if (enable) {
        return autostart_enable();
    } else {
        return autostart_disable();
    }
}