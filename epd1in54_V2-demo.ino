/*
 * ESP32 + Waveshare 1.54" e-Paper V1
 * 显示 A-G 和 1-9，等间隔排列
 * BUSY->21  RST->4  DC->16  CS->17  SCK->18  MOSI->19
 */
#define MODEL 0

#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include "WiFiTest.h"
#include <stdlib.h>
#include <string.h>

/* 被测目标 */
#define TEST_HOST "www.deepseek.com"
#define TEST_PORT 443

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

void setup()
{
  Serial.begin(115200);
  delay(100);
  DEV_Module_Init();

  // 1. Init & Clear (内部自动处理首次刷新问题)
  EPD_FUNC_INIT();
  EPD_FUNC_CLEAR();
  DEV_Delay_ms(500);

  // 2. 运行网络测试 (串口全程有日志)
  WiFiTestResult r = WiFiTest_run(TEST_HOST, TEST_PORT);

  // 3. Create buffer
  UBYTE *BlackImage;
  UWORD Imagesize = ((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1)) * EPD_HEIGHT;
  BlackImage = (UBYTE *)malloc(Imagesize);

  // 4. Draw：把测试结果画到墨水屏 (白底黑字，本面板极性正常)
  Paint_NewImage(BlackImage, EPD_WIDTH, EPD_HEIGHT, 270, WHITE);
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);

  Paint_DrawRectangle(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawString_EN(6, 6, "Net Test", &Font20, BLACK, WHITE);
  Paint_DrawLine(4, 30, EPD_WIDTH - 5, 30, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  // WiFi 状态
  Paint_DrawString_EN(6, 40, r.wifiConnected ? "WiFi: OK" : "WiFi: FAIL",
                      &Font16, BLACK, WHITE);
  // 本机 IP
  Paint_DrawString_EN(6, 60, r.localIP.c_str(), &Font16, BLACK, WHITE);
  // DNS 解析结果
  Paint_DrawString_EN(6, 80, r.dnsOK ? "DNS: OK" : "DNS: FAIL",
                      &Font16, BLACK, WHITE);
  Paint_DrawString_EN(6, 100, r.hostIP.c_str(), &Font16, BLACK, WHITE);
  // TCP 连通性 + 耗时
  if (r.tcpOK) {
    Paint_DrawString_EN(6, 120, "Net: OK", &Font16, BLACK, WHITE);
    Paint_DrawNum(90, 120, (int32_t)r.tcpMs, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(140, 120, "ms", &Font16, BLACK, WHITE);
  } else {
    Paint_DrawString_EN(6, 120, "Net: FAIL", &Font16, BLACK, WHITE);
  }

  EPD_FUNC_DISPLAY(BlackImage);
  DEV_Delay_ms(2000);

  // 5. Sleep（保持 WiFi 连着，方便 loop 里复测）
  EPD_FUNC_SLEEP();
  free(BlackImage);
  Serial.println("DONE");
}

void loop()
{
  // 每 30s 复测一次连通性，仅打印到串口（不再刷新墨水屏以省电/护屏）
  static uint32_t last = 0;
  if (millis() - last > 30000) {
    last = millis();
    WiFiTest_pingOnce(TEST_HOST, TEST_PORT);
  }
}
