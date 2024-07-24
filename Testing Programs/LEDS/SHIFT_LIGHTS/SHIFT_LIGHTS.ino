#include <FastLED.h>

#define LED_PIN     4
#define NUM_LEDS    30
#define BRIGHTNESS  64
#define SHIFT_DELAY 40
#define COLOR_ORDER GRB
#define LED_TYPE    WS2811

CRGB leds[NUM_LEDS];

void turnOffLEDs() {
  for(int i = 0; i <= NUM_LEDS; i++) {
    leds[i].setRGB(0,0,0); 
  }
  delay(10); 
  FastLED.show();
}

void simulateRPMLights(){
 for(int i = 0; i < NUM_LEDS; i++) {

  // LEDs should be Green
  if(i < 10) {
     leds[i].setRGB(0,BRIGHTNESS,0);
     delay(SHIFT_DELAY);
     FastLED.show();
  }

  // LEDs should be Red
  else if(i < 20) {
    leds[i].setRGB(BRIGHTNESS,0,0);
    delay(SHIFT_DELAY);
    FastLED.show();
  }

  // LEDs should be Blue
  else if(i < 30) {
    leds[i].setRGB(0,0,BRIGHTNESS);
    delay(SHIFT_DELAY);
    FastLED.show();
  }
  
 }
 
}

void setup() {
  delay(3000); 
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

void loop() {
  simulateRPMLights();
  delay(1000); 
  turnOffLEDs();
  delay(4000);
}
