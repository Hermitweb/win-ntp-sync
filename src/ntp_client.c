/**
 * @file ntp_client.c
 * @brief NTP 客户端模块实现
 * @details 提供 NTPv4 (RFC 5905) 报文构造与解析、UDP socket 通信、时间戳转换功能
 */

#include "ntp_client.h"
#include "log.h"
#include <stdio.h>
#include <math.h>
#include <winsock2.h>
#include <ws2tcpip.h>

/* ============================================================================
 * 常量定义
 * ============================================================================ */

/* Unix 纪元（1970）到 NTP 纪元（1900）的秒数差 */
#define NTP_TO_UNIX_OFFSET 2208988800ULL

/* NTP 时间戳精度：1/2^32 秒 ≈ 233 皮秒 */
#define NTP_FRAC_UNIT 4294967296.0

/* ============================================================================
 * 内部函数
 * ============================================================================ */

/**
 * @brief 网络字节序转换：32 位整数
 */
static uint32_t ntohl_ntp(uint32_t net_val) {
    return ntohl(net_val);
}

/**
 * @brief 主机字节序转换：32 位整数
 */
static uint32_t htonl_ntp(uint32_t host_val) {
    return htonl(host_val);
}

/**
 * @brief 解析域名或 IP 地址
 * @param server 服务器地址
 * @param addr 输出地址结构
 * @return 成功返回 WTS_SUCCESS，失败返回错误码
 */
static WtsError resolve_server_address(const char* server, struct sockaddr_in* addr) {
    struct addrinfo hints;
    struct addrinfo* result = NULL;
    int ret;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;      /* IPv4 */
    hints.ai_socktype = SOCK_DGRAM; /* UDP */
    hints.ai_protocol = IPPROTO_UDP;

    ret = getaddrinfo(server, NULL, &hints, &result);
    if (ret != 0) {
        LOG_ERROR("DNS 解析失败: %s, 错误: %d", server, ret);
        return WTS_ERR_DNS_RESOLVE;
    }

    /* 获取第一个地址 */
    memcpy(addr, result->ai_addr, sizeof(struct sockaddr_in));
    addr->sin_port = htons(NTP_PORT);

    freeaddrinfo(result);
    return WTS_SUCCESS;
}

/**
 * @brief 设置 socket 超时
 * @param sockfd socket 句柄
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功返回 true，失败返回 false
 */
static bool set_socket_timeout(SOCKET sockfd, int timeout_ms) {
    DWORD timeout = timeout_ms;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&timeout, sizeof(timeout)) != 0) {
        LOG_ERROR("设置接收超时失败: %d", WSAGetLastError());
        return false;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,
                   (const char*)&timeout, sizeof(timeout)) != 0) {
        LOG_ERROR("设置发送超时失败: %d", WSAGetLastError());
        return false;
    }
    return true;
}

/* ============================================================================
 * 时间戳转换函数
 * ============================================================================ */

void ntp_get_current_time(NtpTimestamp* timestamp) {
    FILETIME filetime;
    GetSystemTimeAsFileTime(&filetime);
    filetime_to_ntp(&filetime, timestamp);
}

void filetime_to_ntp(const FILETIME* filetime, NtpTimestamp* ntp) {
    /* FILETIME 是 100ns 单位，从 1601-01-01 开始 */
    /* NTP 是秒 + 小数，从 1900-01-01 开始 */
    ULARGE_INTEGER ul;
    ul.LowPart = filetime->dwLowDateTime;
    ul.HighPart = filetime->dwHighDateTime;

    /* 减去偏移量（1900-1601），转换为秒 */
    ul.QuadPart -= NTP_TO_FILETIME_OFFSET;

    /* 转换为 NTP 格式 */
    ntp->seconds = (uint32_t)(ul.QuadPart / 10000000ULL);
    ntp->fraction = (uint32_t)((ul.QuadPart % 10000000ULL) * NTP_FRAC_UNIT / 10000000.0);
}

void ntp_to_filetime(const NtpTimestamp* ntp, FILETIME* filetime) {
    ULARGE_INTEGER ul;

    /* 转换为 100ns 单位 */
    ul.QuadPart = (ntp->seconds * 10000000ULL) +
                  (uint64_t)(ntp->fraction / NTP_FRAC_UNIT * 10000000.0);

    /* 加上偏移量 */
    ul.QuadPart += NTP_TO_FILETIME_OFFSET;

    filetime->dwLowDateTime = ul.LowPart;
    filetime->dwHighDateTime = ul.HighPart;
}

uint32_t ntp_to_unix_seconds(const NtpTimestamp* ntp) {
    /* Unix 纪元是 1970，NTP 纪元是 1900，差值 2208988800 秒 */
    return ntp->seconds - NTP_TO_UNIX_OFFSET;
}

double ntp_to_ms(const NtpTimestamp* ntp) {
    return ntp->seconds * 1000.0 + ntp->fraction / NTP_FRAC_UNIT * 1000.0;
}

