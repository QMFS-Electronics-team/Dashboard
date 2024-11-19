#include <FS.h>
#include <FastLED.h>
#include <LovyanGFX.hpp>
#include <NimBLEDevice.h>
#include <ProductionDefinitions.h>
#include <SD.h>
#include <SPI.h>
#include <lvgl.h>
#include <math.h>
#include <mcp2515.h>
#include "ui.h"


int LEDBrightness = 20;
int rpmLightInterval = 9000 / NUM_RPM_LEDS;

struct can_frame canMsg;
struct can_frame canReqMsg;
MCP2515 mcp2515(CANBUS_CS_PIN);

CRGBArray<NUM_RPM_LEDS> leds;

SemaphoreHandle_t gui_mutex;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10];

NimBLEClient *pClient = nullptr;

// SPI
SPIClass spi = SPIClass(HSPI);

// BLE UUIDs
static BLEUUID UART_service_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID TX_characteristic_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

// Configuration
const int outputFrequencyHzSerial = 8; // Hz
const unsigned long outputIntervalMs_serial = 1000 / outputFrequencyHzSerial;

static bool doConnect = false;
static bool bleRequestDisconnect = false;
static bool connected = false;
static bool doScan = false;
static bool updated_RaceBox_Data_Message = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myRaceBox;

unsigned long lastOutputTimeSerial = 0;
unsigned long lastOutputTimeOLED = 0;

// global variables for live data from RaceBox (at 25Hz): (examples see function void parsePayload)
uint16_t header;
uint8_t messageClass;
uint8_t messageId;
uint16_t payloadLength;
uint32_t iTOW;
uint16_t year;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;
uint8_t second;
uint8_t validityFlags;
uint32_t timeAccuracy;
uint32_t nanoseconds;
uint8_t fixStatus;
uint8_t fixStatusFlags;
uint8_t dateTimeFlags;
uint8_t numSVs;
int32_t longitude;
int32_t latitude;
int32_t wgsAltitude;
int32_t mslAltitude;
uint32_t horizontalAccuracy;
uint32_t verticalAccuracy;
uint32_t speed;
uint32_t heading;
uint32_t speedAccuracy;
uint32_t headingAccuracy;
uint16_t pdop;
uint8_t latLonFlags;
uint8_t batteryStatus;
int16_t gForceX;
int16_t gForceY;
int16_t gForceZ;
int16_t rotRateX;
int16_t rotRateY;
int16_t rotRateZ;

int rpm;

float headingDegrees;
String compass_direction;

class LGFX : public lgfx::LGFX_Device {

  lgfx::Panel_ILI9488 _panel_instance;
  lgfx::Bus_Parallel8 _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_FT5x06  _touch_instance;

public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.freq_write = 20000000; // 20Mhz Max. Increasing write frequency generates display glitches
      cfg.pin_wr = 48;
      cfg.pin_rd = 47;
      cfg.pin_rs = 45; // DC Pin
      cfg.pin_d0 = 14;
      cfg.pin_d1 = 13;
      cfg.pin_d2 = 12;
      cfg.pin_d3 = 11;
      cfg.pin_d4 = 10;
      cfg.pin_d5 = 9;
      cfg.pin_d6 = 3;
      cfg.pin_d7 = 8;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();

      cfg.pin_cs = 38;
      cfg.pin_rst = 21;
      cfg.pin_busy = -1;

      cfg.memory_width = 320;
      cfg.memory_height = 480;

      cfg.panel_width = 320;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = true;
      cfg.rgb_order = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();

