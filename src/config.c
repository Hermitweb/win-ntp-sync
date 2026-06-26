/**
 * @file config.c
 * @brief 配置模块实现
 * @details 提供 INI 配置文件读写、默认值、服务器列表管理功能
 */

#include "config.h"
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <direct.h>

/* ============================================================================
 * INI 配置键名定义
 * ============================================================================ */

#define SECTION_APP       "[app]"
#define SECTION_SYNC      "[sync]"
#define SECTION_SERVERS   "[servers]"
#define SECTION_LOG       "[log]"
#define SECTION_NOTIFY    "[notify]"
#define SECTION_AUTOSTART "[autostart]"

/* ============================================================================
 * 默认服务器列表
 * ============================================================================ */

static const char* DEFAULT_SERVERS[] = {
    "pool.ntp.org",
    "time.windows.com",
    "cn.pool.ntp.org",
    "time.pool.aliyun.com",
    "time.tencentcloud.com"
};

#define DEFAULT_SERVER_COUNT 3

/* ============================================================================
 * 内部函数
 * ============================================================================ */

/**
 * @brief 从 INI 文件读取整数值
 * @param filepath 文件路径
 * @param section 段名
 * @param key 键名
 * @param default_value 默认值
 * @return 读取到的值，失败返回默认值
 */
static int read_int_value(const char* filepath, const char* section,
                          const char* key, int default_value) {
    char buffer[256];
    GetPrivateProfileStringA(section, key, "", buffer, sizeof(buffer), filepath);
    if (buffer[0] == '\0') {
        return default_value;
    }
    return atoi(buffer);
}

/**
 * @brief 从 INI 文件读取布尔值
 * @param filepath 文件路径
 * @param section 段名
 * @param key 键名
 * @param default_value 默认值
 * @return 读取到的值，失败返回默认值
 */
static bool read_bool_value(const char* filepath, const char* section,
                            const char* key, bool default_value) {
    char buffer[256];
    GetPrivateProfileStringA(section, key, "", buffer, sizeof(buffer), filepath);
    if (buffer[0] == '\0') {
        return default_value;
    }
    return (strcmp(buffer, "1") == 0 || strcmp(buffer, "true") == 0);
}

/**
 * @brief 从 INI 文件读取字符串值
 * @param filepath 文件路径
 * @param section 段名
 * @param key 键名
 * @param buffer 输出缓冲区
 * @param size 缓冲区大小
 * @param default_value 默认值
 */
static void read_string_value(const char* filepath, const char* section,
                              const char* key, char* buffer, size_t size,
                              const char* default_value) {
    GetPrivateProfileStringA(section, key, default_value, buffer, size, filepath);
}

/**
 * @brief 向 INI 文件写入整数值
 * @param filepath 文件路径
 * @param section 段名
 * @param key 键名
 * @param value 值
 */
static void write_int_value(const char* filepath, const char* section,
                            const char* key, int value) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", value);
    WritePrivateProfileStringA(section, key, buffer, filepath);
}

/**
 * @brief 向 INI 文件写入布尔值
 * @param filepath 文件路径
 * @param section 段名
 * @param key 键名
 * @param value 值
 */
static void write_bool_value(const char* filepath, const char* section,
                             const char* key, bool value) {
    WritePrivateProfileStringA(section, key, value ? "1" : "0", filepath);
}

/**
 * @brief 向 INI 文件写入字符串值
 * @param filepath 文件路径
 * @param section 段名
 * @param key 键名
 * @param value 值
 */
static void write_string_value(const char* filepath, const char* section,
                               const char* key, const char* value) {
    WritePrivateProfileStringA(section, key, value, filepath);
}

/**
 * @brief 确保配置目录存在
 * @param filepath 配置文件路径
 * @return 成功返回 true，失败返回 false
 */
static bool ensure_config_directory(const char* filepath) {
    char dir_path[MAX_PATH_LEN];
    strncpy_s(dir_path, sizeof(dir_path), filepath, _TRUNCATE);

    /* 查找最后一个反斜杠 */
    char* last_slash = strrchr(dir_path, '\\');
    if (last_slash) {
        *last_slash = '\0';
        return _mkdir(dir_path) == 0 || errno == EEXIST;
    }
    return true;
}

/**
 * @brief 检查服务器地址是否为有效的域名或 IP 地址
 * @param server 服务器地址
 * @return 有效返回 true，无效返回 false
 */