double ntp_diff_ms(const NtpTimestamp* t1, const NtpTimestamp* t2) {
    double ms1 = ntp_to_ms(t1);
    double ms2 = ntp_to_ms(t2);
    return ms2 - ms1;
}

/* ============================================================================
 * NTP 报文函数
 * ============================================================================ */

void ntp_build_request(NtpPacket* packet) {
    memset(packet, 0, sizeof(NtpPacket));

    /* 设置 LI=0, VN=4, Mode=3 (client) */
    packet->li_vn_mode = (0 << 6) | (NTP_VERSION << 3) | NTP_MODE_CLIENT;

    /* Stratum = 0 (未指定) */
    packet->stratum = 0;

    /* Poll = 4 (16 秒间隔) */
    packet->poll = 4;

    /* Precision = -6 (约 15.625 微秒精度) */
    packet->precision = -6;

    /* 设置发送时间戳 */
    ntp_get_current_time(&packet->xmit_time);
}

WtsError ntp_parse_response(const NtpPacket* packet) {
    if (!packet) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 检查版本号 */
    uint8_t version = (packet->li_vn_mode >> 3) & 0x07;
    if (version != NTP_VERSION) {
        LOG_WARNING("NTP 版本不匹配: %d (期望 %d)", version, NTP_VERSION);
        return WTS_ERR_NTP_INVALID_VERSION;
    }

    /* 检查模式 */
    uint8_t mode = packet->li_vn_mode & 0x07;
    if (mode != NTP_MODE_SERVER) {
        LOG_WARNING("NTP 模式不正确: %d (期望 %d)", mode, NTP_MODE_SERVER);
        return WTS_ERR_NTP_INVALID_MODE;
    }

    /* 检查 Kiss-o'-Death */
    if (ntp_is_kiss_of_death(packet)) {
        char kod_code[5];
        ntp_get_kod_code(packet, kod_code, sizeof(kod_code));
        LOG_WARNING("收到 Kiss-o'-Death: %s", kod_code);
        return WTS_ERR_NTP_KISS_OF_DEATH;
    }

    /* 检查 Stratum */
    if (packet->stratum == 0 || packet->stratum >= NTP_STRATUM_UNSPEC) {
        LOG_WARNING("NTP Stratum 无效: %d", packet->stratum);
        return WTS_ERR_NTP_INVALID_STRATUM;
    }

    return WTS_SUCCESS;
}

bool ntp_is_kiss_of_death(const NtpPacket* packet) {
    /* KoD 的特征：stratum = 0 */
    return packet->stratum == 0;
}

void ntp_get_kod_code(const NtpPacket* packet, char* kod_code, size_t size) {
    /* KoD 代码存储在 Reference ID 字段的前 4 个字节 */
    if (size >= 5) {
        memcpy(kod_code, &packet->reference_id, 4);
        kod_code[4] = '\0';
    }
}

/* ============================================================================
 * NTP 通信函数
 * ============================================================================ */

WtsError ntp_init_winsock(void) {
    WSADATA wsa_data;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (ret != 0) {
        LOG_ERROR("Winsock 初始化失败: %d", ret);
        return WTS_ERR_SOCKET_INIT;
    }
    LOG_INFO("Winsock 初始化成功");
    return WTS_SUCCESS;
}

void ntp_cleanup_winsock(void) {
    WSACleanup();
    LOG_INFO("Winsock 清理完成");
}

