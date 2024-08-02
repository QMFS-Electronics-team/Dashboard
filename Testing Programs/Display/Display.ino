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
#define TEXT_SIZE         4
#define BACKGROUND_COLOUR BLACK
#define FONT_COLOUR       ORANGE
#define START_COLUMN      20
#define HORIZONTAL        3

#define COLUMN_OFFSET    180


Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);


void setup_display() {
  display.begin();
  display.fillScreen(BACKGROUND_COLOUR);
  display.setRotation(HORIZONTAL);
  display.setTextSize(TEXT_SIZE);
  display.setTextColor(FONT_COLOUR);

  display.setCursor(START_COLUMN, 20);
  display.print("FS Dashboard");

  display.setCursor(START_COLUMN, 60);
  display.print("RPM:");

  display.setCursor(START_COLUMN, 100);
  display.print("MPH:");

  display.setCursor(START_COLUMN, 140);
  display.print("Gear:");

  display.setCursor(START_COLUMN, 180);
  display.print("Fuel:");
}

void set_rpm(int rpm_value, bool clear_text) {
  String text = String(rpm_value);
  if (clear_text) {
    display.setTextColor(BLACK);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 60);
    display.print(text);
  } else {
    display.setTextColor(FONT_COLOUR);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 60);
    display.print(text);
  }
}

void set_speed(int speed_value, bool clear_text) {
  String text = String(speed_value);
  if (clear_text) {
    display.setTextColor(BLACK);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 100);
    display.print(text);
  } else {
    display.setTextColor(FONT_COLOUR);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 100);
    display.print(text);
  }
}

void set_gear(int gear_value, bool clear_text) {
  String gear_text = "";
  if (gear_value == 0) {
    gear_text = "N";
  } else {
    gear_text = String(gear_value);
  }

  if (clear_text) {
    display.setTextColor(BLACK);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 140);
    display.print(gear_text);
  } else {
    display.setTextColor(FONT_COLOUR);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 140);
    display.print(gear_text);
  }
}

void set_fuel(int fuel_value, bool clear_text) {
  String text = String(fuel_value);
  if (clear_text) {
    display.setTextColor(BLACK);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 180);
    display.print(text);
  } else {
    display.setTextColor(FONT_COLOUR);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, 180);
    display.print(text);
  }
}

void setup(void) {
  Serial.begin(9600);
  Serial.println("Setting Up Display");
  setup_display();
  Serial.println("Display Setup Complete");
}

void loop() {

  int rpm = 0;
  int fuel = 1000;
  int gear = 0;

  // x denotes speed
  for (int x = 0; x < 30; x += 5) {
    // Set values
    set_speed(x, false);

    rpm = x * 100;
    set_rpm(rpm, false);

    if (rpm == 0) {
      gear = 0;
    } else if (rpm < 1000) {
      gear = 1;
    } else if (rpm < 1500) {
      gear = 2;
    } else if (rpm < 2000) {
      gear = 3;
    } else if (rpm < 2500) {
      gear = 4;
    } else if (rpm < 3000) {
      gear = 5;
    }

    set_gear(gear, false);

    fuel -= 10;
    set_fuel(fuel, false);

    if (gear == 0) {
      delay(3000);
    }

    if (x == 30) {
      delay(2000);
    }

    // Clear Values
    delay(1000);
    set_speed(x, true);
    set_rpm(rpm, true);
    set_gear(gear, true);
    set_fuel(fuel, true);


    if (x == 30) {
      break;
    }
  }


  for (int x = 30; x > 0; x -= 5) {
    if (x == 0) {
      break;
    }

    // Set values
    set_speed(x, false);

    rpm = x * 100;
    set_rpm(rpm, false);

    if (rpm == 0) {
      gear = 0;
    } else if (rpm < 1000) {
      gear = 1;
    } else if (rpm < 1500) {
      gear = 2;
    } else if (rpm < 2000) {
      gear = 3;
    } else if (rpm < 2500) {
      gear = 4;
    } else if (rpm < 3000) {
      gear = 5;
    }

    set_gear(gear, false);

    fuel -= 10;
    set_fuel(fuel, false);

    if (gear == 0) {
      delay(3000);
    }

    if (x == 0) {
      delay(2000);
    }

    // Clear Values
    delay(1000);

    // Maintain values on screen if they are larger than 0
    if (x != 0) {
      set_speed(x, true);
      set_rpm(rpm, true);
      set_gear(gear, true);
      set_fuel(fuel, true);
    }

  }

  while (1) {
    // Stop program here
  }
}
