/*
 * BeibeiBalance.h
 * ESP32 通过 HTTPS 查询贝贝AI中转站账户余额
 *   GET https://api.beibeiai.top/api/user/self
 *   Header: Cookie: session=<SESSION>
 *   Header: Referer: https://api.beibeiai.top/console
 *
 * 返回示例:
 * {
 *   "success": true,
 *   "data": {
 *     "username": "Damahou",
 *     "email": "1483193713@qq.com",
 *     "quota": 17670002,
 *     "used_quota": 19829998,
 *     "request_count": 1351
 *   }
 * }
 *
 * 兑换比例: 1 USD = 500,000 quota
 *
 * 说明: 用 setInsecure() 跳过 TLS 证书校验 (先跑通); JSON 用手动解析, 不依赖外部库。
 * 需先连上 WiFi (见 WiFiTest.h 的 WiFiTest_connect)。
 * Session Cookie 存于 secrets.h (被 .gitignore 忽略)。
 */
#ifndef _BEIBEI_BALANCE_H_
#define _BEIBEI_BALANCE_H_

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"

/* Session 从 secrets.h 读取 */
#define BEIBEI_SESSION      SECRET_BEIBEI_SESSION
#define BEIBEI_USER_ID      SECRET_BEIBEI_USER_ID

#define BEIBEI_BALANCE_URL    "https://api.beibeiai.top/api/user/self"
#define BEIBEI_REFERER        "https://api.beibeiai.top/console"
#define BEIBEI_HTTP_TIMEOUT_MS 10000

/* 兑换比例: 1 USD = 500,000 quota */
#define BEIBEI_QUOTA_PER_USD  500000.0

/* 查询结果结构体 --------------------------------------------------------*/
struct BBalance {
    bool     httpOk;          // HTTP 请求是否成功 (拿到 2xx)
    int      httpCode;        // HTTP 状态码 (调试用)
    bool     success;         // API 返回 success
    String   username;        // 用户名
    String   email;           // 邮箱
    int64_t  quota;           // 剩余额度 (quota 点数)
    int64_t  usedQuota;       // 已使用额度
    int      requestCount;    // 请求次数
};

/* 从 JSON 文本里提取字符串型字段的值: "key":"value" -----------------------*/
static String BB_extractStr(const String &json, const char *key)
{
    String pat = String("\"") + key + "\"";
    int k = json.indexOf(pat);
    if (k < 0) return "";
    int colon = json.indexOf(':', k + pat.length());
    if (colon < 0) return "";
    int q1 = json.indexOf('"', colon + 1);
    if (q1 < 0) return "";
    int q2 = json.indexOf('"', q1 + 1);
    if (q2 < 0) return "";
    return json.substring(q1 + 1, q2);
}

/* 提取布尔字段: "key":true / false ---------------------------------------*/
static bool BB_extractBool(const String &json, const char *key)
{
    String pat = String("\"") + key + "\"";
    int k = json.indexOf(pat);
    if (k < 0) return false;
    int colon = json.indexOf(':', k + pat.length());
    if (colon < 0) return false;
    String seg = json.substring(colon + 1, colon + 8);
    return seg.indexOf("true") >= 0;
}

/* 提取整数字段: "key":123456 --------------------------------------------*/
static int64_t BB_extractInt(const String &json, const char *key)
{
    String pat = String("\"") + key + "\"";
    int k = json.indexOf(pat);
    if (k < 0) return 0;
    int colon = json.indexOf(':', k + pat.length());
    if (colon < 0) return 0;
    // 跳过冒号后的空格
    int start = colon + 1;
    while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\t'))
        start++;
    // 取连续数字
    String num;
    while (start < (int)json.length() && json[start] >= '0' && json[start] <= '9') {
        num += json[start];
        start++;
    }
    if (num.length() == 0) return 0;
    return num.toInt();   // ESP32 Arduino String::toInt() 是 32-bit, 但 quota 可能很大
    // 实际上 quota 如 17670002 < 2^31, toInt() 够用, 这里用 atoll 更安全
    // return atoll(num.c_str());   // 但没这个函数, 用 toInt() 足够
}

