/**
 * @file config.h
 * @brief 配置模块头文件
 * @details 提供 INI 配置文件读写、默认值、服务器列表管理功能
 */

#ifndef WINTIMESYNC_CONFIG_H
#define WINTIMESYNC_CONFIG_H

#include "common.h"

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief 加载配置文件
 * @param config 配置结构指针
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError config_load(AppConfig* config);

/**
 * @brief 保存配置文件
 * @param config 配置结构指针
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError config_save(const AppConfig* config);

/**
 * @brief 加载默认配置
 * @param config 配置结构指针
 */
void config_load_defaults(AppConfig* config);

/**
 * @brief 添加服务器到列表
 * @param config 配置结构指针
 * @param server 服务器地址
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError config_add_server(AppConfig* config, const char* server);

/**
 * @brief 从列表移除服务器
 * @param config 配置结构指针
 * @param index 服务器索引
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError config_remove_server(AppConfig* config, int index);

/**
 * @brief 设置服务器优先级（移动位置）
 * @param config 配置结构指针
 * @param index 服务器索引
 * @param direction 方向（-1 向上，1 向下）
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError config_move_server(AppConfig* config, int index, int direction);

/**
 * @brief 验证服务器地址格式
 * @param server 服务器地址
 * @return 有效返回 true，无效返回 false
 */
bool config_validate_server(const char* server);

#endif /* WINTIMESYNC_CONFIG_H */