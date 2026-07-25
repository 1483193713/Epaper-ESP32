/*
 * DeepSeekBalance.h
 * ESP32 通过 HTTPS 查询 DeepSeek 账户余额
 *   GET https://api.deepseek.com/user/balance
 *   Header: Authorization: Bearer <API_KEY>
 *
 * 返回示例:
 * {
 *   "is_available": true,
 *   "balance_infos": [
 *     { "currency": "CNY",
 *       "total_balance": "110.00",
 *       "granted_balance": "10.00",
 *       "topped_up_balance": "100.00" }
 *   ]
 * }
 *
 * 说明: 用 setInsecure() 跳过 TLS 证书校验 (先跑通); JSON 用手动解析, 不依赖外部库。
 * 需先连上 WiFi (见 WiFiTest.h 的 WiFiTest_connect)。
 */
#ifndef _DEEPSEEK_BALANCE_H_
#define _DEEPSEEK_BALANCE_H_

#include <WiFiClientSecure.h>
#include <HTTPClient.h>

/* ==== 在这里填写你的 DeepSeek API Key (形如 sk-xxxx) ==== */
#define DEEPSEEK_API_KEY "REPLACE_WITH_YOUR_KEY"

#define DEEPSEEK_BALANCE_URL "https://api.deepseek.com/user/balance"
#define DEEPSEEK_HTTP_TIMEOUT_MS 10000

/* 查询结果结构体 --------------------------------------------------------*/
struct DSBalance {
    bool   httpOk;          // HTTP 请求是否成功 (拿到 2xx)
    int    httpCode;        // HTTP 状态码 (调试用)
    bool   isAvailable;     // 账户是否可用
    String currency;        // 货币 (CNY / USD)
    String totalBalance;    // 总余额
    String grantedBalance;  // 赠金余额
    String toppedUpBalance; // 充值余额
};

/* 从 JSON 文本里提取字符串型字段的值: "key":"value" -----------------------*/
static String DS_extractStr(const String &json, const char *key)
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
static bool DS_extractBool(const String &json, const char *key)
{
    String pat = String("\"") + key + "\"";
    int k = json.indexOf(pat);
    if (k < 0) return false;
    int colon = json.indexOf(':', k + pat.length());
    if (colon < 0) return false;
    // 取冒号后一小段, 看是 true 还是 false
    String seg = json.substring(colon + 1, colon + 8);
    return seg.indexOf("true") >= 0;
}

/* 查询余额 --------------------------------------------------------------*/
static DSBalance DeepSeek_getBalance()
{
    DSBalance b;
    b.httpOk = false;
    b.httpCode = 0;
    b.isAvailable = false;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[DS] WiFi not connected.");
        return b;
    }

    WiFiClientSecure client;
    client.setInsecure();               // 跳过证书校验 (先跑通)

    HTTPClient https;
    https.setTimeout(DEEPSEEK_HTTP_TIMEOUT_MS);

    Serial.println("[DS] GET " DEEPSEEK_BALANCE_URL);
    if (!https.begin(client, DEEPSEEK_BALANCE_URL)) {
        Serial.println("[DS] https.begin() failed.");
        return b;
    }
    https.addHeader("Authorization", "Bearer " DEEPSEEK_API_KEY);
    https.addHeader("Accept", "application/json");

    int code = https.GET();
    b.httpCode = code;
    Serial.printf("[DS] HTTP code = %d\r\n", code);

    if (code == HTTP_CODE_OK) {
        String payload = https.getString();
        Serial.println("[DS] payload:");
        Serial.println(payload);

        b.httpOk      = true;
        b.isAvailable = DS_extractBool(payload, "is_available");

        // balance_infos 是数组, 可能同时含 USD 和 CNY 两条。
        // 先定位到 "currency":"CNY" 那一段, 再从该段之后提取余额字段。
        int cny = payload.indexOf("\"CNY\"");
        String seg = (cny >= 0) ? payload.substring(cny) : payload;

        b.currency        = "CNY";
        b.totalBalance    = DS_extractStr(seg, "total_balance");
        b.grantedBalance  = DS_extractStr(seg, "granted_balance");
        b.toppedUpBalance = DS_extractStr(seg, "topped_up_balance");

        // 万一账户没有 CNY 记录 (取不到), 退回取第一条币种
        if (b.totalBalance.length() == 0) {
            b.currency        = DS_extractStr(payload, "currency");
            b.totalBalance    = DS_extractStr(payload, "total_balance");
            b.grantedBalance  = DS_extractStr(payload, "granted_balance");
            b.toppedUpBalance = DS_extractStr(payload, "topped_up_balance");
        }

        Serial.printf("[DS] %s total=%s granted=%s topup=%s available=%d\r\n",
                      b.currency.c_str(), b.totalBalance.c_str(),
                      b.grantedBalance.c_str(), b.toppedUpBalance.c_str(),
                      b.isAvailable);
    } else {
        Serial.printf("[DS] request failed: %s\r\n",
                      https.errorToString(code).c_str());
        // 打印错误响应体, 便于排查 (如 401 无效 Key)
        String err = https.getString();
        if (err.length()) { Serial.println(err); }
    }

    https.end();
    return b;
}

#endif
