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


// LEDs
CRGBArray<NUM_RPM_LEDS> leds;
int LEDBrightness = LED_DEFAULT_BRIGHTNESS;
int rpmLightInterval = MAX_RPM / NUM_RPM_LEDS;

// GUI and Display
SemaphoreHandle_t gui_mutex;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / SCREEN_COLOUR_DIVISOR];

// Race Box Module Bluetooth
NimBLEClient *pClient = nullptr;
static BLEUUID UART_service_UUID(BLE_UART_UUID);
static BLEUUID TX_characteristic_UUID(BLE_TX_UUID);

// CAN BUS
MCP2515 mcp2515(CANBUS_CS);
struct can_frame canMsg, canReqMsg;

// Race Box Module
const int outputFrequencyHzSerial = GPS_SERIAL_FREQUENCY;
const unsigned long outputIntervalMs_serial = 1000 / outputFrequencyHzSerial;

static bool doConnect, bleRequestDisconnect, connected, updated_RaceBox_Data_Message = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myRaceBox;

unsigned long lastOutputTimeSerial = 0;

// Global variables for live data from RaceBox (at 25Hz): (examples see function void parsePayload)
uint16_t header, payloadLength;
uint8_t messageClass, messageId;
uint32_t iTOW;
uint16_t year;
uint8_t month, day, hour, minute, second;
uint8_t validityFlags, latLonFlags, dateTimeFlags;
uint32_t timeAccuracy, nanoseconds;
uint8_t fixStatus, fixStatusFlags, numSVs;
int32_t longitude, latitude;
int32_t wgsAltitude, mslAltitude;
uint32_t horizontalAccuracy, verticalAccuracy;
uint32_t speed, heading, speedAccuracy, headingAccuracy;
uint16_t pdop;
uint8_t batteryStatus;
int16_t gForceX, gForceY, gForceZ;
int16_t rotRateX, rotRateY, rotRateZ;
float headingDegrees;
String compass_direction;

// S60 ECU Packet Global Variables
int rpm, tps, water_temp = 0;       // Packet 2000 - [0] RPM, [1] Throttle Position Sensor, [2] Water Temperature
int kph = 0;                        // Packet 2001 - [2] Speed reported by ECU
int oil_temp, battery_voltage = 0;  // Packet 2002 - [1] Oil Temperature, [2] Battery Voltage
int gear = 0;                       // Packet 2003 - [0] Gear
int bps = 0;                        // Packet 2004 - [0] Brake Position Sensor


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


//-----------------------------
// Dispay
//-----------------------------

