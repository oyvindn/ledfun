#define DECODE_DISTANCE_WIDTH
#include <IRremote.hpp>
#include <FastLED.h>

// Config
#define LED_TYPE WS2812B
const EOrder ledColorOrder = GRB;
const uint8_t ledDataPin = 39;
const uint8_t ledCount = 60;
const uint8_t ledBrightness = 100;

const uint8_t IrReceiverPin = 3;

// Types
enum Mode {
  Off,
  SingleColor,
  CycleColors,
  Cylon
};

enum Action {
  None,
  ToggleOnOFF,
  ToggleMode,
  ChangeHue
};


// Global varibales
CRGB leds[ledCount];
int mode = SingleColor;
int lastMode = SingleColor;
uint8_t currentHue = 0;
Action action = None;
uint8_t nextHue = 0;


void setup() {
  Serial.begin(9600);

  delay(3000);  // power-up safety delay
  IrReceiver.begin(IrReceiverPin, ENABLE_LED_FEEDBACK);
  FastLED.addLeds<LED_TYPE, ledDataPin, ledColorOrder>(leds, ledCount).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(ledBrightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 2300);
}

void loop() {
  handleIr();
  handleActions();


  bool modeHasChanged = mode != lastMode;

  if (modeHasChanged && mode == Off) {
    FastLED.clear();
    FastLED.show();
  } else if (mode == SingleColor) {
    SingleColorMode(currentHue, modeHasChanged);
  } else if (mode == CycleColors) {
    int updatesPerSecond = 100;
    CycleColorsMode(updatesPerSecond);
  } else if (mode == Cylon) {
    CylonMode(currentHue);
  }

  if (mode != Off)
    lastMode = mode;
}

void handleIr() {
  if (IrReceiver.decode()) {
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

    if (action == None) {
      switch (IrReceiver.decodedIRData.decodedRawData) {
        case 0x1880009:
          action = ToggleOnOFF;
          break;
        case 0x7880009:
          action = ToggleMode;
          break;
        case 0x1820009:
          action = ChangeHue;
          nextHue = HUE_GREEN;
          break;
        case 0x7810009:
          action = ChangeHue;
          nextHue = HUE_YELLOW;
          break;
        case 0x3840009:
          action = ChangeHue;
          nextHue = HUE_BLUE;
          break;
        case 0x1ED0009:
          action = ChangeHue;
          nextHue = HUE_RED;
          break;
      }
    }
    IrReceiver.resume();
  }
}

void handleActions() {
  static unsigned long lastActionTime = 0;
  static bool actionCooldown = false;
  if (action != None && !actionCooldown) {
    switch (action) {
      case ToggleOnOFF:
        if (mode == Off)
          mode = lastMode;
        else
          mode = Off;
        break;
      case ToggleMode:
        ToggleActiveMode();
        break;
      case ChangeHue:
        currentHue = nextHue;
        break;
    }
    lastActionTime = millis();
    actionCooldown = true;
  }

  if (actionCooldown && (millis() - lastActionTime >= 300)) {
    action = None;
    actionCooldown = false;
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

void ToggleActiveMode() {
  mode += 1;
  if (mode > 3) {
    mode = 1;
  }
  Serial.print("Mode changed to ");
  Serial.println(mode);
}
