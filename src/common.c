/**
 * @file common.c
 * @brief 公共工具函数实现
 * @details 提供路径获取、错误消息转换等工具函数
 */

#include "common.h"
#include "log.h"
#include <stdio.h>

/* ============================================================================
 * 公共函数
 * ============================================================================ */

bool get_appdata_path(char* path, size_t size) {
    if (!path || size < 16) {
        return false;
    }

    /* 获取 APPDATA 环境变量 */
    char appdata[MAX_PATH_LEN];
    if (!GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH_LEN)) {
        LOG_ERROR("获取 APPDATA 环境变量失败: %d", GetLastError());
        return false;
    }

    /* 构建应用数据目录路径 */
    _snprintf(path, size, "%s\\%s", appdata, APP_NAME);

    return true;
}

bool get_config_path(char* path, size_t size) {
    if (!path || size < 16) {
        return false;
    }

    /* 获取应用数据目录 */
    char appdata_path[MAX_PATH_LEN];
    if (!get_appdata_path(appdata_path, MAX_PATH_LEN)) {
        return false;
    }

    /* 构建配置文件路径 */
    _snprintf(path, size, "%s\\%s", appdata_path, CONFIG_FILE_NAME);

    return true;
}

bool get_log_path(char* path, size_t size) {
    if (!path || size < 16) {
        return false;
    }

    /* 获取应用数据目录 */
    char appdata_path[MAX_PATH_LEN];
    if (!get_appdata_path(appdata_path, MAX_PATH_LEN)) {
        return false;
    }

    /* 构建日志目录路径 */
    _snprintf(path, size, "%s\\%s", appdata_path, LOG_DIR_NAME);

    return true;
}

const char* get_error_message(WtsError error) {
    switch (error) {
        case WTS_SUCCESS:
            return "成功";

        /* 配置相关错误 */
        case WTS_ERR_CONFIG_NOT_FOUND:
            return "配置文件未找到";
        case WTS_ERR_CONFIG_PARSE:
            return "配置文件解析失败";
        case WTS_ERR_CONFIG_WRITE:
            return "配置文件写入失败";
        case WTS_ERR_CONFIG_INVALID:
            return "配置无效";

        /* 网络相关错误 */
        case WTS_ERR_SOCKET_INIT:
            return "Socket 初始化失败";
        case WTS_ERR_SOCKET_CREATE:
            return "Socket 创建失败";
        case WTS_ERR_SOCKET_CONNECT:
            return "Socket 连接失败";
        case WTS_ERR_SOCKET_SEND:
            return "数据发送失败";
        case WTS_ERR_SOCKET_RECV:
            return "数据接收失败";
        case WTS_ERR_SOCKET_TIMEOUT:
            return "网络超时";
        case WTS_ERR_DNS_RESOLVE:
            return "DNS 解析失败";

        /* NTP 协议相关错误 */
        case WTS_ERR_NTP_INVALID_PACKET:
            return "NTP 报文无效";
        case WTS_ERR_NTP_INVALID_VERSION:
            return "NTP 版本不匹配";
        case WTS_ERR_NTP_INVALID_MODE:
            return "NTP 模式不正确";
        case WTS_ERR_NTP_KISS_OF_DEATH:
            return "收到 Kiss-o'-Death 响应";
        case WTS_ERR_NTP_INVALID_STRATUM:
            return "NTP Stratum 无效";

        /* 时间调整相关错误 */
        case WTS_ERR_TIME_NO_PRIVILEGE:
            return "缺少系统时间调整权限";
        case WTS_ERR_TIME_ADJUST_FAILED:
            return "系统时间调整失败";

        /* 日志相关错误 */
        case WTS_ERR_LOG_OPEN:
            return "日志文件打开失败";
        case WTS_ERR_LOG_WRITE:
            return "日志写入失败";
        case WTS_ERR_LOG_ROTATE:
            return "日志轮转失败";

        /* 系统相关错误 */
        case WTS_ERR_REGISTRY:
            return "注册表操作失败";
        case WTS_ERR_AUTOSTART:
            return "自启动设置失败";

        /* 通用错误 */
        case WTS_ERR_UNKNOWN:
            return "未知错误";
        case WTS_ERR_NULL_PARAM:
            return "空参数";
        case WTS_ERR_BUFFER_TOO_SMALL:
            return "缓冲区太小";
        case WTS_ERR_NOT_INITIALIZED:
            return "未初始化";

        default:
            return "未知错误";
    }
}