void display_task(void *pvParameters) {

  String LVGL_Arduino = "LVGL Arduino ";
  LVGL_Arduino += "\n" + String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);

  tft.init();
  tft.setRotation(DISPLAY_ROTATION);
  tft.setBrightness(TFT_DEFAULT_BRIGHTNESS);

  lv_init();

  #if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
  #endif

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

  // Initialize the display
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);

  // Change the following line to your display resolution
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Initialize the (dummy) input device driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();
  ui_reset();

  // Assign callback functions

  // Restart button
  lv_obj_add_event_cb(ui_SettingScreen_Button_ButtonRestart, ui_event_SettingScreen_Button_ButtonRestart, LV_EVENT_PRESSED, NULL);
  // BLE disconnect button
  lv_obj_add_event_cb(ui_SettingScreen_Button_ButtonBLEDisconnect, ui_event_SettingScreen_Button_ButtonBLEDisconnect, LV_EVENT_PRESSED, NULL);
  // LED brightness
  lv_obj_add_event_cb(ui_SettingScreen_Slider_SliderLEDBrightness, ui_event_SettingScreen_Slider_SliderLEDBrightness, LV_EVENT_VALUE_CHANGED, NULL);
  // Disp brightness
  lv_obj_add_event_cb(ui_SettingScreen_Slider_SliderDisplayBrightness, ui_event_SettingScreen_Slider_SliderDisplayBrightness, LV_EVENT_VALUE_CHANGED, NULL);

  // Main LVGL loop
  while (true) {

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

  delay(DISPLAY_INIT_DELAY); // wait for display init

  // Screen is initiated with the loading screen first
  lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, 0, LV_ANIM_OFF);

  for (int i = 0; i < 100; i++) {
    delay(LOADING_SCREEN_DELAY);
    lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, i, LV_ANIM_OFF);
  }

  lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Connecting..");

  lv_scr_load(ui_MainScreen);

  float gX, gY, gZ, g_mag = 0.0;

  set_rpm_lights(0);

  while (true) {
    if (connected) {

      gX = gForceX / 1000.0;
      gY = gForceY / 1000.0;
      gZ = gForceZ / 1000.0;
      g_mag = sqrt(gX * gX + gY * gY + gZ * gZ);

      lv_label_set_text_fmt(ui_MainScreen_Label_LabelSpeed, "%.0f", (speed / 1000.0) * 2.23694); // Speed converted from m/s to mph
      lv_label_set_text_fmt(ui_MainScreen_Label_LabelGPSTrack, "GPS Fix: %i", numSVs);           // Number of connected satelites
      lv_label_set_text(ui_MainScreen_Label_LabelGForce, String(gX, 1).c_str());                 // G Force

      // Time 
      char timeString[9];                                         
      sprintf(timeString, "%02d:%02d:%02d", hour, minute, second);
      String timeOutput = "Time: " + String(timeString);  
      lv_label_set_text(ui_MainScreen_Label_LabelTime, timeOutput.c_str());

    } else {

      if (bleRequestDisconnect) {
        lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Disconnected");
      } else {
        //TODO add a GPS connection count to facilitate the ... animation
        lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Connecting..");
      }
    }
    
    // SD Card Status
    if(!digitalRead(SD_DETECT)) {
      lv_label_set_text(ui_MainScreen_Label_LabelSDCardMounted, "SD: Mounted"); 
    } else {
      lv_label_set_text(ui_MainScreen_Label_LabelSDCardMounted, "SD: Not Mounted"); 
    }

    // Set RPM Data on UI
    lv_bar_set_value(ui_MainScreen_Bar_BarRPM, map((rpm * 400), 0, 12000, 0, 100), LV_ANIM_OFF);
    set_rpm_lights(rpm * 400);
    // lv_label_set_text_fmt(ui_MainScreen_Label_LabelRPM, rpm * 400);

    lv_bar_set_value(ui_MainScreen_Bar_BarTPS, tps, LV_ANIM_OFF);
    lv_bar_set_value(ui_MainScreen_Bar_BarBPS, bps, LV_ANIM_OFF);

    if(gear > 0) {
      lv_label_set_text_fmt(ui_MainScreen_Label_LabelGear, "%i", gear);
    } else {
      lv_label_set_text_fmt(ui_MainScreen_Label_LabelGear, "N");
    }

    vTaskDelay(10);
  }
}

void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp_drv);
}

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
// CAN BUS
//-----------------------------

void can_bus_task(void *pvParameters) {

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

  delay(CANBUS_START_DELAY); // wait for display init

  while (true) {

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
          set_rpm_lights(rpm_decoded);
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
          lv_label_set_text_fmt(ui_MainScreen_Label_LabelGear, "Gear: %.2f", transmission_actual_gear_decoded);
          break;
        case PID_CONTROL_MODULE_VOLTAGE:
          battery_byte = (uint16_t)(canMsg.data[3] << 8) + (canMsg.data[4]);
          battery_decoded = (battery_byte / 1000.0);
          lv_label_set_text_fmt(ui_MainScreen_Label_LabelBattV, "Batt: %.1f", battery_decoded);
          break;
        default:
          break;
        }

        // Write data to SD card
        String output = ""; 
        output = String(rpm_decoded) + "," + String(throttle_decoded) + "," + String(coolant_temp_decoded) + ",";
        output += String(transmission_actual_gear_decoded) + "," + String(battery_decoded) + "\n";
        appendFile(SD, "/can-bus-data/can-bus-data.csv", output.c_str());

        Serial.print(F("CAN Message ID: "));
        Serial.print(canMsg.can_id, HEX); // print ID
        Serial.print(F(" "));
        Serial.print(F("Message Length: "));
        Serial.print(canMsg.can_dlc, HEX); // print DLC
        Serial.print(F(" "));
        Serial.print("Data: ");
        for (int i = 0; i < canMsg.can_dlc; i++) {
          Serial.print(canMsg.data[i], HEX);
          Serial.print(F(" "));
        }
        Serial.println();
        Serial.print(F("canbus_data_counter: "));
        Serial.println(canbus_data_counter);
        canbus_data_counter++;
        send_rq = 0; // reset request
      } else {
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

void can_bus_s60_ecu(void *pvParameters) {

  String outputString = "";
  
  delay(CANBUS_START_DELAY); // wait for display init
  Serial.println(F("CAN BUS S60 ECU"));

  while (true){
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      Serial.println(F("Reading CAN BUS data"));

      outputString = "CAN Message ID: " + String(canMsg.can_id, HEX)  + " Message Length: " + String(canMsg.can_dlc, HEX) + " Data: ";

      switch(canMsg.can_id) {
        case PID_2000:
          rpm = canMsg.data[0];
          tps = canMsg.data[1];
          water_temp = canMsg.data[2];
          break;
        case PID_2001:
          kph = canMsg.data[2];
          break;
        case PID_2002:
          oil_temp = canMsg.data[1];
          battery_voltage = canMsg.data[2];
          break;
        case PID_2003:
          gear = canMsg.data[0];
          break;
        case PID_2004:
          bps = canMsg.data[0];
          break;
        default:
          break;
      }

      for (int i = 0; i < canMsg.can_dlc; i++)  {
        outputString += String(canMsg.data[i], HEX) + " ";
      }
      outputString += "\n";

      Serial.print(outputString);
      vTaskDelay(10);
    }
  } 
}


