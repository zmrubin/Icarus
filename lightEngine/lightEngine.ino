#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#define USE_DMX_SERIAL_1
#include <Conceptinetics.h>

#ifdef __AVR__
#include <avr/power.h>
#include <avr/sleep.h>
#endif

/*********************DMX*************************/

#define DMX_MASTER_CHANNELS   100
#define RXEN_PIN 2
#define DMX_TX_PIN 4
DMX_Master dmx_master ( DMX_MASTER_CHANNELS, RXEN_PIN );

/*********************NeoPixels*************************/
#define NEO_PIN 6
#define NEO_PIN2 6
#define B 0
#define G 1
#define R 2
#define W 3

typedef union npxColor {
  uint32_t bgrw;
  uint8_t bgrws[4];
} npxColor;

#define LEN 6
#define LEDS 60*5/2 + 60 //LEN*60
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, NEO_PIN, NEO_GRB + NEO_KHZ800);
uint32_t reds [LEDS];
uint32_t oranges [LEDS];
uint32_t blues [LEDS];


// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.


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
/*********************Shitty Wireless*************************/
#define RADIO_A_PIN 47
#define RADIO_B_PIN 51
#define RADIO_C_PIN 45
#define RADIO_D_PIN 49
#define RADIO_PWR_PIN 53
#define RADIO_VT 43

#define RADIO_A (digitalRead(RADIO_A_PIN))
#define RADIO_B (digitalRead(RADIO_B_PIN))
#define RADIO_C (digitalRead(RADIO_C_PIN))
#define RADIO_D (digitalRead(RADIO_D_PIN))
#define RADIO_PRESSED (digitalRead(RADIO_VT))
/*********************System*************************/
int ErrorCode = 0;
uint8_t state = 3;
enum  states {LOW_SPEED = 0, HIGH_SPEED, HIGH_PLATFORM, LOW_PLATFORM, HIGH_PLATFORM_MOTION};
#define MPH_TO_KNOTTS 0.868976
#define PLATFORM_HEIGHT_DELTA 4.2672 // meters
#define STOPPED_LOW_SPEED_THRESHOLD .75 * MPH_TO_KNOTTS
#define LOW_HIGH_SPEED_THRESHOLD 2.8 * MPH_TO_KNOTTS
#define HIGH_LOW_SPEED_THRESHOLD 2.0 * MPH_TO_KNOTTS
#define LOW_STOPPED_SPEED_THRESHOLD .2 * MPH_TO_KNOTTS
#define HIGH_LOW_PLATFORM_THRESHOLD .25 * PLATFORM_HEIGHT_DELTA
#define LOW_HIGH_PLATFORM_THRESHOLD .80 * PLATFORM_HEIGHT_DELTA
#define TOP_SPEED 7.0 * MPH_TO_KNOTTS
#define T1 (filteredAltitude < HIGH_LOW_PLATFORM_THRESHOLD)
#define T2 (filteredAltitude > LOW_HIGH_PLATFORM_THRESHOLD)
#define T3 (filteredSpeed < LOW_STOPPED_SPEED_THRESHOLD)
#define T4 (filteredSpeed > STOPPED_LOW_SPEED_THRESHOLD)
#define T5 (filteredSpeed > LOW_HIGH_SPEED_THRESHOLD)
#define T6 (filteredSpeed < HIGH_LOW_SPEED_THRESHOLD)
#define T7 (T1 && (!T5))
#define T8 T2
#define T9 (T1 && T5)
#define T10 T2
#define T11 T4
#define T12 T3

#define THRUSTER_CHANNEL 6
#define THRUSTER_PHEUMATICS_CHANNEL 40
#define LIGHT_ENGINE_PIN 34
#define MAIN_ENGINE_ON  digitalWrite(LIGHT_ENGINE_PIN,HIGH)
#define MAIN_ENGINE_OFF  digitalWrite(LIGHT_ENGINE_PIN,LOW)
#define THRUSTERS_UP dmx_master.setChannelValue ( THRUSTER_PHEUMATICS_CHANNEL , 255)
#define THRUSTERS_DOWN dmx_master.setChannelValue (THRUSTER_PHEUMATICS_CHANNEL  , 0)
#define THRUSTER_LEDS(r, g, b) dmx_master.setChannelValue(THRUSTER_CHANNEL , 255); dmx_master.setChannelValue(THRUSTER_CHANNEL+4 , r); dmx_master.setChannelValue ( THRUSTER_CHANNEL +5 , g); dmx_master.setChannelValue ( THRUSTER_CHANNEL+6 , b)

unsigned long lastTransitionTime = 0;

float filteredSpeed = 0;
float filteredAltitude, minAltitude, maxAltitude = 1190; //rough playa altitude

