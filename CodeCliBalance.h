/*
 * CodeCliBalance.h
 * ESP32 通过 HTTPS 查询 Code-CLI 中转站账户余额
 *   GET https://code-cli.cn/api/user/self
 *   Header: Cookie: session=<SESSION>
 *   Header: New-Api-User: <USER_ID>
 *   Header: Referer: https://code-cli.cn/dashboard/models
 *
 * 返回示例:
 * {
 *   "success": true,
 *   "data": {
 *     "username": "1483193713",
 *     "email": "1483193713@qq.com",
 *     "quota": 4771616,
 *     "used_quota": 228384,
 *     "request_count": 51
 *   }
 * }
 *
 * 兑换比例: 1 USD = 500,000 quota (同 New API 标准)
 *
 * 说明: 用 setInsecure() 跳过 TLS 证书校验 (先跑通); JSON 用手动解析, 不依赖外部库。
 * 需先连上 WiFi (见 WiFiTest.h 的 WiFiTest_connect)。
 * Session Cookie 存于 secrets.h (被 .gitignore 忽略)。
 */
#ifndef _CODECLI_BALANCE_H_
#define _CODECLI_BALANCE_H_

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"

/* Session 从 secrets.h 读取 */
#define CODECLI_SESSION      SECRET_CODECLI_SESSION
#define CODECLI_USER_ID      SECRET_CODECLI_USER_ID

#define CODECLI_BALANCE_URL     "https://code-cli.cn/api/user/self"
#define CODECLI_REFERER         "https://code-cli.cn/dashboard/models"
#define CODECLI_HTTP_TIMEOUT_MS 10000

/* 兑换比例: 1 USD = 500,000 quota */
#define CODECLI_QUOTA_PER_USD  500000.0

/* 查询结果结构体 --------------------------------------------------------*/
struct CCBalance {
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
static String CC_extractStr(const String &json, const char *key)
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
static bool CC_extractBool(const String &json, const char *key)
{
    String pat = String("\"") + key + "\"";
    int k = json.indexOf(pat);
    if (k < 0) return false;
    int colon = json.indexOf(':', k + pat.length());
    if (colon < 0) return false;
    String seg = json.substring(colon + 1, colon + 8);
    return seg.indexOf("true") >= 0;
}

/* 提取 int64 字段 — 手动解析, 兼容 32-bit 系统 ----------------------------*/
static int64_t CC_extractInt64(const String &json, const char *key)
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

/* 查询 Code-CLI 余额 ----------------------------------------------------*/
static CCBalance CodeCli_getBalance()
{
    CCBalance c;
    c.httpOk       = false;
    c.httpCode     = 0;
    c.success      = false;
    c.username     = "";
    c.email        = "";
    c.quota        = 0;
    c.usedQuota    = 0;
    c.requestCount = 0;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[CC] WiFi not connected.");
        return c;
    }

    WiFiClientSecure client;
    client.setInsecure();               // 跳过证书校验 (先跑通)

    HTTPClient https;
    https.setTimeout(CODECLI_HTTP_TIMEOUT_MS);

    Serial.println("[CC] GET " CODECLI_BALANCE_URL);
    if (!https.begin(client, CODECLI_BALANCE_URL)) {
        Serial.println("[CC] https.begin() failed.");
        return c;
    }

    // 设置请求头
    https.addHeader("Cookie", "session=" CODECLI_SESSION);
    https.addHeader("Referer", CODECLI_REFERER);
    https.addHeader("New-Api-User", CODECLI_USER_ID);
    https.addHeader("Accept", "application/json");

    int code = https.GET();
    c.httpCode = code;
    Serial.printf("[CC] HTTP code = %d\r\n", code);

    if (code == HTTP_CODE_OK) {
        String payload = https.getString();
        Serial.println("[CC] payload:");
        Serial.println(payload);

        c.success = CC_extractBool(payload, "success");

        if (c.success) {
            // 先定位 "data" 对象
            int dataIdx = payload.indexOf("\"data\"");
            String seg = (dataIdx >= 0) ? payload.substring(dataIdx) : payload;

            c.username     = CC_extractStr(seg, "username");
            c.email        = CC_extractStr(seg, "email");
            c.quota        = (int64_t)CC_extractInt64(seg, "quota");
            c.usedQuota    = (int64_t)CC_extractInt64(seg, "used_quota");
            c.requestCount = (int)CC_extractInt64(seg, "request_count");

            // 计算美元余额
            float balanceUSD = (float)c.quota / CODECLI_QUOTA_PER_USD;
            float usedUSD    = (float)c.usedQuota / CODECLI_QUOTA_PER_USD;

            Serial.printf("[CC] user=%s email=%s\r\n", c.username.c_str(), c.email.c_str());
            Serial.printf("[CC] quota=%lld (%.2f USD) used=%lld (%.2f USD) req=%d\r\n",
                          c.quota, balanceUSD, c.usedQuota, usedUSD, c.requestCount);
        } else {
            String msg = CC_extractStr(payload, "message");
            Serial.printf("[CC] API success=false, message=%s\r\n", msg.c_str());
        }
    } else {
        Serial.printf("[CC] request failed: %s\r\n", https.errorToString(code).c_str());
        String err = https.getString();
        if (err.length()) { Serial.println(err); }
    }

    https.end();
    return c;
}

#endif
