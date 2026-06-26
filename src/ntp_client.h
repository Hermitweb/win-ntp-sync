/**
 * @file ntp_client.h
 * @brief NTP 客户端模块头文件
 * @details 提供 NTPv4 (RFC 5905) 报文构造与解析、UDP socket 通信、时间戳转换功能
 */

#ifndef WINTIMESYNC_NTP_CLIENT_H
#define WINTIMESYNC_NTP_CLIENT_H

#include "common.h"

/* ============================================================================
 * Kiss-o'-Death 代码定义
 * ============================================================================ */

#define KOD_DENY    "DENY"
#define KOD_RSTR    "RSTR"
#define KOD_RATE    "RATE"
#define KOD_KISS    "KISS"

/* ============================================================================
 * 时间戳转换函数
 * ============================================================================ */

/**
 * @brief 获取当前 NTP 时间戳
 * @param timestamp 输出时间戳
 */
void ntp_get_current_time(NtpTimestamp* timestamp);

/**
 * @brief 将 FILETIME 转换为 NTP 时间戳
 * @param filetime FILETIME 时间戳
 * @param ntp 输出 NTP 时间戳
 */
void filetime_to_ntp(const FILETIME* filetime, NtpTimestamp* ntp);

/**
 * @brief 将 NTP 时间戳转换为 FILETIME
 * @param ntp NTP 时间戳
 * @param filetime 输出 FILETIME
 */
void ntp_to_filetime(const NtpTimestamp* ntp, FILETIME* filetime);

/**
 * @brief 将 NTP 时间戳转换为 Unix 时间戳（秒）
 * @param ntp NTP 时间戳
 * @return Unix 时间戳（秒）
 */
uint32_t ntp_to_unix_seconds(const NtpTimestamp* ntp);

/**
 * @brief 将 NTP 时间戳转换为毫秒
 * @param ntp NTP 时间戳
 * @return 毫秒时间戳
 */
double ntp_to_ms(const NtpTimestamp* ntp);

/**
 * @brief 计算两个 NTP 时间戳的差值（毫秒）
 * @param t1 时间戳 1
 * @param t2 时间戳 2
 * @return 差值（毫秒），t2 - t1
 */
double ntp_diff_ms(const NtpTimestamp* t1, const NtpTimestamp* t2);

/* ============================================================================
 * NTP 报文函数
 * ============================================================================ */

/**
 * @brief 构造 NTP 客户端请求报文
 * @param packet 输出报文缓冲区
 */
void ntp_build_request(NtpPacket* packet);

/**
 * @brief 解析 NTP 服务器响应报文
 * @param packet 接收到的报文
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError ntp_parse_response(const NtpPacket* packet);

/**
 * @brief 检查是否为 Kiss-o'-Death 响应
 * @param packet 接收到的报文
 * @return 是 KoD 返回 true，否则返回 false
 */
bool ntp_is_kiss_of_death(const NtpPacket* packet);

/**
 * @brief 获取 Kiss-o'-Death 代码
 * @param packet 接收到的报文
 * @param kod_code 输出 KoD 代码缓冲区
 * @param size 缓冲区大小
 */
void ntp_get_kod_code(const NtpPacket* packet, char* kod_code, size_t size);

/* ============================================================================
 * NTP 通信函数
 * ============================================================================ */

/**
 * @brief 初始化 Winsock
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError ntp_init_winsock(void);

/**
 * @brief 清理 Winsock
 */
void ntp_cleanup_winsock(void);

/**
 * @brief 执行 NTP 同步请求
 * @param server 服务器地址（域名或 IP）
 * @param timeout_ms 超时时间（毫秒）
 * @param result 输出同步结果
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
WtsError ntp_sync(const char* server, int timeout_ms, SyncResult* result);

#endif /* WINTIMESYNC_NTP_CLIENT_H */