      cfg.pin_bl = 6;
      cfg.invert = false;
      cfg.freq = 2000; // set a slower frequency for LED driver CE
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    {
      auto cfg = _touch_instance.config();

      cfg.x_min = 0;
      cfg.x_max = 320;
      cfg.y_min = 0;
      cfg.y_max = 480;
      cfg.pin_int = -1;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;

      cfg.i2c_port = 1;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda = 36;
      cfg.pin_scl = 37;
      cfg.freq = 100000;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

static LGFX tft;

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf)
{
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp_drv);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {

  uint16_t x, y;
  if (tft.getTouch(&x, &y))   {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}


//-----------------------------
// RGB LEDS
//-----------------------------

void upshifting_blink() {
  leds[0] = CRGB::Red;
  leds[1] = CRGB::Red;
  leds[2] = CRGB::Red;
  leds[3] = CRGB::Red;
  leds[4] = CRGB::Red;
  leds[5] = CRGB::Red;
  leds[6] = CRGB::Red;
  leds[7] = CRGB::Red;
  leds[8] = CRGB::Red;
  leds[9] = CRGB::Red;
  FastLED.show();
  delay(200);
  leds[0] = CRGB::Black;
  leds[1] = CRGB::Black;
  leds[2] = CRGB::Black;
  leds[3] = CRGB::Black;
  leds[4] = CRGB::Black;
  leds[5] = CRGB::Black;
  leds[6] = CRGB::Black;
  leds[7] = CRGB::Black;
  leds[8] = CRGB::Black;
  leds[9] = CRGB::Black;
  FastLED.show();
  delay(200);
}

void RGB_startup_animation() {

  // reset
  leds[0] = CRGB::Black;
  leds[1] = CRGB::Black;
  leds[2] = CRGB::Black;
  leds[3] = CRGB::Black;
  leds[4] = CRGB::Black;
  leds[5] = CRGB::Black;
  leds[6] = CRGB::Black;
  leds[7] = CRGB::Black;
  leds[8] = CRGB::Black;
  leds[9] = CRGB::Black;
  FastLED.show();

  delay(250);
  leds[0] = CRGB::Green;
  leds[9] = CRGB::Green;
  FastLED.show();
  delay(250);
  leds[1] = CRGB::Green;
  leds[8] = CRGB::Green;
  FastLED.show();
  delay(250);
  leds[2] = CRGB::Orange;
  leds[7] = CRGB::Orange;
  FastLED.show();
  delay(250);
  leds[3] = CRGB::Red;
  leds[6] = CRGB::Red;
  FastLED.show();
  delay(250);
  leds[4] = CRGB::Red;
  leds[5] = CRGB::Red;
  FastLED.show();
  delay(200);

  upshifting_blink();
  upshifting_blink();

  leds[0] = CRGB::Green;
  leds[1] = CRGB::Green;
  leds[2] = CRGB::Orange;
  leds[3] = CRGB::Red;
  leds[4] = CRGB::Red;
  leds[5] = CRGB::Red;
  leds[6] = CRGB::Red;
  leds[7] = CRGB::Orange;
  leds[8] = CRGB::Green;
  leds[9] = CRGB::Green;
  FastLED.show();
}


//-----------------------------
// Buzzer
//-----------------------------

void buzz_double() {
  for(int i = 0; i < 2; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

void demo_rpm_lights(void *pvParameters) {
  while(1) {
    for(int i = 0; i < 13; i++) {
      rpm = i * 1000;
      setRPMLights(rpm);
      delay(250);
    }
  }
}


//-----------------------------
// Dispay
//-----------------------------

void display_task(void *pvParameters) {

  String LVGL_Arduino = "LVGL Arduino ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println(LVGL_Arduino);

  tft.init();
  tft.setRotation(1);
  tft.setBrightness(TFT_DEFAULT_BRIGHTNESS);

  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  /*Change the following line to your display resolution*/
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
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
  ui_reset();

  // assign callback functions

  // restart button
  lv_obj_add_event_cb(ui_SettingScreen_Button_ButtonRestart, ui_event_SettingScreen_Button_ButtonRestart, LV_EVENT_PRESSED, NULL);
  // BLE disconnect button
  lv_obj_add_event_cb(ui_SettingScreen_Button_ButtonBLEDisconnect, ui_event_SettingScreen_Button_ButtonBLEDisconnect, LV_EVENT_PRESSED, NULL);
  // LED brightness
  lv_obj_add_event_cb(ui_SettingScreen_Slider_SliderLEDBrightness, ui_event_SettingScreen_Slider_SliderLEDBrightness, LV_EVENT_VALUE_CHANGED, NULL);
  // Disp brightness
  lv_obj_add_event_cb(ui_SettingScreen_Slider_SliderDisplayBrightness, ui_event_SettingScreen_Slider_SliderDisplayBrightness, LV_EVENT_VALUE_CHANGED, NULL);

  // Main LVGL loop
  while (1) {

    // Take the semaphore to access LVGL resources
    if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
      // Call LVGL's main task handler
      lv_timer_handler();

      // Release the semaphore after LVGL operations
      xSemaphoreGive(gui_mutex);
    }

    // Delay to control LVGL's refresh rate
    vTaskDelay(pdMS_TO_TICKS(1)); // Adjust as needed
  }
}

void display_update_task(void *pvParameters) {

  delay(500); // wait for display init

  // Screen is initiated with the loading screen first

  lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, 0, LV_ANIM_OFF);

  

  for (int i = 0; i < 100; i++) {
    delay(15);
    lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, i, LV_ANIM_OFF);
  }

  lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Connecting..");

  lv_scr_load(ui_MainScreen);

  float gX = 0.0;
  float gY = 0.0;
  float gZ = 0.0;
  float g_mag = 0.0;

  setRPMLights(0);

  while (1) {
    // if racebox connected
    if (connected) {

      gX = gForceX / 1000.0;
      gY = gForceY / 1000.0;
      gZ = gForceZ / 1000.0;
      g_mag = sqrt(gX * gX + gY * gY + gZ * gZ);

      lv_label_set_text_fmt(ui_MainScreen_Label_LabelSpeed, "%.0f", (speed / 1000.0) * 2.23694); // conversion to m/s to mph
      lv_label_set_text_fmt(ui_MainScreen_Label_LabelGPSTrack, "GPS Fix: %i", numSVs);           // no. of connected satelites
      lv_label_set_text(ui_MainScreen_Label_LabelGForce, String(gX, 1).c_str());                 // G force resultant
    } else {

      if (bleRequestDisconnect) {
        lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Disconnected");
      } else {
        lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Connecting..");
      }
    }

    lv_bar_set_value(ui_MainScreen_Bar_BarRPM, map(rpm, 0, 12000, 0, 100), LV_ANIM_OFF); // update rpm bar
    vTaskDelay(10);
  }
}


//-----------------------------
// CAN BUS
//-----------------------------

void sensor_task(void *pvParameters) {

  const uint8_t canbus_data[CANBUS_DATA_COUNT] = {PID_ENGINE_RPM, PID_THROTTLE, PID_COOLANT_TEMP, PID_ENGINE_OIL_TEMP, PID_TRANSMISSION_ACTUAL_GEAR, PID_CONTROL_MODULE_VOLTAGE};
  unsigned long currentMillis = millis();
  unsigned long startMillis = 0;
  const unsigned long canbus_timeout_period = 10; // the value is a number of milliseconds
  uint8_t canbus_data_counter = 0;                // counter to cycle through canbus data
  uint8_t send_rq = 0;
  uint8_t send_rq_timeout = 0;
  uint16_t rpm_byte = 0;
  uint16_t rpm_decoded = 0;
  uint8_t throttle_decoded = 0;
  int coolant_temp_decoded = 0;
  int oil_temp_decoded = 0;
  uint16_t battery_byte = 0;
  float battery_decoded = 0;
  uint16_t transmission_actual_gear_byte = 0;
  float transmission_actual_gear_decoded = 0;

  canReqMsg.can_id = 0x7E0;
  canReqMsg.can_dlc = 8;    // Data len
  canReqMsg.data[0] = 0x02; // No. of additional data byte
  canReqMsg.data[1] = 0x01; // Service
  canReqMsg.data[2] = 0x0C; // PID CODE
  canReqMsg.data[3] = 0xCC; // ISO 15765-2 suggests CCh
  canReqMsg.data[4] = 0xCC; // ISO 15765-2 suggests CCh
  canReqMsg.data[5] = 0xCC; // ISO 15765-2 suggests CCh
  canReqMsg.data[6] = 0xCC; // ISO 15765-2 suggests CCh
  canReqMsg.data[7] = 0xCC; // ISO 15765-2 suggests CCh

  delay(1000); // wait for display init

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Set CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                   // Set CAN at normal mode

  while (1) {

    if (!send_rq || send_rq_timeout) {
      if (canbus_data_counter > CANBUS_DATA_COUNT - 1) {
        canbus_data_counter = 0;
      }

      // set required PID in CAN message
      canReqMsg.data[2] = canbus_data[canbus_data_counter];

      mcp2515.sendMessage(&canReqMsg);

      send_rq_timeout = 0; // reset
      send_rq = 1;         // await recieved data before resend
    }

    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

      if (canMsg.can_id == PID_ECU_RESPONSE) {
        switch (canMsg.data[2]) {
        case PID_ENGINE_RPM:
          rpm_byte = (uint16_t)(canMsg.data[3] << 8) + (canMsg.data[4]);
          rpm_decoded = (rpm_byte / 4);
          setRPMLights(rpm_decoded);
          lv_bar_set_value(ui_MainScreen_Bar_BarRPM, map(rpm_decoded, 0, 12000, 0, 100), LV_ANIM_OFF); // update rpm bar
          lv_label_set_text(ui_MainScreen_Label_LabelRPM, String(rpm_decoded).c_str());                // update rpm label
          break;
        case PID_THROTTLE:
          throttle_decoded = map(canMsg.data[3], 0, 255, 0, 100);
          lv_bar_set_value(ui_MainScreen_Bar_BarTPS, throttle_decoded, LV_ANIM_OFF);
          break;
        case PID_COOLANT_TEMP:
          coolant_temp_decoded = canMsg.data[3] - 40;
          lv_label_set_text_fmt(ui_MainScreen_Label_LabelWaterTemp, "Water: %i C", coolant_temp_decoded);
          break;
        case PID_ENGINE_OIL_TEMP:
          // oil_temp_decoded = canMsg.data[3] - 40;
          // lv_label_set_text_fmt(ui_MainScreen_Label_LabelOilTemp, "Oil: %i C", coolant_temp_decoded);
          break;
        case PID_TRANSMISSION_ACTUAL_GEAR:
          transmission_actual_gear_byte = (uint16_t)(canMsg.data[5] << 8) + (canMsg.data[6]);
          transmission_actual_gear_decoded = (transmission_actual_gear_byte / 1000.0);
          lv_label_set_text_fmt(ui_MainScreen_Label_LabelOilTemp, "Gear: %.2f", transmission_actual_gear_decoded);
          break;
        case PID_CONTROL_MODULE_VOLTAGE:
          battery_byte = (uint16_t)(canMsg.data[3] << 8) + (canMsg.data[4]);
          battery_decoded = (battery_byte / 1000.0);
          lv_label_set_text_fmt(ui_MainScreen_Label_LabelBattV, "Batt: %.1f", battery_decoded);
          break;
        default:
          break;
        }

        Serial.print("CAN Message ID: ");
        Serial.print(canMsg.can_id, HEX); // print ID
        Serial.print(" ");
        Serial.print("Message Length: ");
        Serial.print(canMsg.can_dlc, HEX); // print DLC
        Serial.print(" ");
        Serial.print("Data: ");
        for (int i = 0; i < canMsg.can_dlc; i++) {
          Serial.print(canMsg.data[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
        Serial.print("canbus_data_counter: ");
        Serial.println(canbus_data_counter);
        canbus_data_counter++;
        send_rq = 0; // reset request
      }
      else {
        // timeout resend request
        currentMillis = millis();                                   // get the current "time" (actually the number of milliseconds since the program started)
        if (currentMillis - startMillis >= canbus_timeout_period) { // test whether the period has elapsed
          send_rq_timeout = 1;
          startMillis = currentMillis;
        }
      }
    }
  }
}


//-----------------------------
// UI Functions
//-----------------------------

static void ui_event_SettingScreen_Slider_SliderLEDBrightness(lv_event_t *event)
{
  lv_obj_t *slider = lv_event_get_target(event);
  LEDBrightness = (int)lv_slider_get_value(slider);
  FastLED.setBrightness(map((int)lv_slider_get_value(slider), 0, 100, 0, 255));
  FastLED.show();
}

static void ui_event_SettingScreen_Slider_SliderDisplayBrightness(lv_event_t *event)
{
  lv_obj_t *slider = lv_event_get_target(event);
  tft.setBrightness(map((int)lv_slider_get_value(slider), 0, 100, 5, 255));
}

static void ui_event_SettingScreen_Button_ButtonRestart(lv_event_t *event)
{
  delay(1000);
  ESP.restart();
}

static void ui_event_SettingScreen_Button_ButtonBLEDisconnect(lv_event_t *event) {
  connected = false;
  bleRequestDisconnect = true;

  size_t numClients = NimBLEDevice::getClientListSize();
  if (numClients > 0)
  {
    std::list<NimBLEClient *> *clientList = NimBLEDevice::getClientList();
    for (auto it = clientList->begin(); it != clientList->end(); it++)
    {
      if ((*it)->isConnected())
      {
        (*it)->disconnect();
      }
    }
  }
  NimBLEDevice::deinit();
}

void ui_reset()
{
  lv_label_set_text(ui_MainScreen_Label_LabelRPM, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelGear, "N");
  lv_label_set_text(ui_MainScreen_Label_LabelSpeed, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelBattV, "Batt: USB");
  lv_label_set_text(ui_MainScreen_Label_LabelGForce, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelWaterTemp, "Water: 0 C");
  lv_label_set_text(ui_MainScreen_Label_LabelOilTemp, "Oil: 0 C");
  lv_bar_set_value(ui_MainScreen_Bar_BarTPS, 15, LV_ANIM_OFF);
  lv_bar_set_value(ui_MainScreen_Bar_BarBPS, 15, LV_ANIM_OFF);
}

//----------------
// RGB LEDs
//----------------

void setRPMLights(int rpmValue) {
  for (int i = 0; i < NUM_RPM_LEDS; i++) {
    if (rpmValue >= (i + 1)*rpmLightInterval) {
      if (i < 3) {
        leds[i] = CRGB::Green;
      } else if (i < 7) {
        leds[i] = CRGB:: Red;
      } else if (i < 10) {
        leds[i] = CRGB:: Blue;
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
    Serial.println(F("Data Appneded to File"));
  } else {
    Serial.println(F("Append failed"));
  }
  file.close();
}

void check_and_create_directory(String directory, String module) {
  if (!SD.exists(("/" + directory + "/" + directory + ".csv").c_str())) {
    createDir(SD, ("/" + directory).c_str());
    Serial.println(("Creating " + module + " File").c_str());
        // TODO: Create header
    if(module == "GPS") {
      appendFile(SD, ("/" + directory + "/" + directory + ".csv").c_str(), ("Date,Time-UTC,GPS-Fix,Satellites,Latitude,Longitude,WGS-Altitude,MSL-Altitude,Speed-KPH,Heading,Compass-Direction,G-Force-X,G-Force-Y,G-Force-Z,Rotation-X,Rotation-Y,Rotation-Z\n"));
    } else if(module == "CAN BUS") {
      appendFile(SD, ("/" + directory + "/" + directory + ".csv").c_str(), ("RPM, Gear\n"));
    }
  } else {
    Serial.println((module + " File Exists").c_str());
  }
}

//-----------------------------
// BLE GPS Functions
//-----------------------------

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
      connected = true;
      Serial.println(F("RaceBox Connected!"));
    }

    void onDisconnect(NimBLEClient* pClient) {
      connected = false;
      Serial.println(F("Disconnected from RaceBox!"));
      Serial.println(F("HINT: disconnects can happen if excessive serial output (especially in void parsePayload and functions like void parse_RaceBox_Data_Message_payload etc. delay the code execution."));
      Serial.println(F("Trying to reconnect..."));
      doConnect = true;
    }
};

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      Serial.println(F("Advertised BLE Device found: "));
      // Serial.println(advertisedDevice->toString().c_str());

      if (advertisedDevice->isAdvertisingService(UART_service_UUID)) {
        // Check if the device name starts with "RaceBox"
        std::string deviceName = advertisedDevice->getName();
        if (deviceName.rfind("RaceBox", 0) == 0) { //rfind returns 0 if we find "RaceBox" at the beginning of advertised device name. If you have problems here double check case (other RaceBoxes *could* be named "Racebox" or "racebox")

          // If no specific address is defined, connect to any device whose name starts with "RaceBox"
          Serial.println(F("RaceBox found. TARGET_DEVICE_ADDRESS is not set in code (or commented out ), so we connect to any RaceBox that we find."));
          NimBLEDevice::getScan()->stop();  // Stop scanning
          Serial.println(F("stopped bluetooth scanning."));
          Serial.printf("Connecting to RaceBox with address %s.... \n", advertisedDevice->getAddress().toString().c_str());
          myRaceBox = advertisedDevice;
          doConnect = true;
        }
      }
    }

};

bool connectToRaceBox() {

  // Check if there's an existing client that matches the address
  if (NimBLEDevice::getClientListSize()) {
    pClient = NimBLEDevice::getClientByPeerAddress(myRaceBox->getAddress());
    if (pClient) {
      if (!pClient->connect(myRaceBox)) {
        Serial.println("Failed to reconnect. Retrying...");
        // Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
        return false;
      }
    } else {
      // Create a new client if none matches
      pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(new ClientCallbacks(), false);
      if (!pClient->connect(myRaceBox)) {
        Serial.println("Failed to connect.");
        NimBLEDevice::deleteClient(pClient);
        // Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
        return false;
      }
    }
  } else {
    // Create a new client if there are no existing clients
    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(), false);
    if (!pClient->connect(myRaceBox)) {
      Serial.println("Failed to connect.");
      NimBLEDevice::deleteClient(pClient);
      // Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
      return false;
    }
  }

