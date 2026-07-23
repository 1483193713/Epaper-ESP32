/*
 * ESP32 + Waveshare 1.54" e-Paper V1
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
#elif MODEL == 2
  #define EPD_FUNC_INIT()       EPD_1IN54B_Init()
  #define EPD_FUNC_CLEAR()      EPD_1IN54B_Clear()
  #define EPD_FUNC_DISPLAY(img) EPD_1IN54B_Display(img)
  #define EPD_FUNC_SLEEP()      EPD_1IN54B_Sleep()
  #define EPD_WIDTH             EPD_1IN54B_WIDTH
  #define EPD_HEIGHT            EPD_1IN54B_HEIGHT
#elif MODEL == 3
  #define EPD_FUNC_INIT()       EPD_1IN54C_Init()
  #define EPD_FUNC_CLEAR()      EPD_1IN54C_Clear()
  #define EPD_FUNC_DISPLAY(img) EPD_1IN54C_Display(img)
  #define EPD_FUNC_SLEEP()      EPD_1IN54C_Sleep()
  #define EPD_WIDTH             EPD_1IN54C_WIDTH
  #define EPD_HEIGHT            EPD_1IN54C_HEIGHT
#endif

void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("===== 1.54 e-Paper MODEL=" + String(MODEL) + " =====");
  DEV_Module_Init();

  // 1. Init & Clear
  Serial.println("1. Init & Clear...");
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

  UWORD WidthByte = EPD_WIDTH / 8;

  // === Test 1: 左黑右白 (已验证正确) ===
  Serial.println("2. Test 1: Left BLACK, Right WHITE...");
  for (UWORD y = 0; y < EPD_HEIGHT; y++) {
    for (UWORD x = 0; x < WidthByte; x++) {
      BlackImage[y * WidthByte + x] = (x < WidthByte/2) ? 0x00 : 0xFF;
    }
  }
  EPD_FUNC_DISPLAY(BlackImage);
  DEV_Delay_ms(3000);

  // === Test 2: Paint rotate=270 (已验证能显示) ===
  Serial.println("3. Test 2: Paint rotate=270...");
  Paint_NewImage(BlackImage, EPD_WIDTH, EPD_HEIGHT, 270, WHITE);
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawRectangle(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  Paint_DrawLine(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawLine(EPD_WIDTH - 1, 0, 0, EPD_HEIGHT - 1, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  Paint_DrawString_EN(20, 70, "ESP32 OK!", &Font20, BLACK, WHITE);
  Paint_DrawString_EN(20, 95, "V1", &Font16, BLACK, WHITE);
  EPD_FUNC_DISPLAY(BlackImage);
  Serial.println("   -> displayed. Wait 5s...");
  DEV_Delay_ms(5000);

  // 4. Sleep
  Serial.println("4. Sleep...");
  EPD_FUNC_SLEEP();
  free(BlackImage);
  Serial.println("===== DONE =====");
}

void loop()
{
}
