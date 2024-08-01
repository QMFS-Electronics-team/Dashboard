//----------------
// References
//----------------
// Display
// https://techtutorialsx.com/2021/01/31/esp32-ili9341-display-hello-world/#Testing_the_code
// https://arduino.stackexchange.com/questions/50576/elegant-solution-for-refreshing-tft-display-content
// GPS
// https://arduinogetstarted.com/tutorials/arduino-gps
// LEDs
// https://raw.githubusercontent.com/RuiSantosdotme/Random-Nerd-Tutorials/master/Projects/Arduino_WS2812B_Color_Palette.ino
// CAN BUS
// https://copperhilltech.com/blog/esp32-triple-can-bus-application-through-adding-two-mcp2515-ports/

//----------------
// Definitions
//----------------

// Display
#define TFT_SCK    18
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_CS     22
#define TFT_DC     3
#define TFT_RESET  15

// GUI Default Values
#define TEXT_SIZE         4
#define BACKGROUND_COLOUR BLACK
#define FONT_COLOUR       ORANGE
#define START_COLUMN      20
#define HORIZONTAL        3

// LEDs
#define LED_PIN       4
#define NUM_LEDS      30
#define BRIGHTNESS    64
#define SHIFT_DELAY   1
#define MAX_SHIFT_RPM 3000
#define COLOR_ORDER   GRB
#define LED_TYPE      WS2811

// GPS
#define TXPIN   35
#define RXPIN   34
#define GPSBAUD 9600

// SD Card
#define SDCS 5

// CAN BUS
#define MCPCS 2

//----------------
// Libraries
//----------------

#include <Adafruit_HMC5883_U.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino_GFX_Library.h>
#include <FS.h>
#include <FastLED.h>
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <mcp2515.h>

//----------------
// Objects
//----------------

// Display
Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);

// 3 Axis Gyro
Adafruit_MPU6050 mpu;

// GPS and Compass
TinyGPSPlus gps;                        // The TinyGPS++ object
SoftwareSerial gpsSerial(TXPIN, RXPIN); // The serial interface to the GPS device
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

// LEDs
int rpmLightInterval = MAX_SHIFT_RPM / NUM_LEDS;
CRGB leds[NUM_LEDS];

// RPM Lights simulation
boolean rpmState = true;

// SD Card (Write)
String outputString;

// CAN BUS
struct can_frame canMsg;
MCP2515 mcp2515(MCPCS);

//----------------
// Setup Functions
//----------------