/* 提取 int64 字段 — 手动解析, 兼容 32-bit 系统 ----------------------------*/
static int64_t BB_extractInt64(const String &json, const char *key)
{
    String pat = String("\"") + key + "\"";
    int k = json.indexOf(pat);
    if (k < 0) return 0;
    int colon = json.indexOf(':', k + pat.length());
    if (colon < 0) return 0;
    int start = colon + 1;
    while (start < (int)json.length() &&
           (json[start] == ' ' || json[start] == '\t'))
        start++;
    int64_t val = 0;
    while (start < (int)json.length() && json[start] >= '0' && json[start] <= '9') {
        val = val * 10 + (json[start] - '0');
        start++;
    }
    return val;
}

/* 查询贝贝AI余额 --------------------------------------------------------*/
static BBalance Beibei_getBalance()
{
    BBalance b;
    b.httpOk       = false;
    b.httpCode     = 0;
    b.success      = false;
    b.username     = "";
    b.email        = "";
    b.quota        = 0;
    b.usedQuota    = 0;
    b.requestCount = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[BB] WiFi not connected.");
        return b;
    }

    WiFiClientSecure client;
    client.setInsecure();               // 跳过证书校验 (先跑通)

    HTTPClient https;
    https.setTimeout(BEIBEI_HTTP_TIMEOUT_MS);

    Serial.println("[BB] GET " BEIBEI_BALANCE_URL);
    if (!https.begin(client, BEIBEI_BALANCE_URL)) {
        Serial.println("[BB] https.begin() failed.");
        return b;
    }

    // 设置请求头
    https.addHeader("Cookie", "session=" BEIBEI_SESSION);
    https.addHeader("Referer", BEIBEI_REFERER);
    https.addHeader("New-Api-User", BEIBEI_USER_ID);
    https.addHeader("Accept", "application/json");

    int code = https.GET();
    b.httpCode = code;
    Serial.printf("[BB] HTTP code = %d\r\n", code);

    if (code == HTTP_CODE_OK) {
        String payload = https.getString();
        Serial.println("[BB] payload:");
        Serial.println(payload);

        b.success = BB_extractBool(payload, "success");

        if (b.success) {
            // 先定位 "data" 对象
            int dataIdx = payload.indexOf("\"data\"");
            String seg = (dataIdx >= 0) ? payload.substring(dataIdx) : payload;

            b.username     = BB_extractStr(seg, "username");
            b.email        = BB_extractStr(seg, "email");
            b.quota        = (int64_t)BB_extractInt64(seg, "quota");
            b.usedQuota    = (int64_t)BB_extractInt64(seg, "used_quota");
            b.requestCount = (int)BB_extractInt64(seg, "request_count");

            // 计算美元余额
            float balanceUSD = (float)b.quota / BEIBEI_QUOTA_PER_USD;
            float usedUSD    = (float)b.usedQuota / BEIBEI_QUOTA_PER_USD;

            Serial.printf("[BB] user=%s email=%s\r\n", b.username.c_str(), b.email.c_str());
            Serial.printf("[BB] quota=%lld (%.2f USD) used=%lld (%.2f USD) req=%d\r\n",
                          b.quota, balanceUSD, b.usedQuota, usedUSD, b.requestCount);
        } else {
            String msg = BB_extractStr(payload, "message");
            Serial.printf("[BB] API success=false, message=%s\r\n", msg.c_str());
        }
    } else {
        Serial.printf("[BB] request failed: %s\r\n", https.errorToString(code).c_str());
        String err = https.getString();
        if (err.length()) { Serial.println(err); }
    }

    https.end();
    return b;
}

#endif
