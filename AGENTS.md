# ESP32 墨水屏余额监控器 — AGENTS.md

ESP32 + 微雪 1.54" 墨水屏，定时查询 DeepSeek & 贝贝AI 账户余额并显示。Arduino sketch，arduino-cli 构建。

## Project
- **栈**: ESP32 Arduino (C++), Waveshare e-Paper SPI 驱动, HTTPS 手写 JSON 解析
- **入口**: `epd1in54_V2-demo.ino` — `setup()` 初始化→WiFi→NTP→首次绘制，`loop()` 每 120s 刷新
- **屏幕**: 200×200 黑白，MODEL 0=V1 全刷, MODEL 1=V2 局刷
- **引脚**: BUSY=21 RST=4 DC=16 CS=17 SCK=18 MOSI=19 (SPI)
- **密钥**: `secrets.h` (gitignore)，模板见 `secrets.h.example`
- **远程**: `https://github.com/1483193713/Epaper-ESP32.git`

## Commands

```bash
# 编译+烧录（推荐，当前目录运行）
.\build_flash.bat            # 默认 COM6, all

# 通用脚本（指定目录和端口）
.\esp32_flash.bat . COM6           # all (编译+烧录)
.\esp32_flash.bat . COM6 compile   # 仅编译
.\esp32_flash.bat . COM6 flash     # 仅烧录（需先编译过）

# Linux/Mac
bash build_flash.sh
```

FQBN: `esp32:esp32:esp32`，构建输出在 `.build/`。

## Architecture

```
epd1in54_V2-demo.ino      — 主程序: setup/loop/drawBalance/refreshBalance
├── WiFiTest.h             — WiFi 连接 (WiFiTest_connect)
├── DeepSeekBalance.h      — GET api.deepseek.com/user/balance, Bearer Token, 手写 JSON
├── BeibeiBalance.h        — GET api.beibeiai.top/api/user/self, Cookie Session, 手写 JSON
│                            quota 换算: 1 USD = 500,000, 显示时 /1e6 → "X.XM"
├── DEV_Config.h/.cpp      — SPI/GPIO 底层 (微雪驱动)
├── EPD_1in54*.cpp/.h      — 墨水屏驱动 (V1/V2/b/c)
├── GUI_Paint.h/.cpp       — 绘图 API: 点/线/矩形/字/图片, Bresenham 算法
├── logo_deepseek.h        — DeepSeek logo 100×100 位图
├── beibei_logo.h          — 贝贝AI logo 100×100 位图
├── font8-24.cpp/font*CN.c — 英/中位图字库 (Font8/12/16/20/24)
└── secrets.h              — WiFi SSID/PWD, API Key, Session (本地,不提交)
```

## Conventions

- **不用外部 JSON 库** — 全部 `String::indexOf` 手写解析，API 响应格式变更需同步改解析逻辑
- **TLS 校验已跳过** — `client.setInsecure()`，调试用，生产环境建议固定 CA
- **字体**: 英文 `Paint_DrawString_EN(X,Y,str,&FontXX,COLOR,BG)`，中文 `Paint_DrawString_CN`，等宽点阵
- **位图**: `Paint_DrawImage(data, X, Y, W, H)`，X 建议取 8 的倍数；取模方式=横向+高位在前+阴码
- **坐标**: 左上角原点 (0,0)，200×200 画布，上半 0-99 下半 100-199
- **刷新粒度**: `REFRESH_INTERVAL_MS` 默认 120000 (2分钟)，`millis()` 计时
- **内存**: `BlackImage` 缓冲区一次分配复用，`malloc` + `Paint_NewImage`，不释放
- **`.gitignore`**: 忽略 `secrets.h` 和 `.build/`

## Notes
- 首次开机 NTP 同步需数秒，首帧可能不显示时间，下次刷新自动补上
- 墨水屏全刷约 2 秒，屏幕会闪烁，正常现象
- beibei logo 在左下 (0,100)，deepseek logo 在左上 (0,0) → 后来被替换为左上/左下各自 100×100
