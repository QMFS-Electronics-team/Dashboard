// References: 
// https://techtutorialsx.com/2021/01/31/esp32-ili9341-display-hello-world/#Testing_the_code
// https://arduino.stackexchange.com/questions/50576/elegant-solution-for-refreshing-tft-display-content

#include <Arduino_GFX_Library.h>

// Pins on display
#define TFT_SCK    18
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_CS     22
#define TFT_DC     3
#define TFT_RESET  15

// Default values
#define TEXT_SIZE 4
#define BACKGROUND_COLOUR BLACK
#define FONT_COLOUR ORANGE
#define START_COLUMN 20
#define HORIZONTAL 3

void setup(void)
{
  Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
  Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);
  
  display.begin();
  display.fillScreen(BACKGROUND_COLOUR);
  display.setRotation(HORIZONTAL);
  display.setTextSize(TEXT_SIZE);
  display.setTextColor(FONT_COLOUR);
  
  display.setCursor(START_COLUMN, 20);
  display.print("FS Dashboard");

  display.setCursor(START_COLUMN, 60);
  display.print("RPM:   1200");

  display.setCursor(START_COLUMN, 100);
  display.print("MPH:   30");

  display.setCursor(START_COLUMN, 140);
  display.print("Gear:  5");
  
  display.setCursor(START_COLUMN, 180);
  display.print("Fuel:  1000");
  Serial.begin(9600);
  Serial.println("Done");
}
 
void loop() {

}
