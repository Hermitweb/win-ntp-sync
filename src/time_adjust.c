/**
 * @file time_adjust.c
 * @brief 时间调整模块实现
 * @details 提供 SetSystemTime API 封装、权限检查功能
 */

#include "time_adjust.h"
#include "log.h"
#include "ntp_client.h"
#include <math.h>

/* ============================================================================
 * 公共函数
 * ============================================================================ */

bool time_check_privilege(void) {
    BOOL has_privilege = FALSE;
    HANDLE token = NULL;

    /* 打开当前进程的令牌 */
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                          &token)) {
        LOG_WARNING("打开进程令牌失败: %d", GetLastError());
        return false;
    }

    /* 查询权限 - 使用 Windows SDK 提供的 SE_SYSTEMTIME_NAME 宏（宽字符） */
    LUID luid;
    if (!LookupPrivilegeValue(NULL, SE_SYSTEMTIME_NAME, &luid)) {
        LOG_WARNING("查找权限值失败: %d", GetLastError());
        CloseHandle(token);
        return false;
    }

    PRIVILEGE_SET ps;
    ps.PrivilegeCount = 1;
    ps.Control = PRIVILEGE_SET_ALL_NECESSARY;
    ps.Privilege[0].Luid = luid;
    ps.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!PrivilegeCheck(token, &ps, &has_privilege)) {
        LOG_WARNING("检查权限失败: %d", GetLastError());
        CloseHandle(token);
        return false;
    }

    CloseHandle(token);
    return has_privilege != FALSE;
}

bool time_enable_privilege(void) {
    HANDLE token = NULL;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    /* 打开当前进程的令牌 */
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
                          &token)) {
        LOG_ERROR("打开进程令牌失败: %d", GetLastError());
        return false;
    }

    /* 查找权限 LUID - 使用 Windows SDK 提供的宏 */
    if (!LookupPrivilegeValue(NULL, SE_SYSTEMTIME_NAME, &luid)) {
        LOG_ERROR("查找权限值失败: %d", GetLastError());
        CloseHandle(token);
        return false;
    }

    /* 设置权限 */
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL)) {
        LOG_ERROR("调整权限失败: %d", GetLastError());
        CloseHandle(token);
        return false;
    }

    /* 检查是否成功 */
    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        LOG_ERROR("权限未被分配");
        CloseHandle(token);
        return false;
    }

    CloseHandle(token);
    LOG_INFO("成功启用系统时间调整权限");
    return true;
}

WtsError time_adjust(const FILETIME* filetime) {
    if (!filetime) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 检查权限 */
    if (!time_check_privilege()) {
        LOG_ERROR("缺少系统时间调整权限");
        return WTS_ERR_TIME_NO_PRIVILEGE;
    }

    /* 转换为 SYSTEMTIME */
    SYSTEMTIME systemtime;
    filetime_to_systemtime(filetime, &systemtime);

    /* 调整系统时间 */
    if (!SetSystemTime(&systemtime)) {
        LOG_ERROR("设置系统时间失败: %d", GetLastError());
        return WTS_ERR_TIME_ADJUST_FAILED;
    }

    LOG_INFO("系统时间已调整: %04u-%02u-%02u %02u:%02u:%02u.%03u",
             (unsigned)systemtime.wYear, (unsigned)systemtime.wMonth,
             (unsigned)systemtime.wDay, (unsigned)systemtime.wHour,
             (unsigned)systemtime.wMinute, (unsigned)systemtime.wSecond,
             (unsigned)systemtime.wMilliseconds);

    return WTS_SUCCESS;
}

WtsError time_adjust_from_sync(const SyncResult* sync_result) {
    if (!sync_result) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 如果同步失败，不调整时间 */
    if (sync_result->status == SYNC_STATUS_FAILURE) {
        LOG_WARNING("同步失败，不调整系统时间");
        return WTS_SUCCESS;
    }

    /* 获取当前系统时间 */
    FILETIME current_time;
    time_get_current(&current_time);

    /* 计算需要调整的偏移量（毫秒） */
    double offset_ms = sync_result->offset_ms;

    /* 转换为 100ns 单位 */
    int64_t offset_100ns = (int64_t)(offset_ms * 10000.0);

    /* 计算新的时间 */
    ULARGE_INTEGER ul;
    ul.LowPart = current_time.dwLowDateTime;
    ul.HighPart = current_time.dwHighDateTime;
    ul.QuadPart += offset_100ns;

    FILETIME new_time;
    new_time.dwLowDateTime = ul.LowPart;
    new_time.dwHighDateTime = ul.HighPart;

    /* 调整时间 */
    WtsError err = time_adjust(&new_time);
    if (err != WTS_SUCCESS) {
        return err;
    }

    /* 检查偏移量是否过大 */
    double abs_offset_ms = fabs(offset_ms);
    if (abs_offset_ms > g_app.config.max_offset_warning_ms) {
        LOG_WARNING("时间偏差过大: %.2f ms (> %d ms)",
                    offset_ms, g_app.config.max_offset_warning_ms);
        return WTS_SUCCESS;  /* 仍然成功，但标记为警告 */
    }

    return WTS_SUCCESS;
}

void time_get_current(FILETIME* filetime) {
    GetSystemTimeAsFileTime(filetime);
}

void filetime_to_systemtime(const FILETIME* filetime, SYSTEMTIME* systemtime) {
    FileTimeToSystemTime(filetime, systemtime);
}

void systemtime_to_filetime(const SYSTEMTIME* systemtime, FILETIME* filetime) {
    SystemTimeToFileTime(systemtime, filetime);
}
