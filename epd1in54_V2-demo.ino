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
#include <stdlib.h>
#include <string.h>

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

  // 2. Create buffer
  UBYTE *BlackImage;
  UWORD Imagesize = ((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1)) * EPD_HEIGHT;
  BlackImage = (UBYTE *)malloc(Imagesize);

  // 3. Draw
  Paint_NewImage(BlackImage, EPD_WIDTH, EPD_HEIGHT, 270, WHITE);
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);

  // 边框 + 对角线
  Paint_DrawRectangle(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawLine(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(EPD_WIDTH - 1, 0, 0, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  // A ~ G  等间隔（Font20 宽 14px, 7 字符等分 200px）
  Paint_DrawString_EN(10, 50, "A  B  C  D  E  F  G", &Font20, BLACK, WHITE);
  // 1 ~ 9
  Paint_DrawString_EN(10, 80, "1 2 3 4 5 6 7 8 9", &Font20, BLACK, WHITE);

  EPD_FUNC_DISPLAY(BlackImage);
  DEV_Delay_ms(5000);

  // 4. Sleep
  EPD_FUNC_SLEEP();
  free(BlackImage);
  Serial.println("DONE");
}

void loop() {}