static bool is_valid_server_address(const char* server) {
    if (!server || strlen(server) == 0) {
        return false;
    }

    /* 检查长度 */
    if (strlen(server) > MAX_SERVER_LEN - 1) {
        return false;
    }

    /* 检查是否包含非法字符 */
    for (const char* p = server; *p; p++) {
        if (!(*p >= 'a' && *p <= 'z') &&
            !(*p >= 'A' && *p <= 'Z') &&
            !(*p >= '0' && *p <= '9') &&
            *p != '.' && *p != '-' && *p != ':' &&
            *p != '[' && *p != ']') {
            return false;
        }
    }

    return true;
}

/* ============================================================================
 * 公共函数
 * ============================================================================ */

WtsError config_load(AppConfig* config) {
    if (!config) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 加载默认值 */
    config_load_defaults(config);

    /* 获取配置文件路径 */
    char filepath[MAX_PATH_LEN];
    if (!get_config_path(filepath, sizeof(filepath))) {
        LOG_WARNING("无法获取配置文件路径，使用默认配置");
        return WTS_SUCCESS;  /* 使用默认配置 */
    }

    /* 检查配置文件是否存在 */
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        LOG_INFO("配置文件不存在，创建默认配置: %s", filepath);

        /* 创建默认配置文件 */
        ensure_config_directory(filepath);
        WtsError err = config_save(config);
        if (err != WTS_SUCCESS) {
            LOG_ERROR("创建默认配置文件失败");
        }
        return WTS_SUCCESS;  /* 使用默认配置 */
    }
    fclose(fp);

    /* 读取配置 */
    LOG_INFO("加载配置文件: %s", filepath);

    /* [sync] 段 */
    config->sync_interval_minutes = read_int_value(filepath, "sync",
        "interval_minutes", DEFAULT_SYNC_INTERVAL_MINUTES);
    config->timeout_seconds = read_int_value(filepath, "sync",
        "timeout_seconds", DEFAULT_TIMEOUT_SECONDS);
    config->max_offset_warning_ms = read_int_value(filepath, "sync",
        "max_offset_warning_ms", DEFAULT_MAX_OFFSET_WARNING_MS);
    config->sync_on_startup = read_bool_value(filepath, "sync",
        "sync_on_startup", true);

    /* [servers] 段 */
    int server_count = read_int_value(filepath, "servers", "count", DEFAULT_SERVER_COUNT);
    server_count = WTS_MIN(server_count, MAX_SERVERS);
    config->server_count = server_count;

    for (int i = 0; i < server_count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "server_%d", i + 1);
        read_string_value(filepath, "servers", key,
            config->servers[i].address, MAX_SERVER_LEN,
            DEFAULT_SERVERS[i % DEFAULT_SERVER_COUNT]);
        config->servers[i].status = SERVER_STATUS_OK;
    }

    /* [log] 段 */
    config->max_log_file_size_mb = read_int_value(filepath, "log",
        "max_file_size_mb", MAX_LOG_FILE_SIZE_MB);
    config->max_log_files = read_int_value(filepath, "log",
        "max_files", MAX_LOG_FILES);
    read_string_value(filepath, "log", "path",
        config->log_path, MAX_PATH_LEN, LOG_DIR_NAME);

    /* [notify] 段 */
    config->notify_on_success = read_bool_value(filepath, "notify",
        "on_success", false);
    config->notify_on_failure = read_bool_value(filepath, "notify",
        "on_failure", true);
    config->notify_on_large_offset = read_bool_value(filepath, "notify",
        "on_large_offset", true);

    /* [autostart] 段 */
    config->autostart_enabled = read_bool_value(filepath, "autostart",
        "enabled", false);

    return WTS_SUCCESS;
}