  // obtain the service and characteristic
  BLERemoteService *pService = pClient->getService(UART_service_UUID);
  if (pService != nullptr)
  {
    pRemoteCharacteristic = pService->getCharacteristic(TX_characteristic_UUID);
    if (pRemoteCharacteristic != nullptr)
    {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      Serial.println("DEBUG: connectToRaceBox() will now return true and exit.");
      return true;
    }
  }

  // if service or characteristic is not found, return false
  // Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
  return false;
}

void print_RaceBox_Data_message_payload_to_serial() {
  // serial print the received data:
  unsigned long currentTime = millis();
  if (currentTime - lastOutputTimeSerial >= outputIntervalMs_serial) { 
    // limits the amount how often we print current values to serial

    // Serial output with correct formatting - HINT: the serial output as well as excessive updating of the OLED will take time and can hinder fast operation (e.g. reading in at 25hz), so output should be limited
    Serial.println();
    Serial.println("--- updated values from RaceBox Data Message available: ---");
    Serial.println("--------------------------------------------------------------------------------------------------");
    
    String output = "";

    // Date
    // Serial.println("iTOW: " + String(iTOW) + " ms");
    Serial.println("Date: " + String(day) + "/" + String(month) + "/" + String(year));
    output += String(day) + "/" + String(month) + "/" + String(year);

    // Time
    char timeString[9];                                          // Buffer to store the formatted time string
    sprintf(timeString, "%02d:%02d:%02d", hour, minute, second); // build a time string that always has the time format 00:00:00
    Serial.println("Time (UTC): " + String(timeString));
    output += ","+String(timeString);

    // GPS Fix
    String fixStatusText;
    if (fixStatus == 0) {
      fixStatusText = "No Fix";
    } else if (fixStatus == 2) {
      fixStatusText = "2D Fix";
    } else if (fixStatus == 3) {
      fixStatusText = "3D Fix";
    } else {
      fixStatusText = "Unknown";
    }
    Serial.println("GPS: " + fixStatusText);
    output += "," + fixStatusText;

    // Number of satellites
    Serial.println("Satellites: " + String(numSVs));
    output += "," + String(numSVs);

    // Lattitude and longitude
    // we need to divide the latitude by 10^7 because the datasheet states that it is transmitted with a factor of 10^7
    Serial.println("Latitude: " + String(latitude / 1e7, 7) + " deg, " + "Longitude: " + String(longitude / 1e7, 7) + " deg"); 
    output += "," + String(latitude / 1e7, 7)  + "," + String(longitude / 1e7, 7);
    
    // WGS and MSL Altitude 
    Serial.println("WGS Altitude: " + String(wgsAltitude / 1000.0, 2) + " m, " + "MSL Altitude: " + String(mslAltitude / 1000.0, 2) + " m");
    output += "," + String(wgsAltitude / 1000.0, 2) + "," + String(mslAltitude / 1000.0, 2); 

    // Horizontal and vertical accuracy (not logged to SD card)
    Serial.println("Horizontal Accuracy: " + String(horizontalAccuracy / 1000.0, 2) + " m, " +"Vertical Accuracy: " + String(verticalAccuracy / 1000.0, 2) + " m");
    
    // Speed accuracy (not logged to SD card)
    Serial.println("Speed Accuracy: " + String(speedAccuracy / 1000.0, 2) + " m/s");
    
    // Speed
    // Serial.println("Speed: " + String(speed / 1000.0, 2) + " m/s");
    Serial.println("Speed: " + String(speed * 3.6 / 1000.0, 2) + " km/h");
    output += "," + String(speed * 3.6 / 1000.0, 2);
    
    // Heading and compass direction
    Serial.print("Heading Accuracy: " + String(headingAccuracy / 1e5, 1) + " deg");
    Serial.println(" (heading " + String((fixStatusFlags & 0x20) ? "valid)" : "NOT valid - may need movement to become valid)"));
    // Serial.print("Heading: " + String(heading / 1e5, 1) + " deg");
    Serial.print("Heading: ");
    Serial.print(headingDegrees, 1); // heading (one decimal)
    Serial.print(" deg, compass direction: ");
    Serial.println(compass_direction); // magnetic compass direction (e.g., "N", "NO")
    output += "," + String(headingDegrees, 1) + "," + String(compass_direction);

    // G Force
    // Serial.println("PDOP: " + String(pdop / 100.0, 2));
    Serial.println("G-Force X: " + String(gForceX / 1000.0, 3) + ", Y: " + String(gForceY / 1000.0, 3) + ", Z: " + String(gForceZ / 1000.0, 3));
    output += "," + String(gForceX / 1000.0, 3) + "," + String(gForceY / 1000.0, 3) + "," + String(gForceZ / 1000.0, 3);

    Serial.println("Rot Rate X: " + String(rotRateX / 100.0, 2) + " deg/s" + ", Y: " + String(rotRateY / 100.0, 2) + " deg/s" + " Z: " + String(rotRateZ / 100.0, 2) + " deg/s");
    output += "," + String(rotRateX / 100.0, 2) + "," + String(rotRateY / 100.0, 2) + "," + String(rotRateZ / 100.0, 2);

    // print fix status flags
    //        Serial.println("Fix Status Flags (Hex): " + String(fixStatusFlags, HEX));
    //        Serial.println("Fix Status Flags (Binary): " + String(fixStatusFlags, BIN));

    // print fix status flags with interpretation
    Serial.println("Fix Status Flags Interpretation:");
    Serial.println("  Bit 0: Valid Fix: " + String((fixStatusFlags & 0x01) ? "Yes" : "No"));
    Serial.println("  Bit 1: Differential Corrections Applied: " + String((fixStatusFlags & 0x02) ? "Yes" : "No"));
    Serial.println("  Bits 4..2: Power State: " + String((fixStatusFlags >> 2) & 0x07));
    Serial.println("  Bit 5: Valid Heading: " + String((fixStatusFlags & 0x20) ? "Yes" : "No"));
    Serial.println("  Bits 7..6: Carrier Phase Range Solution: " + String((fixStatusFlags >> 6) & 0x03));
    Serial.println();

    // Write data to sd card
    output += "\n";
    appendFile(SD, "/gps-data/gps-data.csv", output.c_str());

    // Serial.println("Battery Status: " + String(batteryStatus)); //needs a function for interpretation, which is depending on device type:
    decodeBatteryStatus(batteryStatus); // a separate decoding funtion is a better solution, as there are differences in interpretation depending if it is a racebox mini, mini s oder micro
    Serial.println();
  }
  else {
    Serial.println("skipping serial output due to set serial update limitation");
  }
  Serial.println("--------------------------------------------------------------------------------------------------");
  Serial.println();
}