//-----------------------------
// UI Functions
//-----------------------------

static void ui_event_SettingScreen_Slider_SliderLEDBrightness(lv_event_t *event) {
  lv_obj_t *slider = lv_event_get_target(event);
  LEDBrightness = (int)lv_slider_get_value(slider);
  FastLED.setBrightness(map((int)lv_slider_get_value(slider), 0, 100, 0, 255));
  FastLED.show();
}

static void ui_event_SettingScreen_Slider_SliderDisplayBrightness(lv_event_t *event) {
  lv_obj_t *slider = lv_event_get_target(event);
  tft.setBrightness(map((int)lv_slider_get_value(slider), 0, 100, 5, 255));
}

static void ui_event_SettingScreen_Button_ButtonRestart(lv_event_t *event) {
  delay(RESTART_DELAY);
  ESP.restart();
}

static void ui_event_SettingScreen_Button_ButtonBLEDisconnect(lv_event_t *event) {
  connected = false;
  bleRequestDisconnect = true;

  size_t numClients = NimBLEDevice::getClientListSize();
  if (numClients > 0) {
    std::list<NimBLEClient *> *clientList = NimBLEDevice::getClientList();
    for (auto it = clientList->begin(); it != clientList->end(); it++) {
      if ((*it)->isConnected()) {
        (*it)->disconnect();
      }
    }
  }
  NimBLEDevice::deinit();
}

void ui_reset() {
  lv_label_set_text(ui_MainScreen_Label_LabelRPM, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelGear, "N");
  lv_label_set_text(ui_MainScreen_Label_LabelSpeed, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelBattV, "Batt: USB");
  lv_label_set_text(ui_MainScreen_Label_LabelGForce, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelWaterTemp, "Water: 0 C");
  lv_label_set_text(ui_MainScreen_Label_LabelOilTemp, "Oil: 0 C");
  lv_bar_set_value(ui_MainScreen_Bar_BarTPS, 0, LV_ANIM_OFF);
  lv_bar_set_value(ui_MainScreen_Bar_BarBPS, 0, LV_ANIM_OFF);
  lv_bar_set_value(ui_MainScreen_Bar_BarRPM, 0, LV_ANIM_OFF);
}


//----------------
// RGB LEDs
//----------------

void set_rpm_lights(int rpmValue) {
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
      leds[i] = CRGB::Black;
      FastLED.show();
    }
  }
}

void set_all_leds(struct CRGB colour) {
  leds.fill_solid(colour);
}

void upshifting_blink() {
  set_all_leds(CRGB::Red); 
  FastLED.show();
  delay(UPSHIFT_BLINK_DELAY);
  set_all_leds(CRGB::Black); 
  FastLED.show();
  delay(UPSHIFT_BLINK_DELAY);
}

