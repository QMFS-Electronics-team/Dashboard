
// v1.0.0 を有効にします(v0からの移行期間の特別措置です。これを書かない場合は旧v0系で動作します。)
#define LGFX_USE_V1

//----------------
// Definitions
//----------------

// Display
#define TFT_SCK    18
#define TFT_MOSI   23
#define TFT_MISO   19
#define TFT_CS     0
#define TFT_DC     3
#define TFT_RESET  15

// GUI Default Values
#define TEXT_SIZE         4
#define BACKGROUND_COLOUR BLACK
#define FONT_COLOUR       ORANGE
#define START_COLUMN      20
#define HORIZONTAL        3
#define COLUMN_OFFSET     180

// Rows for Information
#define RPM_ROW           20
#define MPH_ROW           60
#define GEAR_ROW          100
#define FUEL_ROW          140

// LEDs
#define LED_PIN       4
#define NUM_LEDS      30
#define BRIGHTNESS    64
#define SHIFT_DELAY   1
#define MAX_SHIFT_RPM 3000
#define COLOR_ORDER   GRB
#define LED_TYPE      WS2811

// GPS
#define TXPIN       35
#define RXPIN       34
#define GPSBAUD     9600
#define GPSCPUTIME  20

// SD Card
#define SDCS 5

// CAN BUS
#define MCPCS         2
#define CANBUSCPUTIME 20

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

#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "ui.h"

//----------------
// Objects
//----------------

// Display
Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);

// For updating display data
int rpm_old_value = -1;
int tps_old_value = -1;
int water_temp_old_value = -1;
int kph_old_value = -1;
int gear_old_value = -1;
int mph_old_value = -1;
int oil_temp_old_value = -1;
int battery_voltage_old_value = -1;
int num_satellites_old_value = -1;

// 3 Axis Gyro
Adafruit_MPU6050 mpu;

// GPS and Compass
TinyGPSPlus gps;                        // The TinyGPS++ object
SoftwareSerial gpsSerial(TXPIN, RXPIN); // The serial interface to the GPS device
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);

// LEDs
int rpmLightInterval = MAX_SHIFT_RPM / NUM_LEDS;
CRGB leds[NUM_LEDS];

// RPM Lights
boolean rpmState = true; // Simulation

// SD Card (Write)
String outputString;

// CAN BUS
struct can_frame canMsg;
MCP2515 mcp2515(MCPCS);

// CAN BUS Data to present on display

// Packet 2000
int rpm = 0;             // [0] RPM
int tps = 0;             // [1] Throttle Position Sensor
int water_temp = 0;      // [2] Water Temperature

// Packet 2001
int kph = 0;             // [2] Speed reported by ECU

// Packet 2002
int oil_temp = 0;        // [1] Oil Temperature
int battery_voltage = 0; // [2] Battery Voltage

// Packet 2003
int gear = 0;            // [0] Gear

