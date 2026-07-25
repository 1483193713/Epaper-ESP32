/*
 * WiFiTest.h
 * ESP32 WiFi 连接 + 互联网连通性测试 (DNS 解析 + TCP 连接)
 *
 * 用法:
 *   WiFiTestResult r = WiFiTest_run("www.deepseek.com", 443);
 *   // r.wifiConnected / r.localIP / r.dnsOK / r.hostIP / r.tcpOK / r.tcpMs
 * 结果同时打印到串口, 并可由调用方绘制到墨水屏。
 *
 * 说明: ESP32 的 WiFi 走内部射频, 不占用墨水屏的 SPI 引脚, 不会冲突。
 */
#ifndef _WIFI_TEST_H_
#define _WIFI_TEST_H_

#include <WiFi.h>

/* ==== 在这里填写你的 WiFi ==== */
#define WIFI_SSID     "XIAOMIMI"
#define WIFI_PASSWORD "00000000"

/* 连接 WiFi 的最长等待时间 (毫秒) */
#define WIFI_CONNECT_TIMEOUT_MS 15000
/* 单次 TCP 连接的超时 (毫秒) */
#define NET_TEST_TIMEOUT_MS     5000

/* 测试结果结构体 (IP 存成 String, 方便直接画到屏上) --------------------*/
struct WiFiTestResult {
    bool     wifiConnected; // WiFi 是否连接成功
    String   localIP;       // 本机 IP
    bool     dnsOK;         // 域名是否解析成功
    String   hostIP;        // 解析出的目标 IP
    bool     tcpOK;         // TCP 连接是否成功
    uint32_t tcpMs;         // TCP 连接耗时 (毫秒)
};

/* 连接 WiFi, 返回是否成功 -----------------------------------------------*/
static bool WiFiTest_connect()
{
    if (WiFi.status() == WL_CONNECTED) return true;   // 已连上就不重连

    Serial.printf("\r\n[WiFi] Connecting to \"%s\" ...\r\n", WIFI_SSID);

    WiFi.mode(WIFI_STA);            // station 模式
    WiFi.disconnect(true);          // 清掉可能残留的连接
    delay(100);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("\r\n[WiFi] Connect TIMEOUT.");
            return false;
        }
        delay(300);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("[WiFi] Connected. IP = ");
    Serial.println(WiFi.localIP());
    Serial.printf("[WiFi] RSSI = %d dBm\r\n", WiFi.RSSI());
    return true;
}

/* 对 host:port 做一次 DNS+TCP 探测, 返回是否连通; 耗时写入 outMs -------*/
static bool WiFiTest_probe(const char *host, uint16_t port,
                           String *outHostIP, uint32_t *outMs)
{
    // DNS 解析
    Serial.printf("[DNS] Resolving %s ...\r\n", host);
    IPAddress ip;
    if (WiFi.hostByName(host, ip) != 1) {
        Serial.println("[DNS] FAILED.");
        if (outHostIP) *outHostIP = "";
        return false;
    }
    if (outHostIP) *outHostIP = ip.toString();
    Serial.print("[DNS] -> ");
    Serial.println(ip);

    // TCP 连接 (完成三次握手即证明可达互联网, 不做 TLS)
    Serial.printf("[TCP] Connecting %s:%u ...\r\n", host, port);
    WiFiClient client;
    uint32_t t0 = millis();
    bool ok = client.connect(ip, port, NET_TEST_TIMEOUT_MS);
    uint32_t dt = millis() - t0;
    if (outMs) *outMs = dt;

    if (ok) {
        Serial.printf("[TCP] Connected OK, %lu ms\r\n", (unsigned long)dt);
        client.stop();
    } else {
        Serial.printf("[TCP] FAILED (%lu ms)\r\n", (unsigned long)dt);
    }
    return ok;
}

/* 运行完整测试: 连 WiFi -> DNS 解析 -> TCP 连接 -------------------------*/
static WiFiTestResult WiFiTest_run(const char *host, uint16_t port)
{
    WiFiTestResult r;
    r.wifiConnected = false;
    r.localIP = "";
    r.dnsOK   = false;
    r.hostIP  = "";
    r.tcpOK   = false;
    r.tcpMs   = 0;

    // 1) 连 WiFi
    r.wifiConnected = WiFiTest_connect();
    if (!r.wifiConnected) return r;
    r.localIP = WiFi.localIP().toString();

    // 2) DNS + 3) TCP
    r.tcpOK = WiFiTest_probe(host, port, &r.hostIP, &r.tcpMs);
    r.dnsOK = (r.hostIP.length() > 0);
    return r;
}

/* 复测: 只打串口, 不改屏 (用于 loop 周期性检查) ------------------------*/
static void WiFiTest_pingOnce(const char *host, uint16_t port)
{
    if (!WiFiTest_connect()) {
        Serial.println("[PING] WiFi down.");
        return;
    }
    String ipStr;
    uint32_t ms = 0;
    bool ok = WiFiTest_probe(host, port, &ipStr, &ms);
    Serial.printf("[PING] %s -> %s : %s (%lu ms)\r\n",
                  host, ipStr.c_str(),
                  ok ? "OK" : "FAIL", (unsigned long)ms);
}

#endif