void rgb_startup_animation() {

  set_all_leds(CRGB::Black);
  FastLED.show();

  delay(LED_ANIMATION_DELAY);
  leds[0] = leds[9] = CRGB::Green;
  FastLED.show();
  delay(LED_ANIMATION_DELAY);
  leds[1] = leds[8] = CRGB::Green;
  FastLED.show();
  delay(LED_ANIMATION_DELAY);
  leds[2] = leds[7] = CRGB::Orange;
  FastLED.show();
  delay(LED_ANIMATION_DELAY);
  leds[3] = leds[6] = CRGB::Red;
  FastLED.show();
  delay(LED_ANIMATION_DELAY);
  leds[4] = leds[5] = CRGB::Red;
  FastLED.show();
  delay(LED_ANIMATION_DELAY);

  upshifting_blink();
  upshifting_blink();

  leds[0] = leds[1] = leds[8] = leds[9] = CRGB::Green; // First and last two
  leds[2] = leds[7] = CRGB::Orange;                    // Third from the start and end
  leds[3] = leds[4] = leds[5] = leds[6] = CRGB::Red;   // Middle four
   
  FastLED.show();
}

void demo_rpm_lights(void *pvParameters) {
  while (true) {
    for (int i = 0; i < 13; i++) {
      rpm = i * 1000;
      set_rpm_lights(rpm);
      delay(LED_ANIMATION_DELAY);
    }
  }
}


//-----------------------------
// Buzzer
//-----------------------------

void buzz_double() {
  for (int i = 0; i < 2; i++) {
    digitalWrite(BUZZER, HIGH);
    delay(BUZZER_TONE_DELAY);
    digitalWrite(BUZZER, LOW);
    delay(BUZZER_TONE_DELAY);
  }
}


//-----------------------------
// Directory and File Functions
//-----------------------------

void createDir(fs::FS &fs, const char * path) {
  Serial.printf("Creating Directory: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println(F("Directory created"));
  } else {
    Serial.println(F("Directory creation failed"));
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
    Serial.println(F("Data Appended to File"));
  } else {
    Serial.println(F("Data Append to File Failed"));
  }
  file.close();
}