WtsError config_save(const AppConfig* config) {
    if (!config) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 获取配置文件路径 */
    char filepath[MAX_PATH_LEN];
    if (!get_config_path(filepath, sizeof(filepath))) {
        return WTS_ERR_CONFIG_WRITE;
    }

    /* 确保目录存在 */
    if (!ensure_config_directory(filepath)) {
        return WTS_ERR_CONFIG_WRITE;
    }

    LOG_INFO("保存配置文件: %s", filepath);

    /* [app] 段 */
    write_string_value(filepath, "app", "version", APP_VERSION);

    /* [sync] 段 */
    write_int_value(filepath, "sync", "interval_minutes", config->sync_interval_minutes);
    write_int_value(filepath, "sync", "timeout_seconds", config->timeout_seconds);
    write_int_value(filepath, "sync", "max_offset_warning_ms", config->max_offset_warning_ms);
    write_bool_value(filepath, "sync", "sync_on_startup", config->sync_on_startup);

    /* [servers] 段 */
    write_int_value(filepath, "servers", "count", config->server_count);
    for (int i = 0; i < config->server_count; i++) {
        char key[32];
        snprintf(key, sizeof(key), "server_%d", i + 1);
        write_string_value(filepath, "servers", key, config->servers[i].address);
    }

    /* [log] 段 */
    write_int_value(filepath, "log", "max_file_size_mb", config->max_log_file_size_mb);
    write_int_value(filepath, "log", "max_files", config->max_log_files);
    write_string_value(filepath, "log", "path", config->log_path);

    /* [notify] 段 */
    write_bool_value(filepath, "notify", "on_success", config->notify_on_success);
    write_bool_value(filepath, "notify", "on_failure", config->notify_on_failure);
    write_bool_value(filepath, "notify", "on_large_offset", config->notify_on_large_offset);

    /* [autostart] 段 */
    write_bool_value(filepath, "autostart", "enabled", config->autostart_enabled);

    return WTS_SUCCESS;
}

void config_load_defaults(AppConfig* config) {
    if (!config) {
        return;
    }

    /* 清零结构 */
    memset(config, 0, sizeof(AppConfig));

    /* 同步配置 */
    config->sync_interval_minutes = DEFAULT_SYNC_INTERVAL_MINUTES;
    config->timeout_seconds = DEFAULT_TIMEOUT_SECONDS;
    config->max_offset_warning_ms = DEFAULT_MAX_OFFSET_WARNING_MS;
    config->sync_on_startup = true;

    /* 默认服务器列表 */
    config->server_count = DEFAULT_SERVER_COUNT;
    for (int i = 0; i < DEFAULT_SERVER_COUNT; i++) {
        strncpy_s(config->servers[i].address, MAX_SERVER_LEN,
                   DEFAULT_SERVERS[i], _TRUNCATE);
        config->servers[i].status = SERVER_STATUS_OK;
    }

    /* 日志配置 */
    config->max_log_file_size_mb = MAX_LOG_FILE_SIZE_MB;
    config->max_log_files = MAX_LOG_FILES;
    strncpy_s(config->log_path, MAX_PATH_LEN, LOG_DIR_NAME, _TRUNCATE);

    /* 通知配置 */
    config->notify_on_success = false;
    config->notify_on_failure = true;
    config->notify_on_large_offset = true;

    /* 自启动配置 */
    config->autostart_enabled = false;
}

WtsError config_add_server(AppConfig* config, const char* server) {
    if (!config || !server) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 验证服务器地址 */
    if (!is_valid_server_address(server)) {
        return WTS_ERR_CONFIG_INVALID;
    }

    /* 检查服务器数量上限 */
    if (config->server_count >= MAX_SERVERS) {
        return WTS_ERR_CONFIG_INVALID;
    }

    /* 添加服务器 */
    strncpy_s(config->servers[config->server_count].address, MAX_SERVER_LEN,
               server, _TRUNCATE);
    config->servers[config->server_count].status = SERVER_STATUS_OK;
    config->server_count++;

    return WTS_SUCCESS;
}

WtsError config_remove_server(AppConfig* config, int index) {
    if (!config) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 检查索引范围 */
    if (index < 0 || index >= config->server_count) {
        return WTS_ERR_CONFIG_INVALID;
    }

    /* 必须至少保留一个服务器 */
    if (config->server_count <= 1) {
        return WTS_ERR_CONFIG_INVALID;
    }

    /* 移除服务器（移动数组元素） */
    for (int i = index; i < config->server_count - 1; i++) {
        config->servers[i] = config->servers[i + 1];
    }
    config->server_count--;

    return WTS_SUCCESS;
}

WtsError config_move_server(AppConfig* config, int index, int direction) {
    if (!config) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 检查索引范围 */
    if (index < 0 || index >= config->server_count) {
        return WTS_ERR_CONFIG_INVALID;
    }

    /* 计算新位置 */
    int new_index = index + direction;
    if (new_index < 0 || new_index >= config->server_count) {
        return WTS_ERR_CONFIG_INVALID;
    }

    /* 交换位置 */
    NtpServer temp = config->servers[index];
    config->servers[index] = config->servers[new_index];
    config->servers[new_index] = temp;

    return WTS_SUCCESS;
}

bool config_validate_server(const char* server) {
    return is_valid_server_address(server);
}