void setup_three_axis_gyro(void) {
  while (!Serial)
    delay(10); // will pause until serial console opens

  Serial.println(F("Setting up MPU6050"));

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println(F("No MPU6050 detected"));
    while (1) {
      delay(10);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void setup_gps(void) {
  gpsSerial.begin(GPSBAUD);
  Serial.println(F("Setting up BN880"));
}

void setup_compass(void) {
  Serial.println(F("Setting up HMC5883"));

  // Initialise the sensor
  if (!mag.begin()) {
    Serial.println(F("No HMC5883 detected"));
    while (1);
  }
}

void setup_leds() {
  delay(500); // Delay powerup
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

void set_display_data() {
  Serial.println(F("Setting up Display"));
  display.begin();
  display.fillScreen(BACKGROUND_COLOUR);
  display.setRotation(HORIZONTAL);
  display.setTextSize(TEXT_SIZE);
  display.setTextColor(FONT_COLOUR);

  display.setCursor(START_COLUMN, 20);
  display.print(F("FS Dashboard"));

  display.setCursor(START_COLUMN, 60);
  display.print(F("RPM:   1200"));

  display.setCursor(START_COLUMN, 100);
  display.print(F("MPH:   30"));

  display.setCursor(START_COLUMN, 140);
  display.print(F("Gear:  5"));

  display.setCursor(START_COLUMN, 180);
  display.print(F("Fuel:  1000"));
}

void setup_sd_card() {

  Serial.println(F("Setting up SD Reader"));
  if (!SD.begin(SDCS)) {
    Serial.println(F("Card Mount Failed"));
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println(F("No SD card attached"));
    return;
  }

  listDir(SD, "/", 0);

  // GPS Data
  if (!SD.exists("/gps-data/gps-data.txt")) {
    Serial.println(F("Creating GPS File"));
    createDir(SD, "/gps-data");
    writeFile(SD, "/gps-data/gps-data.txt", "Start of GPS Data\n");
  } else {
    Serial.println(F("GPS File Exists"));
    appendFile(SD, "/gps-data/gps-data.txt", "Start of New GPS Data\n");
  }

  // Compass Data
  if (!SD.exists("/compass-data/compass-data.txt")) {
    Serial.println(F("Creating Compass File"));
    createDir(SD, "/compass-data");
    writeFile(SD, "/compass-data/compass-data.txt", "Start of Compass Data\n");
  } else {
    Serial.println(F("Compass File Exists"));
    appendFile(SD, "/compass-data/compass-data.txt", "Start of New Compass Data\n");
  }

  // MPU Data
  if (!SD.exists("/mpu-data/mpu-data.txt")) {
    Serial.println(F("Creating MPU File"));
    createDir(SD, "/mpu-data");
    writeFile(SD, "/mpu-data/mpu-data.txt", "Start of MPU Data\n");
  } else {
    Serial.println(F("MPU File Exists"));
    appendFile(SD, "/mpu-data/mpu-data.txt", "Start of New MPU Data\n");
  }

  // CAN Data
  if (!SD.exists("/can-bus-data/can-bus-data.txt")) {
    Serial.println(F("Creating CAN-BUS File"));
    createDir(SD, "/can-bus-data");
    writeFile(SD, "/can-bus-data/can-bus-data.txt", "Start of CAN-BUS Data\n");
  } else {
    Serial.println(F("CAN-BUS File Exists"));
    appendFile(SD, "/can-bus-data/can-bus-data.txt", "Start of New CAN-BUS Data\n");
  }

  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void setup_can_bus() {
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Set CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                   // Set CAN at normal mode
}

//----------------
// Sensor Related
//----------------
void get_three_axis_gyro_data() {

  // Get new sensor events with the readings
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  outputString = "Acceleration X: " + String(a.acceleration.x) + ", Y: " + String(a.acceleration.y) + ", Z: " + String(a.acceleration.z) + " m/s^2" + "\n";
  outputString += "Rotation X: " + String(g.gyro.x) + ", Y: " + String(g.gyro.y) + ", Z: " + String(g.gyro.z) + " rad/s" + "\n";
  outputString += "Temperature: " + String(temp.temperature) + " C" + "\n";

  appendFile(SD, "/mpu-data/mpu-data.txt", outputString.c_str());
  Serial.println(outputString);
}

void read_gps_data() {

  if (gps.location.isValid()) {
    outputString = "Speed (Mph): " + String(gps.speed.mph()) + "\n";
    outputString += "Lat: " + String(gps.location.lat(), 7)  + " Long: " + String(gps.location.lng(), 7) + "\n";
    outputString += "Deg: " + String(gps.course.deg()) + "\n";
    outputString += "Heading: " + String(gps.cardinal(gps.course.value())) + "\n";
    outputString += "Altitude (Miles): " + String(gps.altitude.miles()) + "\n";
  }
  if (gps.satellites.isValid()) {
    outputString += "Number of Satellite: " + String(gps.satellites.value()) + "\n";
  }
  if (gps.date.isValid()) {
    outputString += "Date: " + String(gps.date.day()) + "/" + String(gps.date.month()) + "/" + String(gps.date.year()) + "\n";
    outputString += "Time: " + String(gps.time.hour() + 1) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + "\n";
  }

  appendFile(SD, "/gps-data/gps-data.txt", outputString.c_str());
  Serial.println(outputString);
}

void get_gps_data() {
  boolean newData = false;

  for (int start = millis(); millis() - start < 1000; ) {
    while (gpsSerial.available()) {
      if (gps.encode(gpsSerial.read())) {
        newData = true;
        break;
      }
    }
  }

  if (newData) {
    newData = false;
    read_gps_data();
  }

}

void get_compass_data(void) {
  // Get a new sensor event
  sensors_event_t event;
  mag.getEvent(&event);

  // Display the results (magnetic vector values are in micro-Tesla (uT))
  outputString = "Compass - X: " + String(event.magnetic.x) + "  Y:" + String(event.magnetic.y) + "  Z:" + String(event.magnetic.z) + "  uT\n";

  // Hold the module so that Z is pointing 'up' and you can measure the heading with x&y
  // Calculate heading when the magnetometer is level, then correct for signs of axis.
  float heading = atan2(event.magnetic.y, event.magnetic.x);

  // Once you have your heading, you must then add your 'Declination Angle'- the 'Error' of the magnetic field in your location.
  // Find yours here: http://www.magnetic-declination.com/
  float declinationAngle = 1.13;
  heading += declinationAngle;

  // Correct for when signs are reversed.
  if (heading < 0)
    heading += 2 * PI;

  // Check for wrap due to addition of declination.
  if (heading > 2 * PI)
    heading -= 2 * PI;

  // Convert radians to degrees for readability.
  float headingDegrees = heading * 180 / M_PI;

  outputString += "Heading (degrees): " + String(headingDegrees) + "\n";
  appendFile(SD, "/compass-data/compass-data.txt", outputString.c_str());
  Serial.println(outputString);
}

int get_can_bus_data() {
  int rpm = -1;
  Serial.println("Getting CAN DATA");
  while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    outputString = "CAN Message ID: " + String(canMsg.can_id, HEX)  + " Message Length: " + String(canMsg.can_dlc, HEX) + " Data: ";

    if (canMsg.can_id == 0) {
      rpm = canMsg.data[0];
      rpm = rpm * 100;
    }

    for (int i = 0; i < canMsg.can_dlc; i++)  {
      outputString += String(canMsg.data[i], HEX);
      outputString += " ";
    }
    outputString += "\n";

    appendFile(SD, "/can-bus-data/can-bus-data.txt", outputString.c_str());
    Serial.println(outputString);
  }

  return rpm;
}

//----------------
// LED Related
//----------------

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

void simulateRPMLights() {
  if (rpmState) {
    simulateRPMIncrease();
  } else {
    simulateRPMDecrease();
  }
  delay(200);
  rpmState = !rpmState;
}

//-----------------------------
// Directory and File Functions
//-----------------------------

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println(F("Failed to open directory"));
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(F("Not a directory"));
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print(F("  DIR : "));
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print(F("  FILE: "));
      Serial.print(file.name());
      Serial.print(F("  SIZE: "));
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println(F("Dir created"));
  } else {
    Serial.println(F("mkdir failed"));
  }
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to open file for writing"));
    return;
  }
  if (file.print(message)) {
    Serial.println(F("File written"));
  } else {
    Serial.println(F("Write failed"));
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println(F("Failed to open file for appending"));
    return;
  }
  if (file.print(message)) {
  } else {
    Serial.println(F("Append failed"));
  }
  file.close();
}

//----------------
// Setup
//----------------

void setup() {
  Serial.begin(115200);
  Serial.println(F("\nSetting up Dashboard"));
  setup_can_bus();
  setup_sd_card();
  setup_three_axis_gyro();
  setup_compass();
  setup_gps();
  setup_leds();
  setRPMLights(0);
  set_display_data();
  Serial.println(F("Dashboard Setup Complete\n"));
}

//----------------
// Main Loop
//----------------

int rpm = 0;
void loop() {
  get_gps_data();
  get_compass_data();
  get_three_axis_gyro_data();
  rpm = get_can_bus_data();
  if (rpm > 0) {
    setRPMLights(rpm);
    rpm = 0;
  }
  //  simulateRPMLights();
}