/*********************Program*************************/
void setup() {
  randomSeed(analogRead(0));
  Serial.begin(9600);
  initGPS();
  dmx_master.enable ();
  //dmx_master.setChannelRange ( 2, 25, 127 );
  pinMode(DMX_TX_PIN, INPUT); // becasue we aren't really using it
  pinMode( RADIO_VT, INPUT);
  pinMode( RADIO_PWR_PIN, OUTPUT);
  pinMode(LIGHT_ENGINE_PIN, OUTPUT);
  digitalWrite(RADIO_PWR_PIN, HIGH);

  reset(reds);
  reset(oranges);
  reset(blues);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

void loop() {

  uint8_t nextState = state;
  static unsigned long lastStopGo = 0;
  int gpsStatus = updateGPS();
  if (1) { //gpsStatus == 0) {
    filteredSpeed = fastFilter(filteredSpeed, GPS.speed, .2);
    filteredAltitude = fastFilter(filteredAltitude, GPS.altitude, .2);
    //Serial.print("Speed: "); Serial.print(filteredSpeed); Serial.println(GPS.speed);
    //Serial.print("Altitude: "); Serial.print(filteredAltitude); Serial.println(GPS.altitude);

    static void (*neoPixelBlendOut)() = &orangeFlame;

    switch (state) {
      case LOW_SPEED:
        if (T3) nextState = LOW_PLATFORM;
        else if (T5) nextState = HIGH_SPEED;
        else if (T8) nextState = HIGH_PLATFORM_MOTION;

        MAIN_ENGINE_ON;
        THRUSTERS_DOWN;
        THRUSTER_LEDS(255, 255, 255);
        break;
      case HIGH_SPEED:
        if (T6) nextState = LOW_SPEED;
        else if (T10) nextState = HIGH_PLATFORM_MOTION;

        MAIN_ENGINE_ON;
        THRUSTERS_DOWN;
        THRUSTER_LEDS(random(120,220), random(5,40), 0);

        break;
      case HIGH_PLATFORM:
        if (T1) nextState = LOW_PLATFORM;
        else if (T11) nextState = HIGH_PLATFORM_MOTION;

        MAIN_ENGINE_OFF;
        THRUSTERS_UP;
        THRUSTER_LEDS(255, 255, 255);

        break;
      case LOW_PLATFORM:
        if (T2) nextState = HIGH_PLATFORM;
        else if (T4) nextState = LOW_SPEED;

        MAIN_ENGINE_OFF;
        THRUSTERS_DOWN;
        THRUSTER_LEDS(random(120,220), random(5,40), 0);

        break;
      case HIGH_PLATFORM_MOTION:
        if (T7) nextState = LOW_SPEED;
        else if (T9) nextState = HIGH_SPEED;
        else if (T12) nextState = HIGH_PLATFORM;

        MAIN_ENGINE_ON;
        THRUSTERS_UP;
        THRUSTER_LEDS(random(220,255), random(40,80), 0);

        break;
      default:
        Serial.println("how the fuck did i get here?");
        break;
    }

  }//force states
  if ( RADIO_PRESSED) {
    if (RADIO_A) nextState = LOW_PLATFORM;
    else if (RADIO_B) nextState = LOW_SPEED;
    else if (RADIO_C) nextState = HIGH_SPEED;
    else if (RADIO_D) nextState = HIGH_PLATFORM;

  }
  if (nextState != state) {
    lastTransitionTime = millis();
    if (stateType(nextState) != stateType(state))
      lastStopGo = lastTransitionTime;
    Serial.print("Transitioning from state ");
    Serial.print(state);
    Serial.print(" to ");
    Serial.println(nextState);
    state = nextState;
  }


  orangeFlame();
  if ((state == LOW_SPEED) || (state == HIGH_SPEED)) {


    union npxColor duty;
    duty.bgrws[R] = min(255, 255 * (millis() - lastStopGo) / 5000);
    duty.bgrws[G] = min(255, 255 * (millis() - lastStopGo) / 19000);
    duty.bgrws[B] = min(255, 255 * (millis() - lastStopGo) / 100000);

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

  static long lastErrorCheck =  0;
  
  if ( (ErrorCode != 0) && (millis() - lastErrorCheck)  > 1000) {

    lastErrorCheck = millis();
    blinkError(ErrorCode);
  }
}
//transition ( (millis() - startTime)5000,30000, 100000)
/*
void transition(  union npxColor *from, union  npxColor *to, union npxColor * result, uint16_t durR, uint16_t durG, uint16_t durB, int scalar) {

  npxColor duty;
  duty.bgrws[R] = min(1000, scalar / durR);
  duty.bgrws[G] = min(1000, scalar / durG);
  duty.bgrws[B] = min(1000, scalar / durB);


  for (int i = 0; i < LEDS; i++) {
    npxColor thisColor;
    thisColor.bgrw = strip.getPixelColor(i);
    to[i].bgrws[R] = map(duty.bgrws[R], 0, 255, min(from[i].bgrws[R], to[i].bgrws[R]), max(from[i].bgrws[R], to[i].bgrws[R]));
    to[i].bgrws[G] = map(duty.bgrws[G], 0, 255, min(from[i].bgrws[G], to[i].bgrws[G]), max(from[i].bgrws[G], to[i].bgrws[G]));
    to[i].bgrws[B] = map(duty.bgrws[B], 0, 255, min(from[i].bgrws[B], to[i].bgrws[B]), max(from[i].bgrws[B], to[i].bgrws[B]));
    /*
          if (thisColor.bgrws[R] < duty.bgrws[R])
            thisColor.bgrws[R] = duty.bgrws[R];
          if (thisColor.bgrws[G] < duty.bgrws[G])
            thisColor.bgrws[G] = duty.bgrws[G];
          if (thisColor.bgrws[B] < duty.bgrws[B])
            thisColor.bgrws[B] = duty.bgrws[B];


 }
}*/

uint8_t stateType(uint8_t s) {
  if (s <= 1)
    return 1;
  else
    return 0;
}




void blinkError(int errno) {
  static uint8_t count = 0;
  if (count < 2 * errno) {
    if (count % 2)
      digitalWrite(GPS_LED, LOW);
    else
      digitalWrite(GPS_LED, HIGH);
  }
  count++;
  if (count >= 10)
    count = 0;
}


float fastFilter(float oldVal, float newVal, float alpha) {
  return (oldVal - newVal) * alpha + oldVal;
}
