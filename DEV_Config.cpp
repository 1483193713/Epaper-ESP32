/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2020-02-19
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "DEV_Config.h"
#include <SPI.h>

/*
 * SPI mode selection:
 *   USE_HARDWARE_SPI = 1  -> fast, uses ESP32 hardware SPI (VSPI)
 *   USE_HARDWARE_SPI = 0  -> compatible, uses bit-bang soft SPI (works with any pins)
 */
#define USE_HARDWARE_SPI 0

static SPIClass *spi = NULL;
static SPISettings spiSettings;

void GPIO_Config(void)
{
    pinMode(EPD_BUSY_PIN,  INPUT);
    pinMode(EPD_RST_PIN , OUTPUT);
    pinMode(EPD_DC_PIN  , OUTPUT);
    pinMode(EPD_CS_PIN  , OUTPUT);

    // Match STM32 reference: CS=0, DC=0, RST=1 as safe initial state
    digitalWrite(EPD_CS_PIN , LOW);
    digitalWrite(EPD_DC_PIN , LOW);
    digitalWrite(EPD_RST_PIN, HIGH);

#if USE_HARDWARE_SPI
    // Hardware SPI: let the SPI peripheral control SCK and MOSI
    spi = new SPIClass(VSPI);
    spi->begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, -1);  // SCK, MISO(unused), MOSI, CS(manual)
    spiSettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);  // 1MHz — match STM32 speed
#else
    // Software (bit-bang) SPI
    pinMode(EPD_SCK_PIN,  OUTPUT);
    pinMode(EPD_MOSI_PIN, OUTPUT);
    digitalWrite(EPD_SCK_PIN, LOW);
#endif
}

void GPIO_Mode(UWORD GPIO_Pin, UWORD Mode)
{
    if(Mode == 0) {
        pinMode(GPIO_Pin , INPUT);
    } else {
        pinMode(GPIO_Pin , OUTPUT);
    }
}

/******************************************************************************
function:	Module Initialize, the BCM2835 library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
UBYTE DEV_Module_Init(void)
{
    // Serial.begin() is called in setup() before this function — don't reinitialize here.

    // --- Verify pin configuration (remove after confirming) ---
    Serial.println("========== EPD Pin Config ==========");
    Serial.printf("BUSY: %d  RST: %d  DC: %d  CS: %d\n", EPD_BUSY_PIN, EPD_RST_PIN, EPD_DC_PIN, EPD_CS_PIN);
    Serial.printf("SCK:  %d  MOSI:%d\n", EPD_SCK_PIN, EPD_MOSI_PIN);
#if USE_HARDWARE_SPI
    Serial.println("SPI Mode: HARDWARE (VSPI)");
#else
    Serial.println("SPI Mode: SOFTWARE (bit-bang)");
#endif
    Serial.println("====================================");
    // --- End verify ---

    GPIO_Config();

    // Match STM32 DEV_Module_Init: ensure safe initial pin states
    DEV_Digital_Write(EPD_DC_PIN, 0);
    DEV_Digital_Write(EPD_CS_PIN, 0);
    DEV_Digital_Write(EPD_RST_PIN, 1);

    return 0;
}

/******************************************************************************
function: SPI read and write
******************************************************************************/
void DEV_SPI_WriteByte(UBYTE data)
{
    // CS is controlled at the EPD driver layer (SendCommand/SendData),
    // matching the STM32 reference code behavior.
#if USE_HARDWARE_SPI
    spi->beginTransaction(spiSettings);
    spi->transfer(data);
    spi->endTransaction();
#else
    for (int i = 0; i < 8; i++) {
        if (data & 0x80) digitalWrite(EPD_MOSI_PIN, GPIO_PIN_SET);
        else             digitalWrite(EPD_MOSI_PIN, GPIO_PIN_RESET);
        data <<= 1;
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_SET);
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_RESET);
    }
#endif
}

UBYTE DEV_SPI_ReadByte()
{
    UBYTE j = 0xFF;
#if USE_HARDWARE_SPI
    spi->beginTransaction(spiSettings);
    digitalWrite(EPD_CS_PIN, LOW);
    j = spi->transfer(0xFF);
    digitalWrite(EPD_CS_PIN, HIGH);
    spi->endTransaction();
#else
    GPIO_Mode(EPD_MOSI_PIN, 0);
    digitalWrite(EPD_CS_PIN, GPIO_PIN_RESET);
    for (int i = 0; i < 8; i++) {
        j = j << 1;
        if (digitalRead(EPD_MOSI_PIN))  j = j | 0x01;
        else                            j = j & 0xFE;
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_SET);
        digitalWrite(EPD_SCK_PIN, GPIO_PIN_RESET);
    }
    digitalWrite(EPD_CS_PIN, GPIO_PIN_SET);
    GPIO_Mode(EPD_MOSI_PIN, 1);
#endif
    return j;
}

void DEV_SPI_Write_nByte(UBYTE *pData, UDOUBLE len)
{
    // CS is controlled at the EPD driver layer (SendCommand/SendData).
#if USE_HARDWARE_SPI
    spi->beginTransaction(spiSettings);
    spi->transfer(pData, len);
    spi->endTransaction();
#else
    for (int i = 0; i < len; i++)
        DEV_SPI_WriteByte(pData[i]);
#endif
}
