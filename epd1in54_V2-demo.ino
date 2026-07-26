/*
 * ESP32 + Waveshare 1.54" e-Paper V1
 * 开机连 WiFi -> 查询 DeepSeek 账户余额 -> 显示到墨水屏
 * 每 1 分钟刷新一次余额
 * BUSY->21  RST->4  DC->16  CS->17  SCK->18  MOSI->19
 */
#define MODEL 0

#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include "WiFiTest.h"
#include "DeepSeekBalance.h"
#include "logo_deepseek.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 余额刷新间隔 (毫秒) */
#define REFRESH_INTERVAL_MS 120000UL

/* NTP 时区 (秒) — UTC+8 */
#define TZ_OFFSET_SEC (8 * 3600)

#if MODEL == 0
  #define EPD_FUNC_INIT()       EPD_1IN54_Init(EPD_1IN54_FULL)
  #define EPD_FUNC_CLEAR()      EPD_1IN54_Clear()
  #define EPD_FUNC_DISPLAY(img) EPD_1IN54_Display(img)
  #define EPD_FUNC_SLEEP()      EPD_1IN54_Sleep()
  #define EPD_WIDTH             EPD_1IN54_WIDTH
  #define EPD_HEIGHT            EPD_1IN54_HEIGHT
#elif MODEL == 1
  #define EPD_FUNC_INIT()       EPD_1IN54_V2_Init()
  #define EPD_FUNC_CLEAR()      EPD_1IN54_V2_Clear()
  #define EPD_FUNC_DISPLAY(img) EPD_1IN54_V2_Display(img)
  #define EPD_FUNC_SLEEP()      EPD_1IN54_V2_Sleep()
  #define EPD_WIDTH             EPD_1IN54_V2_WIDTH
  #define EPD_HEIGHT            EPD_1IN54_V2_HEIGHT
#endif

/* 复用的图像缓冲区 */
static UBYTE *BlackImage = NULL;

/* 把余额信息画到墨水屏 (白底黑字) --------------------------------------*/
static void drawBalance(const DSBalance &b)
{
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);

  Paint_DrawRectangle(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  // 左上角画 DeepSeek logo (100x100, X 需为 8 的倍数)
  
  Paint_DrawImage(gImage_deepseek, 0, 100, LOGO_DS_WIDTH, LOGO_DS_HEIGHT);
  Paint_DrawLine(4, 100, EPD_WIDTH - 5, 100, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);
  Paint_DrawLine(100, 4, 100, 100, BLACK, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

  if (!b.httpOk) {
    // 查询失败：显示错误码
    Paint_DrawString_EN(6, 122, "Query FAIL", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(6, 145, "HTTP:", &Font16, BLACK, WHITE);
    Paint_DrawNum(70, 145, (int32_t)b.httpCode, &Font16, BLACK, WHITE);
    EPD_FUNC_DISPLAY(BlackImage);
    return;
  }

  // 横线下方分开显示 币种 和 总余额
  Paint_DrawString_EN(110, 5, b.currency.c_str(), &Font20, BLACK, WHITE);
  Paint_DrawString_EN(110, 30, b.totalBalance.c_str(), &Font24, BLACK, WHITE);

  // 当前时间 (Font8, hh:mm:ss)
  {
    struct tm ti;
    char tbuf[16];
    if (getLocalTime(&ti, 100)) {
      snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", ti.tm_hour, ti.tm_min, ti.tm_sec);
      Paint_DrawString_EN(130, 86, tbuf, &Font12, BLACK, WHITE);
    }
  }

  EPD_FUNC_DISPLAY(BlackImage);
}

/* 查询余额并刷新屏幕 ----------------------------------------------------*/
static void refreshBalance()
{
  DSBalance b = DeepSeek_getBalance();
  drawBalance(b);
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  DEV_Module_Init();

  // 1. Init & Clear
  EPD_FUNC_INIT();
  EPD_FUNC_CLEAR();
  DEV_Delay_ms(500);

  // 2. 连 WiFi (串口有日志)
  WiFiTest_connect();

  // 2.5 同步网络时间 (NTP)
  configTime(TZ_OFFSET_SEC, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("[NTP] Syncing time...");

  // 3. 分配图像缓冲区 (只分配一次, 反复复用)
  UWORD Imagesize = ((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1)) * EPD_HEIGHT;
  BlackImage = (UBYTE *)malloc(Imagesize);
  Paint_NewImage(BlackImage, EPD_WIDTH, EPD_HEIGHT, 270, WHITE);

  // 4. 查一次余额并显示 (界面右上角含 DeepSeek logo)
  refreshBalance();

  Serial.println("DONE");
}

void loop()
{
  // 每 REFRESH_INTERVAL_MS 刷新一次余额
  static uint32_t last = 0;
  if (last == 0 || millis() - last > REFRESH_INTERVAL_MS) {
    last = millis();
    refreshBalance();
  }
}
