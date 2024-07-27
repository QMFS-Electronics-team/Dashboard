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
// Pin definitions
//----------------

// Display
#define TFT_SCK    18
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_CS     22
#define TFT_DC     3
#define TFT_RESET  15

// LEDs
#define LED_PIN     4



//----------------
// Default Values
//----------------

// Display Default values
#define TEXT_SIZE 4
#define BACKGROUND_COLOUR BLACK
#define FONT_COLOUR ORANGE
#define START_COLUMN 20
#define HORIZONTAL 3

// LEDs
#define NUM_LEDS    30
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 100

// A sample NMEA stream.
const char *gpsStream =
  "$GPRMC,045103.000,A,3014.1984,N,09749.2872,W,0.67,161.46,030913,,,A*7C\r\n"
  "$GPGGA,045104.000,3014.1985,N,09749.2873,W,1,09,1.2,211.6,M,-22.5,M,,0000*62\r\n"
  "$GPRMC,045200.000,A,3014.3820,N,09748.9514,W,36.88,65.02,030913,,,A*77\r\n"
  "$GPGGA,045201.000,3014.3864,N,09748.9411,W,1,10,1.2,200.8,M,-22.5,M,,0000*6C\r\n"
  "$GPRMC,045251.000,A,3014.4275,N,09749.0626,W,0.51,217.94,030913,,,A*7D\r\n"
  "$GPGGA,045252.000,3014.4273,N,09749.0628,W,1,09,1.3,206.9,M,-22.5,M,,0000*6F\r\n";



//----------------
// Libraries
//----------------

// Display
#include <Arduino_GFX_Library.h>

// 3 Axis Gyro
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// GPS
#include <TinyGPSPlus.h>

// Compass
#include <Adafruit_HMC5883_U.h>

// LEDs
#include <FastLED.h>

// SD
#include "FS.h"
#include "SD.h"

// CAN BUS
#include <mcp2515.h>

#include "SPI.h"


//----------------
// Objects
//----------------

// Display
Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);

// 3 Axis Gyro
Adafruit_MPU6050 mpu;

// GPS
TinyGPSPlus gps; // the TinyGPS++ object
const int TXPin = 35;
const int RXPin = 34;


// TODO consider changing this to hardware serial?
//SoftwareSerial gpsSerial(TXPin, RXPin); // the serial interface to the GPS device

// Compass
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

// LEDs
CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

// SD Card (Write)
String outputString;

// CAN BUS
struct can_frame canMsg;
MCP2515 mcp2515(2);




//----------------
// Setup Functions
//----------------

void setup_three_axis_gyro(void) {
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G:
      Serial.println("+-2G");
      break;
    case MPU6050_RANGE_4_G:
      Serial.println("+-4G");
      break;
    case MPU6050_RANGE_8_G:
      Serial.println("+-8G");
      break;
    case MPU6050_RANGE_16_G:
      Serial.println("+-16G");
      break;
  }
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
    case MPU6050_RANGE_250_DEG:
      Serial.println("+- 250 deg/s");
      break;
    case MPU6050_RANGE_500_DEG:
      Serial.println("+- 500 deg/s");
      break;
    case MPU6050_RANGE_1000_DEG:
      Serial.println("+- 1000 deg/s");
      break;
    case MPU6050_RANGE_2000_DEG:
      Serial.println("+- 2000 deg/s");
      break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
    case MPU6050_BAND_260_HZ:
      Serial.println("260 Hz");
      break;
    case MPU6050_BAND_184_HZ:
      Serial.println("184 Hz");
      break;
    case MPU6050_BAND_94_HZ:
      Serial.println("94 Hz");
      break;
    case MPU6050_BAND_44_HZ:
      Serial.println("44 Hz");
      break;
    case MPU6050_BAND_21_HZ:
      Serial.println("21 Hz");
      break;
    case MPU6050_BAND_10_HZ:
      Serial.println("10 Hz");
      break;
    case MPU6050_BAND_5_HZ:
      Serial.println("5 Hz");
      break;
  }

  Serial.println("");
  delay(100);
}

void setup_gps(void) {
//  gpsSerial.begin(9600);
  while (*gpsStream)
    if (gps.encode(*gpsStream++))
      get_gps_data();
}

void setup_compass(void) {
  Serial.println("Settingup HMC5883");
  Serial.println("");
  
  // Initialise the sensor
  if(!mag.begin()) {
    Serial.println("No HMC5883 detected ... Check your wiring!");
    while(1);
  }
  
  /* Display some basic information on this sensor */
  get_compass_details();
}

void setup_leds() {
  delay(3000); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
}

void set_display_data() {
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
  Serial.println("Done");
}

