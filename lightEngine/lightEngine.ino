#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>

#ifdef __AVR__
#include <avr/power.h>
#include <avr/sleep.h>
#endif

#define PIN 6
#define B 0
#define G 1
#define R 2
#define W 3

union npxColor {
  uint32_t bgrw;
  uint8_t bgrws[4];
};
/*********************NeoPixex*************************/
#define LEDS 60
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, PIN, NEO_GRB + NEO_KHZ800);

uint32_t reds [LEDS];
uint32_t oranges [LEDS];
uint32_t blues [LEDS];



/*********************GPS*************************/
SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  true
/* set to true to only log to SD when GPS has a fix, for debugging, keep it false */
#define LOG_FIXONLY false

boolean usingInterrupt = false;

// Set the pins used
#define GPS_CS
#define GPS_LED 13
#define GPS_MOSI
#define GPS_MISO
#define GPS_SCK

#define SD_CS 10
#define SD_MOSI 11
#define SD_MISO 12
#define SD_SCK 13


File logfile;

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

void setup() {

  randomSeed(analogRead(0));
  Serial.begin(9600);

  initGPS();

  reset(reds);
  reset(oranges);
  reset(blues);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

}

void loop() {
  int gpsStatus = updateGPS();
  if (gpsStatus != 3)
    Serial.println(gpsStatus);
  orangeFlame();

  if (millis() > 5000) {
    union npxColor duty;
    duty.bgrws[R] = min(255, 255 * (millis() - 5000) / 5000);
    duty.bgrws[G] = min(255, 255 * (millis() - 5000) / 30000);
    duty.bgrws[B] = min(255, 255 * (millis() - 5000) / 100000);

    //orangeHighlights();
    for (int i = 0; i < LEDS; i++) {
      union npxColor thisColor;
      thisColor.bgrw = strip.getPixelColor(i);


      if (thisColor.bgrws[R] < duty.bgrws[R])
        thisColor.bgrws[R] = duty.bgrws[R];
      if (thisColor.bgrws[G] < duty.bgrws[G])
        thisColor.bgrws[G] = duty.bgrws[G];
      if (thisColor.bgrws[B] < duty.bgrws[B])
        thisColor.bgrws[B] = duty.bgrws[B];

      if (oranges[i] != 0)
        strip.setPixelColor(i, oranges[i]);
      else
        strip.setPixelColor(i, thisColor.bgrw);
    }

  }
  strip.show();
}