// Other data for display
// int bps = 0;             // Brake Position Sensor - Not currently implemented
// int gforce = 0;          // GForce                - Not currently implemented
int num_satellites = 0;  // Number of Satellites
int mph = 0;             // Miles per hour

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
  display.setCursor(START_COLUMN, RPM_ROW);
  display.print("RPM:");
  display.setCursor(START_COLUMN, MPH_ROW);
  display.print("MPH:");
  display.setCursor(START_COLUMN, GEAR_ROW);
  display.print("Gear:");
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
    mph = gps.speed.mph();
    outputString = "Speed (Mph): " + String(mph) + "\n";
    outputString += "Lat: " + String(gps.location.lat(), 7)  + " Long: " + String(gps.location.lng(), 7) + "\n";
    outputString += "Deg: " + String(gps.course.deg()) + "\n";
    outputString += "Heading: " + String(gps.cardinal(gps.course.value())) + "\n";
    outputString += "Altitude (Miles): " + String(gps.altitude.miles()) + "\n";
  }
  if (gps.satellites.isValid()) {
    num_satellites = gps.satellites.value();
    outputString += "Number of Satellite: " + String(num_satellites) + "\n";
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

  for (int start = millis(); millis() - start < GPSCPUTIME; ) {
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

void get_can_bus_data() {
  for (int start = millis(); millis() - start < CANBUSCPUTIME; ) {
    while (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

      outputString = "CAN Message ID: " + String(canMsg.can_id, HEX)  + " Message Length: " + String(canMsg.can_dlc, HEX) + " Data: ";

      if (canMsg.can_id == 0) {
        rpm = canMsg.data[0] * 100;
        tps = canMsg.data[1];
        water_temp = canMsg.data[2];
      }

      if (canMsg.can_id == 1) {
        kph = canMsg.data[2];
      }

      if (canMsg.can_id == 2) {
        oil_temp = canMsg.data[1];
        battery_voltage = canMsg.data[2];
      }

      if(canMsg.can_id == 3) {
        gear = canMsg.data[0];
      }

      for (int i = 0; i < canMsg.can_dlc; i++)  {
        outputString += String(canMsg.data[i], HEX);
        outputString += " ";
      }
      outputString += "\n";

      appendFile(SD, "/can-bus-data/can-bus-data.txt", outputString.c_str());
      Serial.print(outputString);
    }
  }
  Serial.println(F(""));
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
  for (int r = 0; r < 3500; r += 100) {
    setRPMLights(r);
  }
}

void simulateRPMDecrease() {
  for (int r = 3500; r > 0; r -= 100) {
    setRPMLights(r);
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
// Display Functions
//-----------------------------

void set_rpm_label(int rpm_value, bool clear_text) {
  Serial.println("Setting rpm label");
  String text = String(rpm_value);
  if (clear_text) {
    Serial.println("Clearning rpm label");
    display.setTextColor(BLACK);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, RPM_ROW);
    display.print(text);
  } else {
    Serial.println("Setting new rpm label value");
    display.setTextColor(FONT_COLOUR);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, RPM_ROW);
    display.print(text);
  }
}

void set_speed_label(int speed_value, bool clear_text) {
  String text = String(speed_value);
  if (clear_text) {
    display.setTextColor(BLACK);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, MPH_ROW);
    display.print(text);
  } else {
    display.setTextColor(FONT_COLOUR);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, MPH_ROW);
    display.print(text);
  }
}

void set_gear_label(int gear_value, bool clear_text) {
  String gear_text = "";
  if (gear_value == 0) {
    gear_text = "N";
  } else {
    gear_text = String(gear_value);
  }

  if (clear_text) {
    display.setTextColor(BLACK);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, GEAR_ROW);
    display.print(gear_text);
  } else {
    display.setTextColor(FONT_COLOUR);
    display.setCursor(START_COLUMN + COLUMN_OFFSET, GEAR_ROW);
    display.print(gear_text);
  }
}

void update_rpm_display() {
  // Set RPM Light and RPM value on GUI
  if (rpm != rpm_old_value && rpm >= 0) {
    set_rpm_label(rpm_old_value, true);
    set_rpm_label(rpm, false);
    setRPMLights(rpm);
    rpm_old_value = rpm;
  }
}

void update_mph_display() {
  // Set MPH value on GUI
  if (mph != mph_old_value && mph >= 0) {
    set_speed_label(mph_old_value, true);
    set_speed_label(mph, false);
    mph_old_value = mph;
  }
}

void update_gear_display() {
  if(gear != gear_old_value && gear >= 0) {
    set_gear_label(gear_old_value, true);
    set_gear_label(gear, false);
    gear_old_value = gear;
  }
}