WtsError ntp_sync(const char* server, int timeout_ms, SyncResult* result) {
    if (!server || !result) {
        return WTS_ERR_NULL_PARAM;
    }

    /* 初始化结果 */
    memset(result, 0, sizeof(SyncResult));
    strncpy_s(result->server, MAX_SERVER_LEN, server, _TRUNCATE);

    LOG_INFO("开始 NTP 同步: %s (超时: %d ms)", server, timeout_ms);

    /* 解析服务器地址 */
    struct sockaddr_in server_addr;
    WtsError err = resolve_server_address(server, &server_addr);
    if (err != WTS_SUCCESS) {
        result->error_code = err;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "DNS 解析失败: %s", server);
        return err;
    }

    /* 创建 UDP socket */
    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        LOG_ERROR("创建 socket 失败: %d", WSAGetLastError());
        result->error_code = WTS_ERR_SOCKET_CREATE;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "创建 socket 失败");
        return WTS_ERR_SOCKET_CREATE;
    }

    /* 设置超时 */
    if (!set_socket_timeout(sockfd, timeout_ms)) {
        closesocket(sockfd);
        result->error_code = WTS_ERR_SOCKET_TIMEOUT;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "设置 socket 超时失败");
        return WTS_ERR_SOCKET_TIMEOUT;
    }

    /* 构造 NTP 请求报文 */
    NtpPacket request;
    ntp_build_request(&request);

    /* 记录发送时间 T1 */
    NtpTimestamp t1 = request.xmit_time;
    FILETIME t1_filetime;
    ntp_to_filetime(&t1, &t1_filetime);

    /* 转换为网络字节序 */
    NtpPacket net_request;
    memcpy(&net_request, &request, sizeof(NtpPacket));
    net_request.xmit_time.seconds = htonl_ntp(request.xmit_time.seconds);
    net_request.xmit_time.fraction = htonl_ntp(request.xmit_time.fraction);

    /* 发送请求 */
    int sent = sendto(sockfd, (const char*)&net_request, sizeof(net_request), 0,
                      (const struct sockaddr*)&server_addr, sizeof(server_addr));
    if (sent != sizeof(net_request)) {
        LOG_ERROR("发送 NTP 请求失败: %d", WSAGetLastError());
        closesocket(sockfd);
        result->error_code = WTS_ERR_SOCKET_SEND;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "发送请求失败");
        return WTS_ERR_SOCKET_SEND;
    }

    /* 接收响应 */
    NtpPacket response;
    struct sockaddr_in from_addr;
    int from_len = sizeof(from_addr);
    int received = recvfrom(sockfd, (char*)&response, sizeof(response), 0,
                            (struct sockaddr*)&from_addr, &from_len);

    /* 记录接收时间 T4 */
    NtpTimestamp t4;
    ntp_get_current_time(&t4);

    if (received != sizeof(response)) {
        int wsa_error = WSAGetLastError();
        if (wsa_error == WSAETIMEDOUT || wsa_error == WSAEWOULDBLOCK) {
            LOG_WARNING("NTP 请求超时: %s", server);
            closesocket(sockfd);
            result->error_code = WTS_ERR_SOCKET_TIMEOUT;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "请求超时 (%d ms)", timeout_ms);
            return WTS_ERR_SOCKET_TIMEOUT;
        }
        LOG_ERROR("接收 NTP 响应失败: %d", wsa_error);
        closesocket(sockfd);
        result->error_code = WTS_ERR_SOCKET_RECV;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "接收响应失败");
        return WTS_ERR_SOCKET_RECV;
    }

    closesocket(sockfd);

    /* 转换为主机字节序 */
    NtpPacket host_response;
    memcpy(&host_response, &response, sizeof(NtpPacket));
    host_response.orig_time.seconds = ntohl_ntp(response.orig_time.seconds);
    host_response.orig_time.fraction = ntohl_ntp(response.orig_time.fraction);
    host_response.recv_time.seconds = ntohl_ntp(response.recv_time.seconds);
    host_response.recv_time.fraction = ntohl_ntp(response.recv_time.fraction);
    host_response.xmit_time.seconds = ntohl_ntp(response.xmit_time.seconds);
    host_response.xmit_time.fraction = ntohl_ntp(response.xmit_time.fraction);

    /* 解析响应 */
    err = ntp_parse_response(&host_response);
    if (err != WTS_SUCCESS) {
        result->error_code = err;
        if (err == WTS_ERR_NTP_KISS_OF_DEATH) {
            char kod_code[5];
            ntp_get_kod_code(&host_response, kod_code, sizeof(kod_code));
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Kiss-o'-Death: %s", kod_code);
        } else {
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "响应报文无效");
        }
        return err;
    }

    /* 提取时间戳 */
    /* T1: 发起时间戳（客户端发送时间）*/
    NtpTimestamp t2 = host_response.recv_time;  /* T2: 服务器接收时间 */
    NtpTimestamp t3 = host_response.xmit_time;  /* T3: 服务器发送时间 */
    /* T4: 客户端接收时间（已记录）*/

    /* 计算偏移量和延迟（RFC 5905）*/
    /* delay  = (T4 - T1) - (T3 - T2) */
    /* offset = ((T2 - T1) + (T3 - T4)) / 2 */
    double t4_t1_diff = ntp_diff_ms(&t1, &t4);
    double t3_t2_diff = ntp_diff_ms(&t2, &t3);
    double t2_t1_diff = ntp_diff_ms(&t1, &t2);
    double t3_t4_diff = ntp_diff_ms(&t4, &t3);

    result->delay_ms = t4_t1_diff - t3_t2_diff;
    result->offset_ms = (t2_t1_diff + t3_t4_diff) / 2.0;

    /* 检查结果有效性 */
    if (result->delay_ms < 0) {
        LOG_WARNING("计算延迟为负值，可能时钟偏差过大");
        result->delay_ms = fabs(result->delay_ms);
    }

    /* 获取服务器时间 */
    FILETIME server_time;
    ntp_to_filetime(&t3, &server_time);

    /* 记录同步完成时间 */
    GetSystemTimeAsFileTime(&result->sync_time);

    LOG_INFO("NTP 同步成功: 偏移 %.2f ms, 延迟 %.2f ms",
             result->offset_ms, result->delay_ms);

    result->status = SYNC_STATUS_SUCCESS;
    result->error_code = WTS_SUCCESS;
    return WTS_SUCCESS;
}