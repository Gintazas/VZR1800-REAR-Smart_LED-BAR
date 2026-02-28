#include <FastLED.h>

#define NUM_LEDS 80
#define DATA_PIN 13
#define LED_TYPE WS2812B
#define COLOR_ORDER RGB 

CRGB leds[NUM_LEDS];

const int pinL = 2;
const int pinR = 3;
const int pinKnight = 5;
const int pinBrake = 6;
const int pinTail = 7;
const int pinSmartBrakeEn = 9;

int half = NUM_LEDS / 2;
int stateLT = 0;
int stateRT = 0;
bool brakeActive = false; 
bool lastStaticState = false; 

unsigned long lastDebounceL = 0;
unsigned long lastDebounceR = 0;
const int debounceDelay = 50;

// REGULIAVIMAS
int animSpeed = 13;                 
int offDelay = 180;                 
int extraFlashes = 3;               
unsigned long turnColor = 0xFFAF00; 

void setup() {
  pinMode(pinL, INPUT);
  pinMode(pinR, INPUT);
  pinMode(pinKnight, INPUT);
  pinMode(pinBrake, INPUT);
  pinMode(pinTail, INPUT);
  pinMode(pinSmartBrakeEn, INPUT);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(200);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

// --- TIKROJI ŠIAURĖJANTI KOMETOS UODEGA ---
void shrinkToCenter(int speedMs) {
  // Ciklas vyksta, kol visi LED užgęsta (pridedame extra žingsnių uodegai)
  for (int i = 0; i < half + 30; i++) {
    
    // Einame per visus LED ir blukiname tik tuos, kurie yra "užgesimo zonoje"
    for (int j = 0; j < half; j++) {
      if (j <= i) { 
        // Kuo toliau į šoną nuo judančio i taško, tuo tamsiau turėtų būti,
        // bet paprasčiausias būdas yra blukinti viską, kas išeina už i ribų
        leds[j].fadeToBlackBy(50);                // Kairė pusė gęsta
        leds[NUM_LEDS - 1 - j].fadeToBlackBy(50); // Dešinė pusė gęsta
      }
    }

    FastLED.show();
    delay(speedMs);
  }
  
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void handleStaticLights() {
  if (digitalRead(pinTail) == HIGH) {
    fill_solid(leds, NUM_LEDS, CHSV(0, 255, 60)); 
  } 
  else if (digitalRead(pinKnight) == HIGH) {
    fadeToBlackBy(leds, NUM_LEDS, 45); 
    int pos = beatsin16(15, 0, NUM_LEDS - 1);
    leds[pos] = CRGB::Red; 
  } 
  else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
  FastLED.show();
}

void handleBrake() {
  bool smartEnabled = digitalRead(pinSmartBrakeEn);
  if (smartEnabled && !brakeActive) {
    for (int i = 0; i < 3; i++) {
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      FastLED.show();
      delay(60);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      delay(60);
    }
    brakeActive = true; 
  }
  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
}

void runLeft() {
  for (int i = 0; i < half; i++) {
    leds[half - 1 - i] = turnColor;
    FastLED.show();
    delay(animSpeed);
  }
  delay(offDelay);
  CRGB backColor = (digitalRead(pinTail) == HIGH) ? CRGB(CHSV(0, 255, 60)) : CRGB::Black;
  for (int i = 0; i < half; i++) leds[half - 1 - i] = backColor;
  FastLED.show();
  delay(offDelay);
  if (digitalRead(pinL) == LOW) {
    stateLT++;
    if (stateLT > extraFlashes) stateLT = 0;
  }
}

void runRight() {
  for (int i = 0; i < half; i++) {
    leds[half + i] = turnColor;
    FastLED.show();
    delay(animSpeed);
  }
  delay(offDelay);
  CRGB backColor = (digitalRead(pinTail) == HIGH) ? CRGB(CHSV(0, 255, 60)) : CRGB::Black;
  for (int i = 0; i < half; i++) leds[half + i] = backColor;
  FastLED.show();
  delay(offDelay);
  if (digitalRead(pinR) == LOW) {
    stateRT++;
    if (stateRT > extraFlashes) stateRT = 0;
  }
}

void runHazard() {
  for (int i = 0; i < half; i++) {
    leds[half - 1 - i] = turnColor;
    leds[half + i] = turnColor;
    FastLED.show();
    delay(animSpeed);
  }
  delay(offDelay);
  CRGB backColor = (digitalRead(pinTail) == HIGH) ? CRGB(CHSV(0, 255, 60)) : CRGB::Black;
  fill_solid(leds, NUM_LEDS, backColor);
  FastLED.show();
  delay(offDelay);
  stateLT = 0; stateRT = 0;
}

void loop() {
  bool readL = digitalRead(pinL);
  bool readR = digitalRead(pinR);
  bool readBrake = digitalRead(pinBrake);
  bool currentStatic = (digitalRead(pinTail) == HIGH || digitalRead(pinKnight) == HIGH);

  // Stabdžių užgesimas
  if (!readBrake && brakeActive) {
    shrinkToCenter(5); // Padidintas greitis geresniam vaizdui
    brakeActive = false;
  }

  if (readL) {
    if (millis() - lastDebounceL > debounceDelay && stateLT == 0) stateLT = 1;
  } else { lastDebounceL = millis(); }

  if (readR) {
    if (millis() - lastDebounceR > debounceDelay && stateRT == 0) stateRT = 1;
  } else { lastDebounceR = millis(); }

  if (readBrake) {
    handleBrake();
  } 
  else if (readL && readR) {
    runHazard();
  } 
  else if (stateLT > 0) {
    runLeft();
  } 
  else if (stateRT > 0) {
    runRight();
  } 
  else {
    if (lastStaticState == true && currentStatic == false) {
      shrinkToCenter(5);
    }
    handleStaticLights();
  }
  lastStaticState = currentStatic;
}
