#include "Adafruit_TestBed.h"

Adafruit_TestBed::Adafruit_TestBed(void) {
#if defined(ADAFRUIT_METRO_M0_EXPRESS)
  neopixelPin = 40;
  neopixelNum = 1;
#endif
}

/**************************************************************************/
/*!
    @brief  Initializer, sets up the timestamp, neopixels, piezo, led,
            and analog reference. So get all pins assigned before calling
*/
/**************************************************************************/
void Adafruit_TestBed::begin(void) {
  millis_timestamp = millis();

  targetPower(0);
  if (neopixelNum > 0) {
    pixels =
        new Adafruit_NeoPixel(neopixelNum, neopixelPin, NEO_GRB + NEO_KHZ800);
    pixels->begin();
    pixels->show(); // Initialize all pixels to 'off'
    pixels->setBrightness(20);
  }

  if (piezoPin >= 0) {
    pinMode(piezoPin, OUTPUT);
  }
  if (ledPin >= 0) {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
  }

#if defined(__AVR__)
  analogRef = 5.0;
#elif defined(ARDUINO_ARCH_RP2040)
  analogBits = 4096;
#elif defined(ARDUINO_ARCH_ESP32)
  analogBits = 4096;
  analogReadResolution(12);
  analogRef = 2.4;
#endif
}

/**************************************************************************/
/*!
    @brief  Sets a timestamp and returns time since last call
    @return Time elapsed since last time this function called, in milliseconds
*/
/**************************************************************************/
uint32_t Adafruit_TestBed::timestamp(void) {
  uint32_t delta = millis() - millis_timestamp;
  millis_timestamp = millis();
  return delta;
}

/**************************************************************************/
/*!
    @brief  Pretty printer for timestamp
    @param    restamp Pass in true to reset the timestamp on call.
*/
/**************************************************************************/
void Adafruit_TestBed::printTimeTaken(bool restamp) {
  Serial.print(F("Took: "));
  Serial.print(millis() - millis_timestamp);
  Serial.println(F(" ms"));
  if (restamp) {
    timestamp();
  }
}

/**************************************************************************/
/*!
    @brief  Sets a pin to be a low output, then set input to see if it rises
    @param    pin The GPIO to use
    @param    inter_delay How long to wait in ms for the pin to rise
    @return True if there was a pullup detected on this pin
*/
/**************************************************************************/
bool Adafruit_TestBed::testPullup(uint16_t pin, uint8_t inter_delay) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(inter_delay);
  pinMode(pin, INPUT);
  delay(inter_delay);
  return (digitalRead(pin));
}

/**************************************************************************/
/*!
    @brief  Perform a I2C scan for a given address on the bus
    @param    addr The address to look for
    @param    post_delay How many ms to wait after a scan
    @return True if the address was found
*/
/**************************************************************************/
bool Adafruit_TestBed::scanI2CBus(byte addr, uint8_t post_delay) {
  theWire->begin();
  theWire->beginTransmission(addr);
  bool found = (theWire->endTransmission() == 0);
  delay(post_delay);
  return found;
}

/**************************************************************************/
/*!
    @brief  Perform a I2C scan for 0x00-0x7F and print results
*/
/**************************************************************************/
void Adafruit_TestBed::printI2CBusScan(void) {
  theWire->begin();
  Serial.print("I2C scan: ");
  for (uint8_t addr = 0x00; addr <= 0x7F; addr++) {
    theWire->beginTransmission(addr);
    if (theWire->endTransmission() == 0) {
      Serial.print("0x");
      Serial.print(addr, HEX);
      Serial.print(", ");
    }
  }
  Serial.println();
}

/**************************************************************************/
/*!
    @brief  Turn on the target by setting the power pin active
    @param  on What value to set the pin to be powered
*/
/**************************************************************************/
void Adafruit_TestBed::targetPower(bool on) {
  if (targetPowerPin < 0)
    return;

  pinMode(targetPowerPin, OUTPUT);
  digitalWrite(targetPowerPin, on ? targetPowerPolarity : !targetPowerPolarity);
}

/**************************************************************************/
/*!
    @brief  Cycle the target power pin
    @param  off_time How many ms to wait between off and back on
*/
/**************************************************************************/
void Adafruit_TestBed::targetPowerCycle(uint16_t off_time) {
  targetPower(0);
  delay(off_time);
  targetPower(1);
}

/**************************************************************************/
/*!
    @brief  Read the ADC on a pin and convert it to a voltage
    @param  pin The ADC pin to read on
    @param  multiplier If there's a resistor divider, put the inverse here
    @return Voltage as a floating point
*/
/**************************************************************************/
float Adafruit_TestBed::readAnalogVoltage(uint16_t pin, float multiplier) {
  float x = analogRead(pin);
  Serial.println(x);
  x /= analogBits;
  x *= analogRef;
  x *= multiplier;
  return x;
}

/**************************************************************************/
/*!
    @brief  Perform a test of a pin's analog voltage, within 10%
    @param  pin The ADC pin to read on
    @param  name Human readable name for the pin
    @param  multiplier If there's a resistor divider, put the inverse here
    @param  value What voltage the pin should be
    @return True if the pin voltage is within 10% of target
*/
/**************************************************************************/
bool Adafruit_TestBed::testAnalogVoltage(uint16_t pin, const char *name,
                                         float multiplier, float value) {
  float voltage = readAnalogVoltage(pin, multiplier);
  Serial.print(name);
  Serial.print(F(" output voltage: "));
  Serial.print(voltage);
  Serial.print(F(" V (should be "));
  Serial.print(value);
  Serial.print(" V)...");
  if (abs(voltage - value) > (value / 10.0)) {
    Serial.println("Failed");
    return false;
  }
  Serial.println(F("OK within 10%"));

  return true;
}

