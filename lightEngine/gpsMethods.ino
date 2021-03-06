uint8_t updateGPS() {
  static unsigned long lastLog = 0;

  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) Serial.print(c);
  }

  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse( GPS.lastNMEA()))
      return 1;
    // Sentence parsed!
    Serial.println("OK");
    if (LOG_FIXONLY && !GPS.fix) {
      Serial.print("No Fix");
      return 2;
    }
    if ((millis() - lastLog) >= 2000) {
      lastLog = millis();
      Serial.println("Logging");
      char sentance[128];
      sprintf(sentance, "%u/%u/20%u,%f,%f,%f,%f,%f\n\0",GPS.day,GPS.month,GPS.year,GPS.latitudeDegrees,
      GPS.longitudeDegrees,GPS.speed,GPS.angle,GPS.altitude);
       uint8_t stringsize = strlen(sentance) ;
      if (stringsize != logfile.write((uint8_t *)sentance, stringsize))    //write the string to the SD file
        error(4);
     // if (strstr(stringptr, "RMC") || strstr(stringptr, "GGA"))  
      logfile.flush();
      Serial.println();
    }
    return 0;
  }
  return 3;
}

void initGPS() {
  Serial.println("\r\nInitializing GPS");
  pinMode(GPS_LED, OUTPUT);

  pinMode(SD_CS, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_SCK)) {
    //if (!SD.begin(chipSelect)) {      // if you're using an UNO, you can use this line instead
    Serial.println("Card init. failed!");
    error(2);
  }
  char* filename= "LOG000.csv";
  for (uint16_t i = 0; i < 100; i++) {
    filename[3] = '0' + (i/100) ;
    filename[4] = '0' + (i / 10) % 10;
    filename[5] = '0' + i % 10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if ( ! logfile ) {
    Serial.print("Couldnt create ");
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to ");
  Serial.println(filename);
  char * csvHeader = "Date, Time, Lat, Lon, Speed, Heading, Altitude\n";
  uint8_t stringsize = strlen(csvHeader);
  if (stringsize != logfile.write((uint8_t *)csvHeader, stringsize)) {
    error(4);
  }
  logfile.flush();
  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);   // 100 millihertz (once every 10 seconds), 1Hz or 5Hz update rate
  // Turn off updates on antenna status, if the firmware permits it
  GPS.sendCommand(PGCMD_NOANTENNA);

  useInterrupt(true);

  Serial.println("GPS initialized!");


}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  if (GPSECHO)
    if (c) UDR0 = c;
  // writing direct to UDR0 is much much faster than Serial.print
  // but only one character can be written at a time.
#endif
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  }
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}


void error(uint8_t errno) {
  while (1) {
    uint8_t i;
    for (i = 0; i < errno; i++) {
      digitalWrite(GPS_LED, HIGH);
      delay(100);
      digitalWrite(GPS_LED, LOW);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}