void update_display_data() {
  update_rpm_display();
  update_mph_display();
  update_gear_display();
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

/// 独自の設定を行うクラスを、LGFX_Deviceから派生して作成します。
class LGFX : public lgfx::LGFX_Device
{
/*
 クラス名は"LGFX"から別の名前に変更しても構いません。
 AUTODETECTと併用する場合は"LGFX"は使用されているため、LGFX以外の名前に変更してください。
 また、複数枚のパネルを同時使用する場合もそれぞれに異なる名前を付けてください。
 ※ クラス名を変更する場合はコンストラクタの名前も併せて同じ名前に変更が必要です。

 名前の付け方は自由に決めて構いませんが、設定が増えた場合を想定し、
 例えばESP32 DevKit-CでSPI接続のILI9341の設定を行った場合、
  LGFX_DevKitC_SPI_ILI9341
 のような名前にし、ファイル名とクラス名を一致させておくことで、利用時に迷いにくくなります。
//*/


// 接続するパネルの型にあったインスタンスを用意します。

  lgfx::Panel_ILI9341     _panel_instance;


// パネルを接続するバスの種類にあったインスタンスを用意します。
  lgfx::Bus_SPI       _bus_instance;   // SPIバスのインスタンス
// バックライト制御が可能な場合はインスタンスを用意します。(必要なければ削除)
  lgfx::Light_PWM     _light_instance;

// タッチスクリーンの型にあったインスタンスを用意します。(必要なければ削除)
//lgfx::Touch_FT5x06           _touch_instance; // FT5206, FT5306, FT5406, FT6206, FT6236, FT6336, FT6436
//lgfx::Touch_GSL1680E_800x480 _touch_instance; // GSL_1680E, 1688E, 2681B, 2682B
//lgfx::Touch_GSL1680F_800x480 _touch_instance;
//lgfx::Touch_GSL1680F_480x272 _touch_instance;
//lgfx::Touch_GSLx680_320x320  _touch_instance;
//lgfx::Touch_GT911            _touch_instance;
//lgfx::Touch_STMPE610         _touch_instance;
//lgfx::Touch_TT21xxx          _touch_instance; // TT21100
lgfx::Touch_XPT2046          _touch_instance;

public:

  // コンストラクタを作成し、ここで各種設定を行います。
  // クラス名を変更した場合はコンストラクタも同じ名前を指定してください。
  LGFX(void)
  {
    { // バス制御の設定を行います。
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。

// SPIバスの設定
      cfg.spi_host = VSPI_HOST;     // 使用するSPIを選択  ESP32-S2,C3 : SPI2_HOST or SPI3_HOST / ESP32 : VSPI_HOST or HSPI_HOST
      // ※ ESP-IDFバージョンアップに伴い、VSPI_HOST , HSPI_HOSTの記述は非推奨になるため、エラーが出る場合は代わりにSPI2_HOST , SPI3_HOSTを使用してください。
      cfg.spi_mode = 0;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 80000000;    // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.freq_read  = 16000000;    // 受信時のSPIクロック
      cfg.spi_3wire  = false;        // 受信をMOSIピンで行う場合はtrueを設定
      cfg.use_lock   = true;        // トランザクションロックを使用する場合はtrueを設定
      //cfg.dma_channel = SPI_DMA_CH_AUTO; // 使用するDMAチャンネルを設定 (0=DMA不使用 / 1=1ch / 2=ch / SPI_DMA_CH_AUTO=自動設定)
      cfg.dma_channel = 1;
      // ※ ESP-IDFバージョンアップに伴い、DMAチャンネルはSPI_DMA_CH_AUTO(自動設定)が推奨になりました。1ch,2chの指定は非推奨になります。
      cfg.pin_sclk = 18;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 23;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = 19;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = 3;            // SPIのD/Cピン番号を設定  (-1 = disable)
     // SDカードと共通のSPIバスを使う場合、MISOは省略せず必ず設定してください。
//*/
/*
// I2Cバスの設定
      cfg.i2c_port    = 0;          // 使用するI2Cポートを選択 (0 or 1)
      cfg.freq_write  = 400000;     // 送信時のクロック
      cfg.freq_read   = 400000;     // 受信時のクロック
      cfg.pin_sda     = 21;         // SDAを接続しているピン番号
      cfg.pin_scl     = 22;         // SCLを接続しているピン番号
      cfg.i2c_addr    = 0x3C;       // I2Cデバイスのアドレス
//*/
/*
// 8ビットパラレルバスの設定
      cfg.i2s_port = I2S_NUM_0;     // 使用するI2Sポートを選択 (I2S_NUM_0 or I2S_NUM_1) (ESP32のI2S LCDモードを使用します)
      cfg.freq_write = 20000000;    // 送信クロック (最大20MHz, 80MHzを整数で割った値に丸められます)
      cfg.pin_wr =  4;              // WR を接続しているピン番号
      cfg.pin_rd =  2;              // RD を接続しているピン番号
      cfg.pin_rs = 15;              // RS(D/C)を接続しているピン番号
      cfg.pin_d0 = 12;              // D0を接続しているピン番号
      cfg.pin_d1 = 13;              // D1を接続しているピン番号
      cfg.pin_d2 = 26;              // D2を接続しているピン番号
      cfg.pin_d3 = 25;              // D3を接続しているピン番号
      cfg.pin_d4 = 17;              // D4を接続しているピン番号
      cfg.pin_d5 = 16;              // D5を接続しているピン番号
      cfg.pin_d6 = 27;              // D6を接続しているピン番号
      cfg.pin_d7 = 14;              // D7を接続しているピン番号
//*/

      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。

      cfg.pin_cs           =    0;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    17;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.pin_busy         =    -1;  // BUSYが接続されているピン番号 (-1 = disable)

      // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。

      // 以下はST7735やILI9163のようにピクセル数が可変のドライバで表示がずれる場合にのみ設定してください。
      cfg.memory_width     =   240;  // ドライバICがサポートしている最大の幅
      cfg.memory_height    =   320;  // ドライバICがサポートしている最大の高さ

      cfg.panel_width      =   240;  // 実際に表示可能な幅
      cfg.panel_height     =   320;  // 実際に表示可能な高さ
      // cfg.offset_x         =   240;  // パネルのX方向オフセット量
      // cfg.offset_y         =   320;  // パネルのY方向オフセット量
      cfg.offset_x         =   0;  // パネルのX方向オフセット量
      cfg.offset_y         =   0;  // パネルのY方向オフセット量
      cfg.offset_rotation  =     0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
      cfg.dummy_read_pixel =     8;  // ピクセル読出し前のダミーリードのビット数
      cfg.dummy_read_bits  =     1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
      cfg.readable         =  true;  // データ読出しが可能な場合 trueに設定
      cfg.invert           = false;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      cfg.dlen_16bit       = false;  // 16bitパラレルやSPIでデータ長を16bit単位で送信するパネルの場合 trueに設定
      cfg.bus_shared       =  true;  // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)


      _panel_instance.config(cfg);
    }

//*
    { // バックライト制御の設定を行います。（必要なければ削除）
      auto cfg = _light_instance.config();    // バックライト設定用の構造体を取得します。

      cfg.pin_bl = 21;              // バックライトが接続されているピン番号
      cfg.invert = false;           // バックライトの輝度を反転させる場合 true
      cfg.freq   = 44100;           // バックライトのPWM周波数
      cfg.pwm_channel = 7;          // 使用するPWMのチャンネル番号

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);  // バックライトをパネルにセットします。
    }
//*/

//*
    { // タッチスクリーン制御の設定を行います。（必要なければ削除）
      auto cfg = _touch_instance.config();

      cfg.x_min      = 0;    // タッチスクリーンから得られる最小のX値(生の値)
      cfg.x_max      = 239;  // タッチスクリーンから得られる最大のX値(生の値)
      cfg.y_min      = 0;    // タッチスクリーンから得られる最小のY値(生の値)
      cfg.y_max      = 319;  // タッチスクリーンから得られる最大のY値(生の値)
      cfg.pin_int    = -1;   // INTが接続されているピン番号
      cfg.bus_shared = true; // 画面と共通のバスを使用している場合 trueを設定
      cfg.offset_rotation = 0;// 表示とタッチの向きのが一致しない場合の調整 0~7の値で設定

// SPI接続の場合
      cfg.spi_host = VSPI_HOST;// 使用するSPIを選択 (HSPI_HOST or VSPI_HOST)
      cfg.freq = 1000000;     // SPIクロックを設定
      cfg.pin_sclk = 18;     // SCLKが接続されているピン番号
      cfg.pin_mosi = 23;     // MOSIが接続されているピン番号
      cfg.pin_miso = 19;     // MISOが接続されているピン番号
      cfg.pin_cs   = 0;     //   CSが接続されているピン番号

// I2C接続の場合
      //cfg.i2c_port = 1;      // 使用するI2Cを選択 (0 or 1)
      //cfg.i2c_addr = 0x38;   // I2Cデバイスアドレス番号
      //cfg.pin_sda  = 23;     // SDAが接続されているピン番号
      //cfg.pin_scl  = 32;     // SCLが接続されているピン番号
      //cfg.freq = 400000;     // I2Cクロックを設定

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);  // タッチスクリーンをパネルにセットします。
    }
//*/

    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};



