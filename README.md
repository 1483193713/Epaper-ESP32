# ESP32 AI 账户余额墨水屏监控器

> 基于 ESP32 + 微雪 1.54 寸墨水屏，实时显示 DeepSeek 官方账户余额和贝贝AI 中转站余额/Token。

![效果示意](https://img.shields.io/badge/Platform-ESP32-blue) ![License](https://img.shields.io/badge/License-MIT-green)

---

## 功能介绍

设备通电后自动执行以下流程：

1. **连接 WiFi**，打印本机 IP 和信号强度到串口
2. **同步网络时间**（NTP，UTC+8），用于屏幕上显示当前时刻
3. **查询 DeepSeek 官方余额**（HTTPS，Bearer Token 鉴权，优先取 CNY 币种）
4. **查询贝贝AI 中转站余额**（HTTPS，Cookie Session 鉴权，按 1 USD = 500,000 quota 换算）
5. **绘制到墨水屏**：双 logo、币种、余额数值、剩余 Token、当前时间，布局为「田」字格
6. **每 2 分钟自动刷新**一次余额（可通过 `REFRESH_INTERVAL_MS` 调整）

### 屏幕布局

```
┌──────────────────────────────────────────┐
│  DeepSeek logo  │  CNY (币种)            │
│  (100×100 px)   │  110.00 (余额)         │
│                 │  17:30:00 (当前时间)   │
├─────────────────┼────────────────────────┤
│  贝贝AI logo    │  CNY: (标签)           │
│  (100×100 px)   │  35.34 (USD 余额)      │
│                 │  Token: 17.7M          │
└──────────────────────────────────────────┘
```

屏幕分辨率 200×200，左右各 100px，横竖两条分隔线将画面分为四个区域。

---

## 硬件连接

| 墨水屏引脚 | ESP32 GPIO |
|-----------|-----------|
| BUSY      | 21        |
| RST       | 4         |
| DC        | 16        |
| CS        | 17        |
| SCK       | 18 (VSPI) |
| MOSI      | 19        |
| GND       | GND       |
| VCC       | 3.3V      |

> 使用微雪（Waveshare）1.54 寸黑白墨水屏，型号选择见下方 `MODEL` 宏说明。

---

## 快速开始

### 1. 安装依赖

使用 [Arduino IDE](https://www.arduino.cc/en/software) 或 [arduino-cli](https://arduino.github.io/arduino-cli/)，需安装：

- ESP32 开发板支持包（`esp32 by Espressif Systems`）
- 内置库：`WiFi`、`HTTPClient`、`WiFiClientSecure`（ESP32 核心自带，无需额外安装）

### 2. 配置密钥

复制 `secrets.h.example` 为 `secrets.h`，填入你自己的值：

```bash
cp secrets.h.example secrets.h
```

编辑 `secrets.h`：

```cpp
#define SECRET_WIFI_SSID        "你的WiFi名称"
#define SECRET_WIFI_PASSWORD    "你的WiFi密码"
#define SECRET_DEEPSEEK_API_KEY "sk-xxxxxxxxxxxx"         // DeepSeek 控制台获取
#define SECRET_BEIBEI_SESSION   "MTc4...你的session值"    // F12 → Network → Cookie
#define SECRET_BEIBEI_USER_ID   "610"                     // 贝贝AI 后台用户 ID
```

> `secrets.h` 已被 `.gitignore` 忽略，不会被提交到版本库，密钥安全。

### 3. 选择屏幕型号

在 `epd1in54_V2-demo.ino` 顶部修改 `MODEL` 宏：

```cpp
#define MODEL 0   // 0 = 1.54" V1（默认）
                  // 1 = 1.54" V2
```

### 4. 编译烧录

**Arduino IDE**：直接打开 `.ino` 文件，选择正确的开发板和端口，点击上传。

**命令行（arduino-cli）**：

```bash
# Linux / macOS
bash build_flash.sh

# Windows
build_flash.bat
```

---

## 文件说明

### 应用层（本项目核心）

| 文件 | 说明 |
|------|------|
| `epd1in54_V2-demo.ino` | **主程序**。`setup()` 完成初始化、WiFi、NTP 和首次绘制；`loop()` 每 2 分钟调用 `refreshBalance()` 刷新屏幕。`drawBalance()` 负责具体布局绘制。 |
| `WiFiTest.h` | WiFi 连接模块。`WiFiTest_connect()` 连接网络，`WiFiTest_probe()` 可做 DNS+TCP 连通性探测（调试用）。 |
| `DeepSeekBalance.h` | 查询 DeepSeek 官方账户余额。HTTPS GET `api.deepseek.com/user/balance`，Bearer Token 鉴权，手写 JSON 解析，优先取 CNY 余额。 |
| `BeibeiBalance.h` | 查询贝贝AI 中转站账户余额。HTTPS GET `api.beibeiai.top/api/user/self`，Cookie Session 鉴权，手写 JSON 解析，按 1 USD = 500,000 quota 换算。 |
| `secrets.h` | **本地密钥文件**（已 gitignore，不提交）。存放 WiFi 密码、DeepSeek API Key、贝贝AI Session。 |
| `secrets.h.example` | 密钥模板，可安全提交。复制为 `secrets.h` 后填入真实值。 |
| `logo_deepseek.h` | DeepSeek logo 位图数据（100×50 px，黑白 1-bit）。 |
| `beibei_logo.h` | 贝贝AI logo 位图数据（100×50 px，黑白 1-bit）。 |
| `build_flash.sh` | Linux/macOS 下用 arduino-cli 编译并烧录的脚本。 |
| `build_flash.bat` | Windows 下对应的编译烧录批处理脚本。 |
| `esp32_flash.bat` | Windows 下仅烧录（不重新编译）的批处理脚本。 |

### 驱动层（微雪官方 Waveshare）

| 文件 | 说明 |
|------|------|
| `DEV_Config.h / .cpp` | 硬件底层接口。定义 ESP32 引脚映射、SPI 读写、GPIO 操作和延时宏。 |
| `EPD.h` | 墨水屏驱动统一入口头文件，包含所有型号驱动。 |
| `EPD_1in54.h / .cpp` | 1.54" V1 型号驱动（全刷）。 |
| `EPD_1in54_V2.h / .cpp` | 1.54" V2 型号驱动（支持局刷）。 |
| `EPD_1in54b.h / .cpp` | 1.54" b 型（黑/白/红三色）驱动。 |
| `EPD_1in54c.h / .cpp` | 1.54" c 型（黑/白/黄三色）驱动。 |
| `GUI_Paint.h / .cpp` | 绘图 API。提供画点、线、矩形、圆、字符串、图片等接口，操作内存缓冲区，最后一次性推送到屏幕。 |
| `fonts.h` | 字库头文件，声明所有可用字体。 |
| `font8/12/16/20/24.cpp` | 英文位图字库（字号 8/12/16/20/24）。 |
| `font12CN.c / font24CN.c` | 中文位图字库（12/24 号）。 |
| `ImageData.h / .c` | 示例图片数据（微雪原始 demo 附带）。 |
| `Debug.h` | 调试宏（串口打印开关）。 |

---

## 注意事项

- **TLS 证书校验已关闭**：两个 HTTPS 模块均使用 `client.setInsecure()` 跳过证书校验，方便调试。在可信网络环境中使用问题不大，若需要更高安全性，可后续换成固定 CA 证书校验。
- **JSON 手写解析**：为避免引入 ArduinoJson 等外部库，采用 `indexOf` 字符串匹配解析 JSON，对 API 响应格式变化较敏感，如 API 更新后解析失败请检查字段名是否变更。
- **墨水屏刷新**：每次全刷耗时约 2 秒，屏幕会闪烁，这是墨水屏正常现象。

---

## 版本历史

| 版本 | 说明 |
|------|------|
| v1.0 | 第一个正式版本。显示 DeepSeek CNY 余额 + 贝贝AI USD 余额及剩余 Token，含 NTP 时间显示，每 2 分钟自动刷新。 |

---

## License

本项目应用层代码（`*.ino`、`WiFiTest.h`、`DeepSeekBalance.h`、`BeibeiBalance.h`）采用 MIT License。  
驱动层代码（`DEV_Config`、`EPD_*`、`GUI_Paint`、字库）版权归 [Waveshare](https://www.waveshare.com) 所有，遵循其原始许可证。
