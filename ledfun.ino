#include <FastLED.h>

// Config
#define LED_TYPE WS2812B
const EOrder ledColorOrder = GRB;
const uint8_t ledDataPin = 7;
const uint8_t ledCount = 60;
const uint8_t ledBrightness = 100;

const uint8_t button1Pin = 2;
const uint8_t potentiometer1Pin = A0;

// Types
enum Mode {
  SingleColor,
  CycleColors,
  Cylon
};


// Global varibales
CRGB leds[ledCount];
int mode = SingleColor;
volatile bool button1Pressed = false;

void setup() {
  Serial.begin(9600);

  pinMode(button1Pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button1Pin), Button1ISR, FALLING);

  delay(3000);  // power-up safety delay
  FastLED.addLeds<LED_TYPE, ledDataPin, ledColorOrder>(leds, ledCount).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(ledBrightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2300);
}

void loop() {
  bool modeHasChanged = ChangeModeIfButtonPressed();

  int pot1Value = analogRead(potentiometer1Pin);

  if (mode == SingleColor) {
    uint8_t hue = map(pot1Value, 0, 1023, 0, 255);
    SingleColorMode(hue, modeHasChanged);
  } else if (mode == CycleColors) {
    int updatesPerSecond = map(pot1Value, 0, 1023, 50, 1000);
    CycleColorsMode(updatesPerSecond);
  } else if (mode == Cylon) {
    uint8_t hue = map(pot1Value, 0, 1023, 0, 255);
    CylonMode(hue);
  }
}

void SingleColorMode(uint8_t hue, bool reset) {
  static uint8_t lastHue = 255;
  if (reset || hue != lastHue) {
    lastHue = hue;
    for (int i = 0; i < ledCount; i++) {
      leds[i] = CHSV(hue, 255, 255);
    }
    FastLED.show();
  }
}

void CycleColorsMode(int updatesPerSecond) {
  static unsigned long previousMillis = millis();
  static uint8_t startHue = 0;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= (1000 / updatesPerSecond)) {
    previousMillis = currentMillis;

    uint8_t hue = startHue++;
    for (int i = 0; i < ledCount; i++) {
      leds[i] = CHSV(hue, 255, 255);
      hue += 3;
    }
    FastLED.show();
  }
}

void CylonMode(uint8_t hue) {
  static unsigned long previousMillis = millis();
  static uint8_t ledIndex = 0;
  static bool rising = true;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 20) {
    previousMillis = currentMillis;

    leds[ledIndex] = CHSV(hue, 255, 255);
    FastLED.show();
    fadeAll();

    if (ledIndex == (ledCount - 1)) {
      rising = false;
    }
    if (ledIndex == 0) {
      rising = true;
    }
    if (rising) {
      ledIndex += 1;
    } else {
      ledIndex -= 1;
    }
  }
}

void fadeAll() {
  for (int i = 0; i < ledCount; i++) { leds[i].nscale8(200); }
}

void Button1ISR(void) {
  if (!button1Pressed) {
    button1Pressed = true;
  }
}

bool ChangeModeIfButtonPressed() {
  static unsigned long debounceStartedAtMillis = 0;
  static bool debounceActive = false;

  if (button1Pressed) {
    button1Pressed = false;
    debounceActive = true;
    debounceStartedAtMillis = millis();
  }

  unsigned long currentMillis = millis();
  if (debounceActive && (currentMillis - debounceStartedAtMillis >= 200)) {
    debounceActive = false;
  
    mode += 1;
    if (mode > 2) {
      mode = 0;
    }

    Serial.print("Mode changed to ");
    Serial.println(mode);
    return true;
  }

  return false;
}
