#define DECODE_DISTANCE_WIDTH
#include <IRremote.hpp>
#include <FastLED.h>

// Config
#define LED_TYPE WS2812B
const EOrder ledColorOrder = GRB;
const uint8_t ledDataPin = 39;
const uint8_t ledCount = 183;
const uint8_t ledBrightness = 128;
const uint8_t ledVoltage = 5;
const uint32_t maxMilliampsPowerDraw = 1500;

const uint8_t IrReceiverPin = 3;

const unsigned long autoTurnOffAfterIdleMillis = 30 * 60 * 1000;

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
  ChangeHue,
  IncreaseHue,
  DecreaseHue,
  IncreaseBightness,
  DecreaseBightness
};


// Global varibales
CRGB leds[ledCount];

uint8_t mode = Off;
uint8_t previousMode = SingleColor;

uint8_t hue = 0;
uint8_t nextHue = 0;

uint8_t brightness = 255;

Action action = Idle;
unsigned long lastActionMillis = 0;


void setup() {
  Serial.begin(115200);

  delay(3000);  // power-up safety delay
  IrReceiver.begin(IrReceiverPin, ENABLE_LED_FEEDBACK);
  FastLED.addLeds<LED_TYPE, ledDataPin, ledColorOrder>(leds, ledCount).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(ledBrightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(ledVoltage, maxMilliampsPowerDraw);
}

// TODO
// * Speed
// * Blålys

void loop() {
  handleIr();
  handleAction();
  autoOff();

  bool modeHasChanged = mode != previousMode;

  if (mode == Off && modeHasChanged) {
    FastLED.clear();
    FastLED.show();
  } else if (mode == SingleColor) {
    SingleColorMode(hue, brightness, modeHasChanged);
  } else if (mode == CycleColors) {
    int updatesPerSecond = 100;
    CycleColorsMode(brightness, updatesPerSecond);
  } else if (mode == Cylon) {
    CylonMode(hue, brightness);
  }
}

void handleIr() {
  if (IrReceiver.decode()) {
    //Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);

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
        case 0x9820009:
          action = IncreaseHue;
          break;
        case 0x2C50009:
          action = DecreaseHue;
          break;
        case 0xF50009:
          action = IncreaseBightness;
          break;
        case 0x1E20009:
          action = DecreaseBightness;
          break;
      }
    }
    IrReceiver.resume();
  }
}

void handleAction() {
  static bool actionCooldown = false;
  if (action != Idle && !actionCooldown) {
    Serial.print("Performing action: ");
    Serial.println(action);
    switch (action) {
      case ToggleOnOFF:
        if (mode == Off) {
          changeMode(previousMode);
        } else {
          changeMode(Off);
        }
        break;
      case ToggleMode:
        ToggleActiveMode();
        break;
      case ChangeHue:
        hue = nextHue;
        break;
      case IncreaseHue:
        if (hue < 245) {
          hue += 10;
        } else {
          hue = 255;
        }
        break;
      case DecreaseHue:
        if (hue > 10) {
          hue -= 10;
        } else {
          hue = 1;
        }
        break;
      case IncreaseBightness:
        if (brightness < 245) {
          brightness += 10;
        } else {
          brightness = 255;
        }
        break;
      case DecreaseBightness:
        if (brightness > 10) {
          brightness -= 10;
        } else {
          brightness = 1;
        }
        break;
    }
    lastActionMillis = millis();
    actionCooldown = true;
  }

  if (actionCooldown && (millis() - lastActionMillis >= 100)) {
    action = Idle;
    actionCooldown = false;
  }
}

void SingleColorMode(uint8_t hue, uint8_t brightness, bool reset) {
  static uint8_t lastHue = 255;
  static uint8_t lastBrightness = 255;
  if (reset || hue != lastHue || brightness != lastBrightness) {
    lastHue = hue;
    lastBrightness = brightness;
    for (int i = 0; i < ledCount; i++) {
      leds[i] = CHSV(hue, 255, brightness);
    }
    FastLED.show();
  }
}

void CycleColorsMode(uint8_t brightness, int updatesPerSecond) {
  static unsigned long previousMillis = millis();
  static uint8_t startHue = 0;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= (1000 / updatesPerSecond)) {
    previousMillis = currentMillis;

    uint8_t hue = startHue++;
    for (int i = 0; i < ledCount; i++) {
      leds[i] = CHSV(hue, 255, brightness);
      hue += 3;
    }
    FastLED.show();
  }
}

void CylonMode(uint8_t hue, uint8_t brightness) {
  static unsigned long previousMillis = millis();
  static uint8_t ledIndex = 0;
  static bool rising = true;

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 20) {
    previousMillis = currentMillis;

    leds[ledIndex] = CHSV(hue, 255, brightness);
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
  changeMode(mode + 1);
  if (mode > 3) {
    changeMode(1);
  }
}

void changeMode(uint8_t newMode) {
  previousMode = mode;
  mode = newMode;
  Serial.print("Mode changed to ");
  Serial.println(mode);
}

void autoOff() {
  if (mode != Off && millis() - lastActionMillis >= autoTurnOffAfterIdleMillis) {
    changeMode(Off);
  }
}
