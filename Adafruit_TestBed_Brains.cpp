/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef ARDUINO_ARCH_RP2040

#include "Adafruit_TestBed_Brains.h"
#include "pio_usb.h"
#include "Adafruit_TinyUSB.h"

#define USBHOST_RHPORT    1

Adafruit_TestBed_Brains Brain;

/**************************************************************************/
/*!
    @brief  Initializer, sets up the timestamp, neopixels, piezo, led,
            and analog reference. So get all pins assigned before calling
*/
/**************************************************************************/

Adafruit_TestBed_Brains::Adafruit_TestBed_Brains() {

  piezoPin = 15; // onboard buzzer
  ledPin = 25;   // green LED on Pico

  targetPowerPin = 6;  // VBat switch

  neopixelNum = 1;  // LCD backlight
  neopixelPin = 13; // LCD backlight

  _sd_detect_pin = 14; // SD detect
  _sd_cs_pin = 17; // SD chip select

  _usbh_dp_pin = 20; // USB Host D+
  _vbus_en_pin = 22; // USB Host VBus enable
}

void Adafruit_TestBed_Brains::begin(void) {
  // neopixel is already set up
  Adafruit_TestBed::begin();

  pinMode(_sd_detect_pin, INPUT_PULLUP);
  pinMode(_vbus_en_pin, OUTPUT);
  usbh_setVBus(false); // disabled by default

  analogReadResolution(12);

  pixels->setBrightness(255);
  setColor(0xFFFFFF);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.home();
  lcd.noCursor();

//  if (SD_detected()) {
//    Serial.print("SD inserted...");
//    if (!SD_begin()) {
//      Serial.println("Could not init SD!");
//    } else {
//      uint32_t SDsize = SD.card()->sectorCount();
//      if (SDsize == 0) {
//        Serial.println("Can't determine the card size");
//      } else {
//        Serial.printf("Card size = %0.1f GB\n", 0.000000512 * (float)SDsize);
//        Serial.println("Files found (date time size name):");
//        SD.ls(LS_R | LS_DATE | LS_SIZE);
//      }
//    }
//  }
}

bool Adafruit_TestBed_Brains::SD_detected(void) {
  return digitalRead(_sd_detect_pin);
}

bool Adafruit_TestBed_Brains::SD_begin(uint32_t max_clock) {
  return SD.begin(_sd_cs_pin, max_clock);
}

void Adafruit_TestBed_Brains::LCD_printf(bool linenum, const char format[], ...) {
  char linebuf[17];
  memset(linebuf, 0, sizeof(linebuf));

  va_list ap;
  va_start(ap, format);
  vsnprintf(linebuf, sizeof(linebuf), format, ap);

  // fill the rest with spaces
  memset(linebuf+strlen(linebuf), ' ', 16 - strlen(linebuf));
  linebuf[16] = 0;
  lcd.setCursor(0, linenum);
  lcd.write(linebuf);
  va_end(ap);

  Serial.print("LCD: "); Serial.println(linebuf);
}

void Adafruit_TestBed_Brains::LCD_info(const char *msg1, const char *msg2) {
  setColor(0xFFFFFF);
  LCD_printf(0, msg1);
  LCD_printf(1, msg2);
}

void Adafruit_TestBed_Brains::LCD_error(const char *errmsg1, const char *errmsg2) {
  setColor(0xFF0000);
  LCD_printf(0, errmsg1);
  LCD_printf(1, errmsg2);
  delay(250);
}

void Adafruit_TestBed_Brains::usbh_setVBus(bool en) {
  digitalWrite(_vbus_en_pin, en ? HIGH : LOW);
}

bool Adafruit_TestBed_Brains::usbh_begin(void) {
  // Check for CPU frequency, must be multiple of 120Mhz for bit-banging USB
  uint32_t cpu_hz = clock_get_hz(clk_sys);
  if ( cpu_hz != 120000000UL && cpu_hz != 240000000UL ) {
    while ( !Serial ) delay(10);   // wait for native usb
    Serial.printf("Error: CPU Clock = %u, PIO USB require CPU clock must be multiple of 120 Mhz\r\n", cpu_hz);
    Serial.printf("Change your CPU Clock to either 120 or 240 Mhz in Menu->CPU Speed \r\n", cpu_hz);
    while(1) delay(1);
  }

  // enable vbus
  pinMode(_vbus_en_pin, OUTPUT);
  usbh_setVBus(true);

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = (uint8_t) _usbh_dp_pin;

  USBHost.configure_pio_usb(USBHOST_RHPORT, &pio_cfg);
  if (!USBHost.begin(USBHOST_RHPORT))  {
    Serial.println("usb host begin failed");
    usbh_setVBus(false);
    return false;
  }

  return true;
}

#endif