static LGFX tft;

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp_drv );
}

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data )
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch( &touchX, &touchY, 600 );

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
    }
}

void setup(void)
{

  Serial.begin( 115200 ); /* prepare for possible serial debug */
  Serial.println(F("\nSetting up Dashboard"));
  setup_can_bus();
  setup_sd_card();
  setup_three_axis_gyro();
  setup_compass();
  setup_gps();
  setup_leds();
  setRPMLights(0);
  Serial.println(F("Dashboard Setup Complete\n"));
  
  String LVGL_Arduino = "LVGL Arduino ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  
  Serial.println( LVGL_Arduino );
  tft.init();
  tft.setRotation(1);
  tft.setBrightness(255);
  lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();
  //ui_reset();

  xTaskCreatePinnedToCore(simulation_task,
                          "simulation_task",
                          4000,
                          NULL,
                          0,
                          NULL,
                          1);

  
}



void simulation_task(void *pvParameters) {

  lv_bar_set_value(ui_loadingBar, 0, LV_ANIM_OFF);

  delay(2000);

  int count_value = 0;

  for(int i=0;i<100;i++){
    delay(50);
    lv_bar_set_value(ui_loadingBar, count_value, LV_ANIM_OFF);
    count_value++;
  }

  //lv_scr_load(ui_Screen3);

  lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, true);

  delay(30000);
  
  ESP.restart();

  while (1) {

    vTaskDelay(10);

  }
}


int speed_value = 0;
int g_force = 0;

void loop(void)
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(1);
  
  // Elements on Display 
  
  // RPM
  rpm += 100; 
  if (rpm > 3000) {
    // Increase Gear
    rpm = 1500;

    // Gear
    gear += 1;
    if (gear > 5){
      gear = 1;
    }
  }

  
  // Speed 
  speed_value += 5;
  if(speed_value > 30) {
    speed_value = 0;
  }
  

  // TODO: Add these later
  // TPS
  // BPS
  
  // G-Force 
  g_force += 1;
  if (g_force > 5) {
    g_force = 0;
  }


  // Set Labels 
  // RPM
  lv_label_set_text(ui_LabelRPM, String(rpm).c_str());
  
  // Gear
  lv_label_set_text(ui_LabelGear, String(gear).c_str());
  
  // Speed
  lv_label_set_text(ui_LabelSpeed, String(speed_value).c_str());
  
  // G-Force
  lv_label_set_text(ui_LabelGForce, String(g_force).c_str());

  delay(500);
  
  
}
