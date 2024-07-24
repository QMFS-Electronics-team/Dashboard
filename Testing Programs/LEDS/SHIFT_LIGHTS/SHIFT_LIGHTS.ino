#include <FastLED.h>

#define LED_PIN       4
#define NUM_LEDS      30
#define BRIGHTNESS    64
#define SHIFT_DELAY   1
#define MAX_SHIFT_RPM 3000
#define COLOR_ORDER   GRB
#define LED_TYPE      WS2811

int rpmLightInterval = MAX_SHIFT_RPM / NUM_LEDS;

CRGB leds[NUM_LEDS];

void setRPMLights(int rpmValue) {
  for (int i = 0; i <= NUM_LEDS; i++) {
    if (rpmValue >= (i + 1)*rpmLightInterval) {
      if (i < 10) {               // LEDs should be Green
        leds[i].setRGB(0, BRIGHTNESS, 0);
        delay(SHIFT_DELAY);
      } else if (i < 20) {        // LEDs should be Red
        leds[i].setRGB(BRIGHTNESS, 0, 0);
        delay(SHIFT_DELAY);
      } else if (i < 30) {        // LEDs should be Blue
        leds[i].setRGB(0, 0, BRIGHTNESS);
        delay(SHIFT_DELAY);
      }
      FastLED.show();
    } else {
      leds[i].setRGB(0, 0, 0);
      FastLED.show();
    }
  }
}

void simulateRPMIncrease() {
  for (int rpm = 0; rpm < 3500; rpm += 100) {
    setRPMLights(rpm);
  }
}

void simulateRPMDecrease() {
  for (int rpm = 3500; rpm > 0; rpm -= 100) {
    setRPMLights(rpm);
  }
}

void setup() {
  delay(1000);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

void loop() {
  simulateRPMIncrease();
  delay(2000);
  simulateRPMDecrease();
  delay(2000);
}
