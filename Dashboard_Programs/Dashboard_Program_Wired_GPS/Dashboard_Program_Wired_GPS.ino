#define LGFX_USE_V1

#include <Adafruit_HMC5883_U.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino_GFX_Library.h>
#include <Definitions.h>
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

class LGFX : public lgfx::LGFX_Device
{

    lgfx::Panel_ILI9341     _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;
    lgfx::Touch_XPT2046          _touch_instance;

  public:

    LGFX(void)
    {
      {
        auto cfg = _bus_instance.config();

        cfg.spi_host = VSPI_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 80000000;
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = false;
        cfg.use_lock   = true;
        cfg.dma_channel = 1;
        cfg.pin_sclk = 18;
        cfg.pin_mosi = 23;
        cfg.pin_miso = 19;
        cfg.pin_dc   = 3;

        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
      }

      {
        auto cfg = _panel_instance.config();

        cfg.pin_cs           =    0;
        cfg.pin_rst          =    17;
        cfg.pin_busy         =    -1;

        cfg.memory_width     =   240;
        cfg.memory_height    =   320;

        cfg.panel_width      =   240;
        cfg.panel_height     =   320;
        cfg.offset_x         =   0;
        cfg.offset_y         =   0;
        cfg.offset_rotation  =     0;
        cfg.dummy_read_pixel =     8;
        cfg.dummy_read_bits  =     1;
        cfg.readable         =  true;
        cfg.invert           = false;
        cfg.rgb_order        = false;
        cfg.dlen_16bit       = false;
        cfg.bus_shared       =  true;

        _panel_instance.config(cfg);
      }


      {
        auto cfg = _light_instance.config();

        cfg.pin_bl = 21;
        cfg.invert = false;
        cfg.freq   = 44100;
        cfg.pwm_channel = 7;

        _light_instance.config(cfg);
        _panel_instance.setLight(&_light_instance);
      }

      {
        auto cfg = _touch_instance.config();

        cfg.x_min      = 0;
        cfg.x_max      = 239;
        cfg.y_min      = 0;
        cfg.y_max      = 319;
        cfg.pin_int    = -1;
        cfg.bus_shared = true;
        cfg.offset_rotation = 0;

        cfg.spi_host = VSPI_HOST;
        cfg.freq = 1000000;
        cfg.pin_sclk = 18;
        cfg.pin_mosi = 23;
        cfg.pin_miso = 19;
        cfg.pin_cs   = 0;

        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
      }
      setPanel(&_panel_instance);
    }
};

//----------------
// Objects
//----------------

// Display
Arduino_ESP32SPI bus = Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_ILI9341 display = Arduino_ILI9341(&bus, TFT_RESET);
static LGFX tft;

/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ screenWidth * screenHeight / 10 ];

// 3 Axis Gyro
Adafruit_MPU6050 mpu;

// GPS and Compass
TinyGPSPlus gps;                        // The TinyGPS++ object
SoftwareSerial gpsSerial(TXPIN, RXPIN); // The serial interface to the GPS device
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345); // Magnetometer

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

// Packet 2004
int bps = 0;             // [0] Brake Position Sensor

// Other data for display
int g_force = 0;         // G-Force
int num_satellites = 0;  // Number of Satellites
int mph = 0;             // Miles per hour

// Hold old values of readings
int rpm_old_value = -1;
int tps_old_value = -1;
int bps_old_value = -1;
int g_force_old_value = -1;
int water_temp_old_value = -1;
int kph_old_value = -1;
int gear_old_value = -1;
int mph_old_value = -1;
int oil_temp_old_value = -1;
int battery_voltage_old_value = -1;
int num_satellites_old_value = -1;

//----------------
// Setup Functions
//----------------

void setup_three_axis_gyro(void) {
  while (!Serial)
    delay(10); // will pause until serial console opens

  Serial.println(F("Setting up MPU6050"));

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

  if (!mag.begin()) {
    Serial.println(F("No HMC5883 detected"));
    while (1);
  }
}

void setup_leds() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
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

  check_and_create_directory("gps-data", "GPS");
  check_and_create_directory("compass-data", "Compass");
  check_and_create_directory("mpu-data", "MPU");
  check_and_create_directory("can-bus-data", "CAN BUS");

  Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
  Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}

void check_and_create_directory(String directory, String module) {
  if (!SD.exists(("/" + directory + "/" + directory + ".txt").c_str())) {
    createDir(SD, ("/" + directory).c_str());
    Serial.println(("Creating " + module + " File").c_str());
    writeFile(SD, ("/" + directory + "/" + directory + ".txt").c_str(), ("Start of " + module + "\n").c_str());
  } else {
    Serial.println((module + " File Exists").c_str());
    appendFile(SD, ("/" + directory + "/" + directory + ".txt").c_str(), ("Start of New " + module + " Data\n").c_str());
  }
}