String getCompassDirection(float headingDegrees) {
  if (headingDegrees >= 337.5 || headingDegrees < 22.5)
    return "N";
  if (headingDegrees >= 22.5 && headingDegrees < 67.5)
    return "NO";
  if (headingDegrees >= 67.5 && headingDegrees < 112.5)
    return "O";
  if (headingDegrees >= 112.5 && headingDegrees < 157.5)
    return "SO";
  if (headingDegrees >= 157.5 && headingDegrees < 202.5)
    return "S";
  if (headingDegrees >= 202.5 && headingDegrees < 247.5)
    return "SW";
  if (headingDegrees >= 247.5 && headingDegrees < 292.5)
    return "W";
  if (headingDegrees >= 292.5 && headingDegrees < 337.5)
    return "NW";
  return ""; // Default case, shouldn't be reached
}

void decodeBatteryStatus(uint8_t batteryStatus) {
  // RaceBox Micro
  Serial.print("RaceBox Micro - ");          // interpreting battery status as input voltage (according to datasheet)
  float inputVoltage = batteryStatus / 10.0; // Input voltage must be multiplied by 10, according to datasheet
  Serial.print("Input Voltage: ");
  Serial.print(inputVoltage, 1); // print with one decimal
  Serial.println(" V");
}

void calculateChecksum(uint8_t *data, uint16_t length, uint8_t &CK_A, uint8_t &CK_B) {
  CK_A = 0;
  CK_B = 0;
  for (int i = 2; i < length - 2; i++)
  { // start after header bytes and end before checksum bytes
    CK_A += data[i];
    CK_B += CK_A;
  }
}

