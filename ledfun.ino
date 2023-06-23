#define DECODE_DISTANCE_WIDTH
#include <IRremote.hpp>
#include <FastLED.h>

// Config
#define LED_TYPE WS2812B
const EOrder ledColorOrder = GRB;
const uint8_t ledDataPin = 39;
const uint8_t ledCount = 15;
const uint8_t ledBrightness = 100;
const uint8_t ledVoltage = 5;
const uint32_t maxMilliampsPowerDraw = 500;

const uint8_t IrReceiverPin = 3;

// Types
enum Mode {
  Off,
  SingleColor,
  CycleColors,
  Cylon
};

enum Action {
  Idle,
  ToggleOnOFF,
  ToggleMode,
  ChangeHue
};


// Global varibales
CRGB leds[ledCount];
uint8_t currentMode = Off;
uint8_t previousMode = SingleColor;
bool modeHasChanged = false;
uint8_t currentHue = 0;
Action action = Idle;
uint8_t nextHue = 0;


void setup() {
  Serial.begin(9600);

  delay(3000);  // power-up safety delay
  IrReceiver.begin(IrReceiverPin, ENABLE_LED_FEEDBACK);
  FastLED.addLeds<LED_TYPE, ledDataPin, ledColorOrder>(leds, ledCount).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(ledBrightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(ledVoltage, maxMilliampsPowerDraw);
}

void loop() {
  handleIr();
  handleActions();

  bool modeHasChanged = currentMode != previousMode;

  if (currentMode == Off && modeHasChanged) {
    FastLED.clear();
    FastLED.show();
  } else if (currentMode == SingleColor) {
    SingleColorMode(currentHue, modeHasChanged);
  } else if (currentMode == CycleColors) {
    int updatesPerSecond = 100;
    CycleColorsMode(updatesPerSecond);
  } else if (currentMode == Cylon) {
    CylonMode(currentHue);
  }
}

void handleIr() {
  if (IrReceiver.decode()) {
    Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

    if (action == Idle) {
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
  if (action != Idle && !actionCooldown) {
    switch (action) {
      case ToggleOnOFF:
        if (currentMode == Off) {
          changeMode(previousMode);
        } else {
          changeMode(Off);
        }
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
    action = Idle;
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
  changeMode(currentMode + 1);
  if (currentMode > 3) {
    changeMode(1);
  }
}

void changeMode(uint8_t newMode) {
  previousMode = currentMode;
  currentMode = newMode;
  Serial.print("Mode changed to ");
  Serial.println(currentMode);
}