void setup_can_bus() {
  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Set CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                   // Set CAN at normal mode
}

void setup_display() {
  String LVGL_Arduino = "LVGL Arduino ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);
  tft.init();
  tft.setRotation(1);
  tft.setBrightness(255);
  lv_init();

  #if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
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
  // ui_reset();

  lv_bar_set_value(ui_loadingBar, 0, LV_ANIM_OFF);

  delay(2000);

  for (int i = 0; i < 100; i++) {
    delay(50);
    lv_bar_set_value(ui_loadingBar, i, LV_ANIM_OFF);
  }

  lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, true);
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

  g_force = sqrt(sq(g.gyro.x) + sq(g.gyro.y) + sq(g.gyro.z));

  appendFile(SD, "/mpu-data/mpu-data.txt", outputString.c_str());
  Serial.println("---       3-Axis Gyro Data    ---");
  Serial.println(outputString);
}

void get_gps_data() {
  Serial.println("---         GPS Data          ---");
  for (int start = millis(); millis() - start < GPSCPUTIME; ) {
    while (gpsSerial.available()) {
      if (gps.encode(gpsSerial.read())) {
        outputString = "";
        if (gps.location.isValid()) {
          mph = gps.speed.mph();
          outputString += "Speed (Mph): " + String(mph) + "\n";
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
        break;
      }
    }
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
  
  Serial.println("---       Compass Data        ---");
  Serial.println(outputString);
}

void get_can_bus_data() {

  Serial.println("---       CAN BUS Data        ---");
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

      if (canMsg.can_id == 3) {
        gear = canMsg.data[0];
      }

      if (canMsg.can_id == 4) {
        bps = canMsg.data[0];
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
  for (int i = 0; i < NUM_LEDS; i++) {
    if (rpmValue >= (i + 1)*rpmLightInterval) {
      if (i < 10) {               // LEDs should be Green
        leds[i].setRGB(0, BRIGHTNESS, 0);
      } else if (i < 20) {        // LEDs should be Red
        leds[i].setRGB(BRIGHTNESS, 0, 0);
      } else if (i < 30) {        // LEDs should be Blue
        leds[i].setRGB(0, 0, BRIGHTNESS);
      }
      FastLED.show();
    } else {
      leds[i].setRGB(0, 0, 0);
      FastLED.show();
    }
  }
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

//-----------------------------
// Debug Logging
//-----------------------------

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buf)
{
  Serial.printf(buf);
  Serial.flush();
}
#endif

void report_loop_duration(long start) {
  long duration = micros() - start;
  Serial.print("Loop cycle time: ");
  Serial.print(duration / 1000.0);
  Serial.println(" ms\n");
}

//-----------------------------
// Display Related
//-----------------------------

void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t * )&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready( disp_drv );
}

void my_touchpad_read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data )
{
  uint16_t touchX, touchY;

  bool touched = tft.getTouch( &touchX, &touchY, 600 );

  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
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

bool update_display_label(int &current_value, int &old_value) {
  if (current_value != old_value && current_value >= 0) {
    old_value = current_value;
    return true;
  }
  return false;
}

void update_display_data() {

  if (update_display_label(rpm, rpm_old_value)) {
    lv_label_set_text(ui_LabelRPM, String(rpm).c_str());
    lv_bar_set_value(ui_BarRPM, rpm / 30, LV_ANIM_OFF);
    setRPMLights(rpm);
  }

  if (update_display_label(gear, gear_old_value)) {
    lv_label_set_text(ui_LabelGear, String(gear).c_str());
  }

  if (update_display_label(mph, mph_old_value)) {
    lv_label_set_text(ui_LabelSpeed, String(mph).c_str());
  }

  if (update_display_label(g_force, g_force_old_value)) {
    lv_label_set_text(ui_LabelGForce, String(g_force).c_str());
  }

  if (update_display_label(tps, tps_old_value)) {
    lv_bar_set_value(ui_BarTPS, tps, LV_ANIM_OFF);
  }

  if (update_display_label(bps, bps_old_value)) {
    lv_bar_set_value(ui_BarBPS, bps, LV_ANIM_OFF);
  }

}

//-----------------------------
// Setup
//-----------------------------

void setup(void) {
  Serial.begin(115200);
  Serial.println(F("\nSetting up Dashboard"));
  setup_can_bus();
  setup_sd_card();
  setup_three_axis_gyro();
  setup_compass();
  setup_gps();
  setup_leds();
  setRPMLights(0);
  setup_display();
  Serial.println(F("Dashboard Setup Complete\n"));



  Serial.println("Finished Setup Function");
}

//-----------------------------
// Loop
//-----------------------------

void loop(void) {
  long start = micros();
  lv_timer_handler();
  get_gps_data();
  get_compass_data();
  get_three_axis_gyro_data();
  get_can_bus_data();
  update_display_data();
  report_loop_duration(start);
}