void parsePayload(uint8_t *data) {
  if (data[0] != 0xB5 || data[1] != 0x62) {
    Serial.println("Invalid frame start of payload data - check may need to be removed or changed for other data than RaceBox Data Message!");
    return;
  }

  // Extract data from payload, at first we need the payloadLength to calculate the checksum
  // examples for data content, mostly copied from RaceBox datasheet:
  header = *(reinterpret_cast<uint16_t *>(data));            // 0xB5 0x62 (that are the two header identification bytes, according to RaceBox datasheet: The first 2 bytes are the frame start - always 0xB5 and 0x62.)
  messageClass = *(reinterpret_cast<uint8_t *>(data + 2));   // expecting 0xFF for a RaceBox data message
  messageId = *(reinterpret_cast<uint8_t *>(data + 3));      // expecting 0x01 for a RaceBox data message (equals 0x1)
  payloadLength = *(reinterpret_cast<uint16_t *>(data + 4)); // e.g. 0x50 0x00 (80 bytes for live data - attention: other data is larger (up to 509 bytes), divided into multiple packets and needs to be reassembled from multiple packets - refer to datasheet)

  // validate the length of the packet
  uint16_t packetLength = 6 + payloadLength + 2; // header (6 bytes) + payload + checksum (2 bytes)
  if (packetLength > 512)
  { // double check if 5
    Serial.print("Received packet size exceeds maximum allowed size (512 bytes). ");
    Serial.print("Packet length is ");
    Serial.print(packetLength);
    Serial.println(" bytes.");
    return;
  }
  else
  { // if packetLength is within allowed limits, print out some info on the data packet:
    // Serial.println("Payload length is " + String(payloadLength) + " bytes");
    // Serial.println("Expected payload length according to datasheet: 0 - 504 bytes. For a RaceBox Data Message payload length is 80 bytes.");
    // Serial.println("Packet length (including checksum) is " + String(packetLength) + " bytes");
  }

  // validate checksum
  uint8_t CK_A, CK_B;
  calculateChecksum(data, packetLength, CK_A, CK_B);
  if (data[packetLength - 2] != CK_A || data[packetLength - 1] != CK_B)
  {
    Serial.println("*** Checksum validation of incoming data package failed. ***");
    return;
  }
  else
  {
    // Serial.println("Checksum validation successful.");
  }
  // Serial.println();

  // print message class and message ID - used to determine the type of message. A RaceBox Data Message has messageClass 0xFF and messageId 0x01.
  //    Serial.print("Message Class: 0x");
  //    Serial.println(messageClass, HEX);
  //    Serial.print("Message ID: 0x");
  //    Serial.println(messageId, HEX);

  // check if the message class and ID match the expected values for a live data packet
  if (messageClass == 0xFF || messageId == 0x01)
  { // in case we receive live data (standard on start of RaceBox) and interpret it accordingly
    // Serial.println("the received message has messageClass 0xFF and messageId 0x01, this is a (valid) RaceBox Data Message. Parsing payload.");
    parse_RaceBox_Data_Message_payload(data); // sending variable data to this function to interpret it
    // outputting received data (this will be triggered each time a payload is parsed, so be aware that it may delay data update rate if e.g. printing a lot of info to serial takes longer than it takes for the next data to arrive.)
    //    print_RaceBox_Data_message_payload_to_oled();
    //
    //      //####### be aware that those serial outputs below are sent each time a payload is parsed (i.e. up to 25x per second) like an interrupt, so the serial output from this function can overlay/interfere with output from void loop #######
    //    print_RaceBox_Data_message_payload_to_serial(); //<-- ATTENTION! see comment above
    //      //if you move this function to loop, you may want to comment out all serial outputs from void parsePayload.
  }

  // examples how to handle other received messages;
  else if (messageClass == 0xFF || messageId == 0x21)
  { // History Data Message
    Serial.println("the received message has messageClass 0xFF and messageId 0x21, this is a (valid) History Data Message message. Parsing payload NOT yet implemented.");
    // parse_History_Data_Message_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  }
  else if (messageClass == 0xFF || messageId == 0x22)
  { // Standalone Recording Status
    Serial.println("the received message has messageClass 0xFF and messageId 0x22, this is a (valid) Standalone Recording Status message. Parsing payload NOT yet implemented.");
    // parse_standalone_Recording_Status_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  }
  else if (messageClass == 0xFF || messageId == 0x23)
  { // Recorded Data Download
    Serial.println("the received message has messageClass 0xFF and messageId 0x23, this is a (valid) Recorded Data Download message. Parsing payload NOT yet implemented.");
    // parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  }
  else if (messageClass == 0xFF || messageId == 0x26)
  { // Standalone Recording State Change Message
    Serial.println("the received message has messageClass 0xFF and messageId 0x26, this is a (valid) Standalone Recording State Change Message. Parsing payload NOT yet implemented.");
    // parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  }

  //    else if (messageClass == 0x_something_else_1 || messageId == 0x_something_else_2){
  //      //handle other message class(es) like this
  //    }

  else
  { // in case we receive different data (with different message class or message IDs as implemented above, we would need to handle it differently, or even assemble multiple messages that may have ben split.
    Serial.print("unknown message class and message ID found (it may be other data?): ");
    Serial.print("Message Class: 0x");
    Serial.print(messageClass, HEX);
    Serial.print(", Message ID: 0x");
    Serial.println(messageId, HEX);
    Serial.println("Ignoring packet. This is not a known/implemented data packet. Interpreting the payload for this kind of packet is not yet implemented.");
    return;
  }
}

void parse_RaceBox_Data_Message_payload(uint8_t *data) { 
  // function to handle payload of a RaceBox Data Message
  // writing updated values (from payload) to the variables:
  iTOW = *(reinterpret_cast<uint32_t *>(data + 6));                // e.g 0xA0 0xE7 0x0C 0x07
  year = *(reinterpret_cast<uint16_t *>(data + 10));               // e.g 0xE6 0x07 (2022) or 0xE8 0x07 (2024)
  month = *(reinterpret_cast<uint8_t *>(data + 12));               // 0x01 (january) or 0x08 (august)
  day = *(reinterpret_cast<uint8_t *>(data + 13));                 // 0x0A (10th) or 0x08 (8th)
  hour = *(reinterpret_cast<uint8_t *>(data + 14));                // 0x08 (08 o'clock)
  minute = *(reinterpret_cast<uint8_t *>(data + 15));              // 0x33 (51 min)
  second = *(reinterpret_cast<uint8_t *>(data + 16));              // 0x08 (08 seconds)
  validityFlags = *(reinterpret_cast<uint8_t *>(data + 17));       // 0x37 (Date/Time valid)
  timeAccuracy = *(reinterpret_cast<uint32_t *>(data + 18));       // 0x19000000 (25 ns)
  nanoseconds = *(reinterpret_cast<uint32_t *>(data + 22));        // 0x2AAD4D0E (239971626 ns = 0.239 seconds)
  fixStatus = *(reinterpret_cast<uint8_t *>(data + 26));           // 0x03 (3D Fix)
  fixStatusFlags = *(reinterpret_cast<uint8_t *>(data + 27));      // 0x01 (GNSS Fix OK)
  dateTimeFlags = *(reinterpret_cast<uint8_t *>(data + 28));       // 0xEA (Date/Time Confirmed)
  numSVs = *(reinterpret_cast<uint8_t *>(data + 29));              // 0x0B (11 satellites)
  longitude = *(reinterpret_cast<int32_t *>(data + 30));           // 0xC693E10D (23.2887238 degrees)
  latitude = *(reinterpret_cast<int32_t *>(data + 34));            // 0x3B376F19 (42.6719035 degrees)
  wgsAltitude = *(reinterpret_cast<int32_t *>(data + 38));         // 0x618C0900 (625.761 meters)
  mslAltitude = *(reinterpret_cast<int32_t *>(data + 42));         // 0x0F010900 (590.095 meters)
  horizontalAccuracy = *(reinterpret_cast<uint32_t *>(data + 46)); // 0x9C030000 (0.924 meters)
  verticalAccuracy = *(reinterpret_cast<uint32_t *>(data + 50));   // 0x2C070000 (1.836 meters)
  speed = *(reinterpret_cast<uint32_t *>(data + 54));              // 0x23000000 (35 mm/s = 0.126 km/h)
  heading = *(reinterpret_cast<uint32_t *>(data + 58));            // 0x00000000 (0 degrees)
  speedAccuracy = *(reinterpret_cast<uint32_t *>(data + 62));      // 0xD0000000 (208 mm/s = 0.704 km/h)
  headingAccuracy = *(reinterpret_cast<uint32_t *>(data + 66));    // 0x88A9DD00 (145.26856 degrees)
  pdop = *(reinterpret_cast<uint16_t *>(data + 70));               // 0x2C01 (3)
  latLonFlags = *(reinterpret_cast<uint8_t *>(data + 72));         // 0x00 (Coordinates valid)
  batteryStatus = *(reinterpret_cast<uint8_t *>(data + 73));       // has to be interpreted depending on if it is a RaceBox micro or mini, see my function void decodeBatteryStatus
  gForceX = *(reinterpret_cast<int16_t *>(data + 74));             // 0xFDFF (-0.003 g)
  gForceY = *(reinterpret_cast<int16_t *>(data + 76));             // 0x7100 (0.113 g)
  gForceZ = *(reinterpret_cast<int16_t *>(data + 78));             // 0xCE03 (0.974 g)

  headingDegrees = heading / 100000.0;                     // convert it to a float variable that is needed for the function getCompassDirection
  compass_direction = getCompassDirection(headingDegrees); // generate human readable compass_direction like N, NW, SW etc. from the heading degrees and save them in String 'compass_direction'
  updated_RaceBox_Data_Message = true;                     // bool is used to determine if updated data for variables in RaceBox Data Message is available (e.g. to print or display them in void loop() )
}

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (length >= 80) {
    parsePayload(pData);
  } else {
    Serial.println(F("payload length is less than 80 bytes. 80 bytes would be expected for a RaceBox data message."));
    Serial.println(F("For other messages, the payload can be shorter."));
    parsePayload(pData);
  }
}

void ble_task(void *pvParameters) {

  Serial.println(F("Scanning for Bluetooth devices."));
  Serial.println();
  Serial.println(F("Scan Results:"));
  Serial.println();

  NimBLEDevice::init("ESP32_RaceBox_Client");
  NimBLEScan* pScan = NimBLEDevice::getScan();

  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(0, false); // scan indefinitely until we stop it manually

  // Main code to run once a connection is established.
  while (1) {
    if (doConnect) {
      if (connectToRaceBox()) {
        Serial.println(F("successfully connected to RaceBox."));
        Serial.println();
        NimBLEDevice::getScan()->stop();
      } else {
        Serial.println(F("Failed to connect to RaceBox. Reattempting BLE connection..."));
        NimBLEScan* pScan = NimBLEDevice::getScan();
        pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
        pScan->setInterval(45);
        pScan->setWindow(15);
        pScan->setActiveScan(true);
        pScan->start(0, false); //scan indefinitely until we stop it manually
      }
      doConnect = false;
    }

    if (connected) {
      if (updated_RaceBox_Data_Message == true) {
        print_RaceBox_Data_message_payload_to_serial();
        updated_RaceBox_Data_Message = false;
      }
    }
  }
}


//-----------------------------
// Setup
//-----------------------------

void setup_sd_card() {
  gpio_set_direction(SD_DETECT_GPIO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(SD_DETECT_GPIO, GPIO_PULLUP_ONLY);
  
  int sd_detected = digitalRead(SD_DETECT); // 0 When SD card is present
  
  if(sd_detected == 0) {
    Serial.println(F("SD Card Detected"));

    if (!SD.begin(SDCS, spi)) {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
      Serial.println(F("No SD card attached"));
      return;
    }
    
    Serial.println(F("SD Card Mounted Successfully"));
    
    // Check and create directories for data capture
    check_and_create_directory("gps-data", "GPS");
    check_and_create_directory("can-bus-data", "CAN BUS");

  } else {
    Serial.println(F("SD Card Not Detected"));
  }

}

void setup_leds() {
  FastLED.addLeds<NEOPIXEL, RGB_PIN>(leds, NUM_RPM_LEDS);
  FastLED.setBrightness(LED_DEFAULT_BRIGHTNESS);
  RGB_startup_animation();
}

void setup_spi() {
  spi.begin(SCLK, MISO, MOSI, SDCS);
}

void setup_buzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  buzz_double();
}

void setup(void) {
  Serial.begin(SERIAL_BAUDRATE);
  setup_buzzer();
  setup_leds();
  setup_spi();
  setup_sd_card();

  gui_mutex = xSemaphoreCreateMutex();
  if (gui_mutex == NULL) {
    Serial.println(F("Semaphore creation failure"));
    return;
  }

  // Args: function, name of task, stack size (bytes), priority, core to pin to
  xTaskCreatePinnedToCore(display_task, "loading_task", 1024 * 10, NULL, 3, NULL, 1); 
  xTaskCreatePinnedToCore(display_update_task, "loading_task", 1024 * 3, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(ble_task, "ble_task", 1024 * 10, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sensor_task, "sensor_task", 1024 * 5, NULL, 1, NULL, 1);

  // RPM Lights Demo
  // xTaskCreatePinnedToCore(demo_rpm_lights, "demo_rpm_lights", 1024 * 5, NULL, 3, NULL, 1);
  
}


//-----------------------------
// Main Loop - DO NOT USE!
//-----------------------------

void loop(void) {
  // WARNING: DO NOT POPULATE FUNCTION
}