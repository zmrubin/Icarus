void printState(uint8_t s) {
  switch (s) {
    case LOW_SPEED:
      Serial.print("LOW SPEED");
      break;
    case HIGH_SPEED:
      Serial.print("HIGH SPEED");
      break;
    case HIGH_PLATFORM:
      Serial.print("HIGH PLATFORM");
      break;
    case LOW_PLATFORM:
      Serial.print("LOW PLATFORM ");
      break;
    case HIGH_PLATFORM_MOTION:
      Serial.print("HIGH PLATFORM MOTION");
      break;
    default:
      Serial.print("WTF");
      break;
  }
}