void check_and_create_directory(String directory, String module) {
  if (!SD.exists(("/" + directory + "/" + directory + ".csv").c_str())) {
    createDir(SD, ("/" + directory).c_str());
    Serial.println(("Creating " + module + " File").c_str());

    if(module == "GPS") {
      appendFile(SD, ("/" + directory + "/" + directory + ".csv").c_str(), ("Date,Time-UTC,GPS-Fix,Satellites,Latitude,Longitude,WGS-Altitude,MSL-Altitude,Speed-KPH,Heading,Compass-Direction,G-Force-X,G-Force-Y,G-Force-Z,Rotation-X,Rotation-Y,Rotation-Z\n"));
    } else if(module == "CAN BUS") {
      appendFile(SD, ("/" + directory + "/" + directory + ".csv").c_str(), ("RPM,Throttle,Coolant,Gear,Battery\n"));
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
        Serial.println(F("Failed to reconnect. Retrying..."));
        // Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
        return false;
      }
    } else {
      // Create a new client if none matches
      pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(new ClientCallbacks(), false);
      if (!pClient->connect(myRaceBox)) {
        Serial.println(F("Failed to connect."));
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
      Serial.println(F("Failed to connect."));
      NimBLEDevice::deleteClient(pClient);
      // Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
      return false;
    }
  }

  // obtain the service and characteristic
  BLERemoteService *pService = pClient->getService(UART_service_UUID);
  if (pService != nullptr) {
    pRemoteCharacteristic = pService->getCharacteristic(TX_characteristic_UUID);
    if (pRemoteCharacteristic != nullptr) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      Serial.println(F("DEBUG: connectToRaceBox() will now return true and exit."));
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
    Serial.println(F(""));
    Serial.println(F("----------------------------------------------------------------------"));
    Serial.println(F("--- Updated Data From RaceBox: ---"));
    Serial.println(F("----------------------------------------------------------------------"));
    
    String output = "";

    // Date and Time
    // Serial.println("iTOW: " + String(iTOW) + " ms");
    output += String(day) + "/" + String(month) + "/" + String(year);

    // Time
    char timeString[9];                                          // Buffer to store the formatted time string
    sprintf(timeString, "%02d:%02d:%02d", hour, minute, second); // build a time string that always has the time format 00:00:00
    Serial.println("Date: " + String(day) + "/" + String(month) + "/" + String(year) + " Time (UTC): " + String(timeString) + "\n");
    output += ","+String(timeString);

    // GPS Fix and Number of Satellites
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
    Serial.println("GPS: " + fixStatusText + ", Satellites: " + String(numSVs) + "\n");
    output += "," + fixStatusText + "," + String(numSVs);

    // print fix status flags
    // Serial.println("Fix Status Flags (Hex): " + String(fixStatusFlags, HEX));
    // Serial.println("Fix Status Flags (Binary): " + String(fixStatusFlags, BIN));

    // print fix status flags with interpretation
    // Serial.println("Fix Status Flags Interpretation:");
    // Serial.println("  Bit 0: Valid Fix: " + String((fixStatusFlags & 0x01) ? "Yes" : "No"));
    // Serial.println("  Bit 1: Differential Corrections Applied: " + String((fixStatusFlags & 0x02) ? "Yes" : "No"));
    // Serial.println("  Bits 4..2: Power State: " + String((fixStatusFlags >> 2) & 0x07));
    // Serial.println("  Bit 5: Valid Heading: " + String((fixStatusFlags & 0x20) ? "Yes" : "No"));
    // Serial.println("  Bits 7..6: Carrier Phase Range Solution: " + String((fixStatusFlags >> 6) & 0x03));
    // Serial.println();

    // Lattitude and longitude
    // we need to divide the latitude by 10^7 because the datasheet states that it is transmitted with a factor of 10^7
    Serial.println("Latitude: " + String(latitude / 1e7, 7) + "deg, " + "Longitude: " + String(longitude / 1e7, 7) + "deg"); 
    output += "," + String(latitude / 1e7, 7)  + "," + String(longitude / 1e7, 7);
    
    // WGS and MSL Altitude 
    Serial.println("WGS Altitude: " + String(wgsAltitude / 1000.0, 2) + "m, " + "MSL Altitude: " + String(mslAltitude / 1000.0, 2) + "m");
    output += "," + String(wgsAltitude / 1000.0, 2) + "," + String(mslAltitude / 1000.0, 2); 

    // Horizontal and vertical accuracy (not logged to SD card)
    Serial.println("Horizontal Accuracy: " + String(horizontalAccuracy / 1000.0, 2) + "m, " + "Vertical Accuracy: " + String(verticalAccuracy / 1000.0, 2) + "m" + "\n");
    
    
    // Speed
    // Serial.println("Speed: " + String(speed / 1000.0, 2) + " m/s");
    Serial.println("Speed: " + String(speed * 3.6 / 1000.0, 2) + "km/h");
    Serial.println("Speed Accuracy: " + String(speedAccuracy / 1000.0, 2) + "m/s" + "\n");
    output += "," + String(speed * 3.6 / 1000.0, 2);
    
    // Heading and compass direction
    Serial.print("Heading Accuracy: " + String(headingAccuracy / 1e5, 1) + "deg");
    Serial.println(" (heading " + String((fixStatusFlags & 0x20) ? "valid)" : "NOT valid - may need movement to become valid)"));
    // Serial.print("Heading: " + String(heading / 1e5, 1) + " deg");
    Serial.print("Heading: ");
    Serial.print(headingDegrees, 1); // heading (one decimal)
    Serial.print("deg, Compass Direction: ");
    Serial.println(compass_direction + "\n"); // magnetic compass direction (e.g., "N", "NO")
    output += "," + String(headingDegrees, 1) + "," + String(compass_direction);

    // G Force
    // Serial.println("PDOP: " + String(pdop / 100.0, 2));
    Serial.println("G-Force X: " + String(gForceX / 1000.0, 3) + ", Y: " + String(gForceY / 1000.0, 3) + ", Z: " + String(gForceZ / 1000.0, 3));
    output += "," + String(gForceX / 1000.0, 3) + "," + String(gForceY / 1000.0, 3) + "," + String(gForceZ / 1000.0, 3);

    Serial.println("Rot Rate X: " + String(rotRateX / 100.0, 2) + "deg/s" + ", Y: " + String(rotRateY / 100.0, 2) + "deg/s" + " Z: " + String(rotRateZ / 100.0, 2) + "deg/s" + "\n");
    output += "," + String(rotRateX / 100.0, 2) + "," + String(rotRateY / 100.0, 2) + "," + String(rotRateZ / 100.0, 2);

    // Battery
    float inputVoltage = batteryStatus / 10.0; // Input voltage must be multiplied by 10, according to datasheet
    Serial.println("RaceBox Input Voltage: " + String(inputVoltage, 1) + "V" + "\n");

    // Write data to sd card
    output += "\n";
    appendFile(SD, "/gps-data/gps-data.csv", output.c_str());
  }
  else {
    Serial.println(F("Skipping serial output due to set serial update limitation"));
  }
  
  Serial.println(F(""));
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

void calculateChecksum(uint8_t *data, uint16_t length, uint8_t &CK_A, uint8_t &CK_B) {
  CK_A = CK_B = 0;
  for (int i = 2; i < length - 2; i++) { // start after header bytes and end before checksum bytes
    CK_A += data[i];
    CK_B += CK_A;
  }
}

void parsePayload(uint8_t *data) {
  if (data[0] != 0xB5 || data[1] != 0x62) {
    Serial.println(F("Invalid frame start of payload data - check may need to be removed or changed for other data than RaceBox Data Message!"));
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
  if (packetLength > 512) { // double check if 5
    Serial.print(F("Received packet size exceeds maximum allowed size (512 bytes). "));
    Serial.print(F("Packet length is "));
    Serial.print(packetLength);
    Serial.println(F(" bytes."));
    return;
  }
  else { 
    // if packetLength is within allowed limits, print out some info on the data packet:
    // Serial.println("Payload length is " + String(payloadLength) + " bytes");
    // Serial.println("Expected payload length according to datasheet: 0 - 504 bytes. For a RaceBox Data Message payload length is 80 bytes.");
    // Serial.println("Packet length (including checksum) is " + String(packetLength) + " bytes");
  }

  // validate checksum
  uint8_t CK_A, CK_B;
  calculateChecksum(data, packetLength, CK_A, CK_B);
  if (data[packetLength - 2] != CK_A || data[packetLength - 1] != CK_B) {
    Serial.println(F("*** Checksum validation of incoming data package failed. ***"));
    return;
  } else {
    // Serial.println("Checksum validation successful.");
  }
  // Serial.println();

  // print message class and message ID - used to determine the type of message. A RaceBox Data Message has messageClass 0xFF and messageId 0x01.
  //    Serial.print("Message Class: 0x");
  //    Serial.println(messageClass, HEX);
  //    Serial.print("Message ID: 0x");
  //    Serial.println(messageId, HEX);

  // check if the message class and ID match the expected values for a live data packet
  if (messageClass == 0xFF || messageId == 0x01) { 
    // in case we receive live data (standard on start of RaceBox) and interpret it accordingly
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
  else if (messageClass == 0xFF || messageId == 0x21) { // History Data Message
    Serial.println(F("the received message has messageClass 0xFF and messageId 0x21, this is a (valid) History Data Message message. Parsing payload NOT yet implemented."));
    // parse_History_Data_Message_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  } else if (messageClass == 0xFF || messageId == 0x22) { 
    // Standalone Recording Status
    Serial.println(F("the received message has messageClass 0xFF and messageId 0x22, this is a (valid) Standalone Recording Status message. Parsing payload NOT yet implemented."));
    // parse_standalone_Recording_Status_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  } else if (messageClass == 0xFF || messageId == 0x23)
  { // Recorded Data Download
    Serial.println(F("the received message has messageClass 0xFF and messageId 0x23, this is a (valid) Recorded Data Download message. Parsing payload NOT yet implemented."));
    // parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  } else if (messageClass == 0xFF || messageId == 0x26) { 
    // Standalone Recording State Change Message
    Serial.println(F("the received message has messageClass 0xFF and messageId 0x26, this is a (valid) Standalone Recording State Change Message. Parsing payload NOT yet implemented."));
    // parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
  }
  //    else if (messageClass == 0x_something_else_1 || messageId == 0x_something_else_2){
  //      //handle other message class(es) like this
  //    }
  else { 
    // in case we receive different data (with different message class or message IDs as implemented above, we would need to handle it differently, or even assemble multiple messages that may have ben split.
    Serial.print(F("unknown message class and message ID found (it may be other data?): "));
    Serial.print(F("Message Class: 0x"));
    Serial.print(messageClass, HEX);
    Serial.print(F(", Message ID: 0x"));
    Serial.println(messageId, HEX);
    Serial.println(F("Ignoring packet. This is not a known/implemented data packet. Interpreting the payload for this kind of packet is not yet implemented."));
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
  batteryStatus = *(reinterpret_cast<uint8_t *>(data + 73));       // has to be interpreted depending on if it is a RaceBox micro or mini
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
  Serial.println();
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
  while (true) {
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

void setup_serial() {
  Serial.begin(SERIAL_BAUDRATE);
  while(!Serial);
}

void setup_sd_card() {
  gpio_set_direction(SD_DETECT_GPIO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(SD_DETECT_GPIO, GPIO_PULLUP_ONLY);
  
  if(!digitalRead(SD_DETECT)) { // 0 When SD card is present
    Serial.println(F("\nSD Card Detected"));

    if (!SD.begin(SD_CS)) {
        Serial.println(F("Card Mount Failed"));
        return;
    }
    uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE) {
      Serial.println(F("No SD card attached"));
      return;
    }
    
    Serial.println(F("SD Card Mounted Successfully \n"));
    
    // Check and create directories for data capture
    check_and_create_directory("gps-data", "GPS");
    check_and_create_directory("can-bus-data", "CAN BUS");
    Serial.println(F(""));

  } else {
    Serial.println(F("SD Card Not Detected"));
  }

}

void setup_leds() {
  FastLED.addLeds<NEOPIXEL, RGB>(leds, NUM_RPM_LEDS);
  FastLED.setBrightness(LED_DEFAULT_BRIGHTNESS);
  rgb_startup_animation();
}

void setup_spi() {
  Serial.println(F(""));
  Serial.println(F("Setting up SPI"));
  if(SCK == CUSTOM_SCLK && MISO == CUSTOM_MISO && MOSI == CUSTOM_MOSI) {
    Serial.println(F("SPI Pins setup correctly"));
    Serial.println("SCLK: " + String(SCK)+ " MISO: " + String(MISO) + " MOSI: " + String(MOSI));
  } else {
    Serial.println(F("Check arduino_pin.h file in Arduino15 folder."));
  }
  SPI.begin();
}

void setup_buzzer() {
  pinMode(BUZZER, OUTPUT);
  if(!SILIENCE_BUZZER) {
    buzz_double();
  }
}

void setup_can_bus() {
  mcp2515.reset();
  Serial.println(F(""));
  if(mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ) == MCP2515::ERROR_OK) {
    Serial.println(F("MCP2515 Initialised Successfully"));
    mcp2515.setNormalMode();
  } else {
    Serial.println(F("Error Initialising MCP2515"));
  }
}

void setup(void) {
  setup_serial();
  setup_buzzer();
  setup_leds();
  setup_spi();
  setup_can_bus();
  setup_sd_card();
  Serial.println(F("Devices Setup"));

  gui_mutex = xSemaphoreCreateMutex();
  if (gui_mutex == NULL) {
    Serial.println(F("Semaphore creation failure"));
    return;
  }
  Serial.println(F("GUI Mutex Created"));

  // Args: function, name of task, stack size (bytes), priority, core to pin to
  xTaskCreatePinnedToCore(display_task, "loading_task", 1024 * 10, NULL, 3, NULL, 1); 
  xTaskCreatePinnedToCore(display_update_task, "loading_task", 1024 * 3, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(ble_task, "ble_task", 1024 * 10, NULL, 1, NULL, 1);

  // Setup CAN Bus Task based on ECU type
  if(CANBUS_S60) {
    xTaskCreatePinnedToCore(can_bus_s60_ecu, "can_bus_s60_ecu", 1024 * 5, NULL, 1, NULL, 1);
    Serial.println(F("CAN BUS S60 Task Created"));
  } else {
    xTaskCreatePinnedToCore(can_bus_task, "can_bus_task", 1024 * 5, NULL, 1, NULL, 1);
    Serial.println(F("CAN BUS ISO Standard Task Created"));
  }

  // RPM Lights Demo
  // xTaskCreatePinnedToCore(demo_rpm_lights, "demo_rpm_lights", 1024 * 5, NULL, 3, NULL, 1);
  Serial.println(F("Core Tasks Created"));
  Serial.println(F("Setup Complete"));
}


//-----------------------------
// Main Loop - DO NOT USE!
//-----------------------------

void loop(void) {
  // WARNING: DO NOT POPULATE FUNCTION
}