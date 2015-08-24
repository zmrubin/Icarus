

void orangeFlame() {
  baseReds();
  orangeHighlights();
  blueSpecs();

  for (int i = 0; i < LEDS; i++) {

    uint32_t thisColor = strip.getPixelColor(i);
     npxColor myRed;
    myRed.bgrw =  reds[i];

    if (blues[i] != 0)
      strip.setPixelColor(i, blues[i]);

     if (((thisColor & 0xff) > 0) && blues[i] != 0)  //has some blue that needs to fade away
      linFade(myRed, i, 5);


    else if (oranges[i] != 0)
      strip.setPixelColor(i, oranges[i]);

    else {

      linFade(myRed, i, 1);
      //strip.setPixelColor(i,myRed.bgrw);
    }
  }




}

void expFade(union npxColor target, uint16_t index, uint16_t rate) {
   npxColor thisColor;
  thisColor.bgrw = strip.getPixelColor(index);
  thisColor.bgrws[R] += (target.bgrws[R] - thisColor.bgrws[R] ) * rate / 100;
  thisColor.bgrws[G] += (target.bgrws[G] - thisColor.bgrws[G] ) * rate / 100;
  thisColor.bgrws[B] += (target.bgrws[B] - thisColor.bgrws[B] ) * rate / 100;
  strip.setPixelColor(index, thisColor.bgrw);
}
void linFade( union npxColor target, uint16_t index, uint16_t rate) {
   npxColor thisColor;
  thisColor.bgrw = strip.getPixelColor(index);
  if (target.bgrws[R] > thisColor.bgrws[R])
    thisColor.bgrws[R] += rate;
  else if (target.bgrws[R] < thisColor.bgrws[R])
    thisColor.bgrws[R] -= rate;

  if (target.bgrws[G] > thisColor.bgrws[G])
    thisColor.bgrws[G] += rate;
  else if (target.bgrws[G] < thisColor.bgrws[G])
    thisColor.bgrws[G] -= rate;

  if (target.bgrws[B] > thisColor.bgrws[B])
    thisColor.bgrws[B] += rate;
  else if (target.bgrws[B] < thisColor.bgrws[B])
    thisColor.bgrws[B] -= rate;

  strip.setPixelColor(index, thisColor.bgrw);
}



void baseReds() {
  int now = millis();
  static int lastUpdateTime = 0;
  if ((now - lastUpdateTime) > 50) {
    lastUpdateTime = millis();
    for (int i = 0; i < strip.numPixels(); i++) {
      reds[i] = strip.Color(random(220, 255), 0, 0);
    }
    //Serial.println("here");
  }
}


void orangeHighlights() {
  int now = millis();
  static int lastUpdateTime = 0;
  if ((now - lastUpdateTime) > 30) {
    lastUpdateTime = millis();
    reset(oranges);
    for (uint16_t i = 0; i < random(1, 5); i++) {
      int startpoint = random(0, strip.numPixels());
      for (uint16_t j = 0; j < random(1, 3); j++) {
        oranges[wrap(startpoint + j)] = strip.Color(random(200, 255), random(30, 110), 0);
        oranges[wrap(startpoint - j)] = strip.Color(random(230, 255), random(30, 110), 0);
      }
    }
    //Serial.println("here");
  }
}
void reset(uint32_t * arr) {
  for (int i = 0 ; i < LEDS; i++)
    arr[i] = 0;
}
void blueSpecs() {
  int now = millis();
  static int lastUpdateTime = 0;
  if ((now - lastUpdateTime) > random(60, 200)) {
    lastUpdateTime = millis();
    reset(blues);
    for (uint16_t i = 0; i < random(1, 4); i++) {
      int startpoint = random(0, strip.numPixels());
      for (uint16_t j = 0; j < random(-7, 2); j++) {
        blues[wrap(startpoint + j)] = strip.Color(random(0, 132), random(0, 20), 240);
        blues[wrap(startpoint - j)] = strip.Color(random(0, 132), random(0, 20), 240);
      }
    }
    //Serial.println("here");
  }
}

uint16_t wrap(int pos) {
  if (pos < 0)  return wrap(strip.numPixels() + pos + 1);
  else if (pos > strip.numPixels()) return wrap(pos - strip.numPixels());
  else return pos;
}
