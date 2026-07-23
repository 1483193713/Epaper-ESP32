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
  Serial.println("===== E-Paper A-G & 1-9 =====");
  DEV_Module_Init();

  // 1. Init & Clear
  EPD_FUNC_INIT();
  EPD_FUNC_CLEAR();
  DEV_Delay_ms(500);

  // 2. Create buffer
  UBYTE *BlackImage;
  UWORD Imagesize = ((EPD_WIDTH % 8 == 0) ? (EPD_WIDTH / 8) : (EPD_WIDTH / 8 + 1)) * EPD_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Serial.println("ERROR: malloc failed!");
    while (1);
  }

  // 3. Draw
  Paint_NewImage(BlackImage, EPD_WIDTH, EPD_HEIGHT, 270, WHITE);
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);

  int screenW = EPD_WIDTH;   // 200
  int cw = Font20.Width;     // 14px per char
  int ch = Font20.Height;    // 20px

  // ===== 第一行: A ~ G (Font20) =====
  int n1 = 7;
  int gap1 = (screenW - n1 * cw) / (n1 + 1);  // 等间隔 = 12px
  // 微调: 把多余像素分到两边
  int margin1 = (screenW - (n1 * cw + (n1 + 1) * gap1)) / 2;

  const char letters[] = "ABCDEFG";
  for (int i = 0; i < n1; i++) {
    int x = margin1 + gap1 + i * (cw + gap1);
    char str[2] = {letters[i], '\0'};
    Paint_DrawString_EN(x, 30, str, &Font20, BLACK, WHITE);
  }

  // 分割线
  Paint_DrawLine(10, 65, screenW - 10, 65, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  // ===== 第二行: 1 ~ 9 (Font20) =====
  int n2 = 9;
  int gap2 = (screenW - n2 * cw) / (n2 + 1);  // 等间隔 = 7px
  int margin2 = (screenW - (n2 * cw + (n2 + 1) * gap2)) / 2;

  for (int i = 0; i < n2; i++) {
    int x = margin2 + gap2 + i * (cw + gap2);
    Paint_DrawNum(x, 85, i + 1, &Font20, BLACK, WHITE);
  }

  // 分割线
  Paint_DrawLine(10, 120, screenW - 10, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);

  // ===== 第三行: 标签 =====
  Paint_DrawString_EN(10, 140, "Letters", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(10, 160, "Digits", &Font16, BLACK, WHITE);
  Paint_DrawNum(70, 160, 123456789, &Font16, BLACK, WHITE);

  EPD_FUNC_DISPLAY(BlackImage);
  Serial.println("   -> displayed.");
  DEV_Delay_ms(5000);

  // 4. Sleep
  EPD_FUNC_SLEEP();
  free(BlackImage);
  Serial.println("===== DONE =====");
}

void loop()
{
}
