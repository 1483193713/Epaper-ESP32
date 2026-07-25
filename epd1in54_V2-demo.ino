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
#include <stdlib.h>
#include <string.h>

/* 余额刷新间隔 (毫秒) */
#define REFRESH_INTERVAL_MS 60000UL

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
  Paint_DrawString_EN(6, 6, "DeepSeek", &Font20, BLACK, WHITE);
  Paint_DrawLine(4, 30, EPD_WIDTH - 5, 30, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  if (!b.httpOk) {
    // 查询失败：显示错误码
    Paint_DrawString_EN(6, 45, "Query FAIL", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(6, 68, "HTTP:", &Font16, BLACK, WHITE);
    Paint_DrawNum(70, 68, (int32_t)b.httpCode, &Font16, BLACK, WHITE);
    EPD_FUNC_DISPLAY(BlackImage);
    return;
  }

  // 货币 + 总余额
  Paint_DrawString_EN(6, 40, "Balance", &Font16, BLACK, WHITE);
  String line1 = b.currency + " " + b.totalBalance;
  Paint_DrawString_EN(6, 62, line1.c_str(), &Font20, BLACK, WHITE);

  // 赠金 / 充值
  String line2 = "grant " + b.grantedBalance;
  String line3 = "top   " + b.toppedUpBalance;
  Paint_DrawString_EN(6, 92,  line2.c_str(), &Font16, BLACK, WHITE);
  Paint_DrawString_EN(6, 114, line3.c_str(), &Font16, BLACK, WHITE);

  // 可用状态
  Paint_DrawString_EN(6, 142, b.isAvailable ? "[OK]" : "[UNAVAILABLE]",
                      &Font16, BLACK, WHITE);

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

  // 3. 分配图像缓冲区 (只分配一次, 反复复用)
  UWORD Imagesize = ((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1)) * EPD_HEIGHT;
  BlackImage = (UBYTE *)malloc(Imagesize);
  Paint_NewImage(BlackImage, EPD_WIDTH, EPD_HEIGHT, 270, WHITE);

  // 4. 开机查一次余额并显示
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
