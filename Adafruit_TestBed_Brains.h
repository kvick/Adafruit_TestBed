#ifndef ADAFRUIT_TESTBED_BRAINS_H
#define ADAFRUIT_TESTBED_BRAINS_H

#include "Adafruit_TestBed.h"
#include <SdFat.h>
#include <LiquidCrystal.h>

/**************************************************************************/
/*!
    @brief A helper class for making RP2040 "Tester Brains"
*/
/**************************************************************************/
class Adafruit_TestBed_Brains: public Adafruit_TestBed {
public:
  Adafruit_TestBed_Brains(void);
  void begin(void);

  LiquidCrystal lcd = LiquidCrystal(7, 8, 9, 10, 11, 12);
  void LCD_printf(bool linenum, const char format[], ...);
  void LCD_error(const char *errmsg1, const char *errmsg2);
  void LCD_info(const char *msg1, const char *msg2);

  bool SD_detected(void);

  SdFs SD;
  SdSpiConfig SD_CONFIG = SdSpiConfig(17, SHARED_SPI, SD_SCK_MHZ(16));

private:
};

#endif