void setup_sd_card() {

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  Serial.printf("SD Setup Complete!");

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

void get_gps_data() {

  if (gps.location.isValid())
  {
    outputString += "Lat: " + String(gps.location.lat(), 6)  + " Long: " + String(gps.location.lng(), 6) + "\n";
  }

  if (gps.date.isValid())
  {
    outputString += "Date: " + String(gps.date.month()) + "/" + String(gps.date.day()) + "/" + String(gps.date.year()) + "\n";
  }

  if (gps.time.isValid())
  {
    outputString += "Time: " + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + "\n";
  }

  if (gps.speed.isValid())
  {
    outputString += "Speed (Mph): " + String(gps.speed.mph()) + "\n";
  }

  if (gps.course.isValid())
  {
    outputString += "Deg: " + String(gps.course.deg()) + "\n";
  }

  if (gps.altitude.isValid())
  {
    outputString += "Altitude (Miles): " + String(gps.altitude.miles()) + "\n";
  }

  if (gps.satellites.isValid())
  {
    outputString += "Number of Satellite: " + String(gps.satellites.value()) + "\n";
  }

  appendFile(SD, "/gps-data/gps-data.txt", outputString.c_str());
  Serial.println(outputString);
}

void get_compass_details(void) {
  sensor_t sensor;
  mag.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
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
  if(heading < 0)
    heading += 2*PI;
    
  // Check for wrap due to addition of declination.
  if(heading > 2*PI)
    heading -= 2*PI;
   
  // Convert radians to degrees for readability.
  float headingDegrees = heading * 180/M_PI; 
  
  outputString += "Heading (degrees): " + String(headingDegrees) + "\n";
  appendFile(SD, "/compass-data/compass-data.txt", outputString.c_str());
  Serial.println(outputString);
}

void get_can_bus_data() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    outputString = "CAN Message ID: " + String(canMsg.can_id, HEX)  + " Message Length: " + String(canMsg.can_dlc, HEX) + " Data: ";
    for (int i = 0; i < canMsg.can_dlc; i++)  {
      outputString += String(canMsg.data[i], HEX);
      outputString += " ";
    }
    outputString += "\n";

    appendFile(SD, "/can-bus-data/can-bus-data.txt", outputString.c_str());
    Serial.println(outputString);
  }
}


//----------------
// LED Related
//----------------

void set_leds()
{
  ChangePalettePeriodically();

  static uint8_t startIndex = 0;
  startIndex = startIndex + 1; /* motion speed */

  FillLEDsFromPaletteColors( startIndex);

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for ( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
    colorIndex += 3;
  }
}

void ChangePalettePeriodically()
{
  uint8_t secondHand = (millis() / 1000) % 60;
  static uint8_t lastSecond = 99;

  if ( lastSecond != secondHand) {
    lastSecond = secondHand;
    if ( secondHand ==  0)  {
      currentPalette = RainbowColors_p;
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 10)  {
      currentPalette = RainbowStripeColors_p;
      currentBlending = NOBLEND;
    }
    if ( secondHand == 15)  {
      currentPalette = RainbowStripeColors_p;
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 20)  {
      SetupPurpleAndGreenPalette();
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 25)  {
      SetupTotallyRandomPalette();
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 30)  {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = NOBLEND;
    }
    if ( secondHand == 35)  {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 40)  {
      currentPalette = CloudColors_p;
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 45)  {
      currentPalette = PartyColors_p;
      currentBlending = LINEARBLEND;
    }
    if ( secondHand == 50)  {
      currentPalette = myRedWhiteBluePalette_p;
      currentBlending = NOBLEND;
    }
    if ( secondHand == 55)  {
      currentPalette = myRedWhiteBluePalette_p;
      currentBlending = LINEARBLEND;
    }
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
  for ( int i = 0; i < 16; i++) {
    currentPalette[i] = CHSV( random8(), 255, random8());
  }
}

void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid( currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;

}

void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV( HUE_PURPLE, 255, 255);
  CRGB green  = CHSV( HUE_GREEN, 255, 255);
  CRGB black  = CRGB::Black;

  currentPalette = CRGBPalette16(
                     green,  green,  black,  black,
                     purple, purple, black,  black,
                     green,  green,  black,  black,
                     purple, purple, black,  black );
}

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Red,
  CRGB::Gray,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black
};



//--------------------
// Directory Functions
//--------------------

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}



//---------------
// File Functions
//---------------

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  //    Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //        Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}



//----------------
// Setup
//----------------

void setup() {
  Serial.begin(115200);
  // Serial2.begin(9600, SERIAL_8N1, RXPin, TXPin);
  
  Serial.println("Setting up Dashboard");
  setup_can_bus();
  setup_sd_card();
  setup_three_axis_gyro();
  setup_compass();
  setup_gps();
  setup_leds();
  set_leds();
  set_display_data();
  Serial.println("Dashboard Setup Complete");
}


//----------------
// Main Loop
//----------------

void loop() {

  // if (Serial2.available() > 0) {
  //   get_gps_data();
  // } else {
  //   Serial.println("GPS Serial Not Available");
  // }

  get_compass_data();
  get_three_axis_gyro_data();
  set_leds();
  get_can_bus_data();
  delay(500);
}