/**************************************************************************/
/*!
    @brief  Perform a test of two connected GPIO pins
    @param  a The first pin
    @param  b The second pin
    @param  allpins An array of uint8_t's of all pins that are tested against a
   and b
    @param  num_allpins The length of allpins array
    @return True if the pins are shorted together but not to any other pins
*/
/**************************************************************************/
bool Adafruit_TestBed::testpins(uint8_t a, uint8_t b, uint8_t *allpins,
                                uint8_t num_allpins) {

  Serial.print(F("\tTesting "));
  Serial.print(a, DEC);
  Serial.print(F(" & "));
  Serial.println(b, DEC);

  // set both to inputs
  pinMode(b, INPUT);
  // turn on 'a' pullup
  pinMode(a, INPUT_PULLUP);
  delay(1);

  // verify neither are grounded
  if (!digitalRead(a) || !digitalRead(b)) {
    Serial.println(F("Ground test 1 fail: both pins should not be grounded"));
    // while (1) yield();
    return false;
  }

  // turn off both pullups
  pinMode(a, INPUT);
  pinMode(b, INPUT);

  // make a an output
  pinMode(a, OUTPUT);
  digitalWrite(a, LOW);
  delay(1);

  int ar = digitalRead(a);
  int br = digitalRead(b);

  // make sure both are low
  if (ar || br) {
    Serial.print(F("Low test fail on "));
    if (br)
      Serial.println(b, DEC);
    if (ar)
      Serial.println(a, DEC);
    return false;
  }

  // a is an input, b is an output
  pinMode(a, INPUT);
  pinMode(b, OUTPUT);
  digitalWrite(b, HIGH);
  delay(10);

  // verify neither are grounded
  if (!digitalRead(a)
#if !defined(ESP32)
      || !digitalRead(b)
#endif
  ) {
    Serial.println(F("Ground test 2 fail: both pins should not be grounded"));
    delay(100);
    return false;
  }

  // make sure no pins are shorted to pin a or b
  for (uint8_t i = 0; i < num_allpins; i++) {
    pinMode(allpins[i], INPUT_PULLUP);
  }

  pinMode(a, OUTPUT);
  digitalWrite(a, LOW);
  pinMode(b, OUTPUT);
  digitalWrite(b, LOW);
  delay(1);

  for (uint8_t i = 0; i < sizeof(allpins); i++) {
    if ((allpins[i] == a) || (allpins[i] == b)) {
      continue;
    }

    // Serial.print("Pin #"); Serial.print(allpins[i]);
    // Serial.print(" -> ");
    // Serial.println(digitalRead(allpins[i]));
    if (!digitalRead(allpins[i])) {
      Serial.print(allpins[i]);
      Serial.println(F(" is shorted?"));

      return false;
    }
  }
  pinMode(a, INPUT);
  pinMode(b, INPUT);

  delay(10);

  return true;
}

/**************************************************************************/
/*!
    @brief  Set the NeoPixels to a set color and show
    @param  color The 0xRRGGBB color to set to
*/
/**************************************************************************/
void Adafruit_TestBed::setColor(uint32_t color) {
  if (!pixels)
    return;

  pixels->fill(color);
  pixels->show();
}

/**************************************************************************/
/*!
    @brief   Input a value 0 to 255 to get a color value. The colours are a
   transition r - g - b - back to r.
    @param  WheelPos The position in the wheel, from 0 to 255
    @returns  The 0xRRGGBB color
*/
/**************************************************************************/
uint32_t Adafruit_TestBed::Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return Adafruit_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return Adafruit_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return Adafruit_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/**************************************************************************/
/*!
    @brief   Turn off I2C peripheral if possible (not supported on some chips)
*/
/**************************************************************************/
void Adafruit_TestBed::disableI2C(void) {
#if defined(__AVR__)
  TWCR = 0;
#elif defined(ESP32) || defined(ESP8266)
  // nothing!
#else
  theWire->end();
#endif
}

/**************************************************************************/
/*!
    @brief  Perform a beep on the piezoPin if defined
    @param  freq Desired tone frequency
    @param  duration Length of beep in ms
*/
/**************************************************************************/
void Adafruit_TestBed::beep(uint32_t freq, uint32_t duration) {
  if (piezoPin < 0)
    return;
  pinMode(piezoPin, OUTPUT);
#if !defined(ARDUINO_ARCH_ESP32)
  tone(piezoPin, freq, duration);
#endif
}

/**************************************************************************/
/*!
    @brief  Perform a 250ms 2KHz beep on the piezoPin, and light the LED for
   500ms, if defined
*/
/**************************************************************************/
void Adafruit_TestBed::beepNblink(void) {
  if (ledPin >= 0) {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, HIGH);
  }

  beep(2000, 250);

  delay(500);

  if (ledPin >= 0) {
    digitalWrite(ledPin, LOW);
  }
}

Adafruit_TestBed TB;
