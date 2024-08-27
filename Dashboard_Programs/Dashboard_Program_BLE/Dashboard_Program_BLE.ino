#define LGFX_USE_V1

#include <Arduino_GFX_Library.h>
#include <Definitions.h>
#include <FS.h>
#include <FastLED.h>
#include <NimBLEDevice.h>
#include <SD.h>
#include <SPI.h>
#include <mcp2515.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "ui.h"

class LGFX : public lgfx::LGFX_Device {

    lgfx::Panel_ILI9341     _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;
    lgfx::Touch_XPT2046          _touch_instance;

  public:

    LGFX(void) {
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

// CAN BUS Data

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


// For the bluetooth GPS module

// BLE UUIDs
static BLEUUID UART_service_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID TX_characteristic_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

// Configuration
const int outputFrequencyHzSerial = 8; // Hz
const unsigned long outputIntervalMs_serial = 1000 / outputFrequencyHzSerial;

static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static bool updated_RaceBox_Data_Message = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myRaceBox;

unsigned long lastOutputTimeSerial = 0;
unsigned long lastOutputTimeOLED = 0;

// Global variables for live data from RaceBox
uint16_t header;
uint8_t messageClass;
uint8_t messageId;
uint16_t payloadLength;
uint16_t year;
uint8_t month;
uint8_t day;
uint8_t hour;
uint8_t minute;
uint8_t second;
uint8_t fixStatusFlags;
uint8_t fixStatus;
uint8_t numSVs;
int32_t longitude;
int32_t latitude;
int32_t wgsAltitude;
int32_t mslAltitude;
uint32_t speed;
uint32_t heading;
int16_t gForceX;
int16_t gForceY;
int16_t gForceZ;
int16_t rotRateX;
int16_t rotRateY;
int16_t rotRateZ;

float headingDegrees;
String compass_direction;

//----------------
// Setup Functions
//----------------

void setup_leds() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  setRPMLights(0);
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

//----------------
// CAN BUS
//----------------

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

void my_disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p ) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t * )&color_p->full, w * h, true) ;
  tft.endWrite();

  lv_disp_flush_ready( disp_drv );
}

void my_touchpad_read( lv_indev_drv_t * indev_drv, lv_indev_data_t * data ) {
  uint16_t touchX, touchY;

  bool touched = tft.getTouch( &touchX, &touchY, 600 );

  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;

    Serial.print("Data x ");
    Serial.println(touchX);

    Serial.print("Data y ");
    Serial.println(touchY);
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
// New Bluetooth GPS Function
//-----------------------------

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (length >= 80) {
    parsePayload(pData);
  } else {
    Serial.println(F("payload length is less than 80 bytes. 80 bytes would be expected for a RaceBox data message."));
    Serial.println(F("For other messages, the payload can be shorter."));
    parsePayload(pData);
  }
}

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
      Serial.print(F("Advertised BLE Device found: "));
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

String getCompassDirection(float headingDegrees) {
  if (headingDegrees >= 337.5 || headingDegrees < 22.5) return "N";
  if (headingDegrees >= 22.5 && headingDegrees < 67.5) return "NO";
  if (headingDegrees >= 67.5 && headingDegrees < 112.5) return "O";
  if (headingDegrees >= 112.5 && headingDegrees < 157.5) return "SO";
  if (headingDegrees >= 157.5 && headingDegrees < 202.5) return "S";
  if (headingDegrees >= 202.5 && headingDegrees < 247.5) return "SW";
  if (headingDegrees >= 247.5 && headingDegrees < 292.5) return "W";
  if (headingDegrees >= 292.5 && headingDegrees < 337.5) return "NW";
  return ""; //Default case, shouldn't be reached
}

void calculateChecksum(uint8_t* data, uint16_t length, uint8_t& CK_A, uint8_t& CK_B) {
  CK_A = 0;
  CK_B = 0;
  for (int i = 2; i < length - 2; i++) { //start after header bytes and end before checksum bytes
    CK_A += data[i];
    CK_B += CK_A;
  }
}

void parsePayload(uint8_t* data) {
  // Check for correct frame start - may need to be removed or changed for other data than RaceBox Data Message!
  if (data[0] != 0xB5 || data[1] != 0x62) {
    // Serial.println("Invalid frame start of payload data - check may need to be removed or changed for other data than RaceBox Data Message!");
    return;
  }

  // Extract data from payload, at first we need the payloadLength to calculate the checksum
  header = *(reinterpret_cast<uint16_t*>(data));                  // 0xB5 0x62 (that are the two header identification bytes, according to RaceBox datasheet: The first 2 bytes are the frame start - always 0xB5 and 0x62.)
  messageClass = *(reinterpret_cast<uint8_t*>(data + 2));         // Expecting 0xFF for a RaceBox data message
  messageId = *(reinterpret_cast<uint8_t*>(data + 3));            // Expecting 0x01 for a RaceBox data message (equals 0x1)
  payloadLength = *(reinterpret_cast<uint16_t*>(data + 4));       // e.g. 0x50 0x00 (80 bytes for live data - attention: other data is larger (up to 509 bytes), divided into multiple packets and needs to be reassembled from multiple packets - refer to datasheet)

  // Validate the length of the packet
  uint16_t packetLength = 6 + payloadLength + 2; //header (6 bytes) + payload + checksum (2 bytes)
  if (packetLength > 512) { // Double check if 5
    // Serial.print("Received packet size exceeds maximum allowed size (512 bytes). ");
    // Serial.print("Packet length is ");
    // Serial.print(packetLength);
    // Serial.println(" bytes.");
    return;
  }

  // Validate checksum
  uint8_t CK_A, CK_B;
  calculateChecksum(data, packetLength, CK_A, CK_B);
  if (data[packetLength - 2] != CK_A || data[packetLength - 1] != CK_B) {
    Serial.println("*** Checksum validation of incoming data package failed. ***");
    return;
  }

  //check if the message class and ID match the expected values for a live data packet
  if (messageClass == 0xFF || messageId == 0x01) {// In case we receive live data (standard on start of RaceBox) and interpret it accordingly
    parse_RaceBox_Data_Message_payload(data); // Sending variable data to this function to interpret it
  } else { // In case we receive different data class
    // Serial.print("unknown message class and message ID found (it may be other data?): ");
    // Serial.print("Message Class: 0x");
    // Serial.print(messageClass, HEX);
    // Serial.print(", Message ID: 0x");
    // Serial.println(messageId, HEX);
    // Serial.println("Ignoring packet. This is not a known/implemented data packet.");
    return;
  }

}

void parse_RaceBox_Data_Message_payload(uint8_t* data) { //function to handle payload of a RaceBox Data Message
  year = *(reinterpret_cast<uint16_t*>(data + 10));               //e.g 0xE6 0x07 (2022) or 0xE8 0x07 (2024)
  month = *(reinterpret_cast<uint8_t*>(data + 12));               //0x01 (january) or 0x08 (august)
  day = *(reinterpret_cast<uint8_t*>(data + 13));                 //0x0A (10th) or 0x08 (8th)
  hour = *(reinterpret_cast<uint8_t*>(data + 14));                //0x08 (08 o'clock)
  minute = *(reinterpret_cast<uint8_t*>(data + 15));              //0x33 (51 min)
  second = *(reinterpret_cast<uint8_t*>(data + 16));              //0x08 (08 seconds)
  fixStatus = *(reinterpret_cast<uint8_t*>(data + 26));           //0x03 (3D Fix)
  fixStatusFlags = *(reinterpret_cast<uint8_t*>(data + 27));      //0x01 (GNSS Fix OK)
  numSVs = *(reinterpret_cast<uint8_t*>(data + 29));              //0x0B (11 satellites)
  longitude = *(reinterpret_cast<int32_t*>(data + 30));           //0xC693E10D (23.2887238 degrees)
  latitude = *(reinterpret_cast<int32_t*>(data + 34));            //0x3B376F19 (42.6719035 degrees)
  wgsAltitude = *(reinterpret_cast<int32_t*>(data + 38));         //0x618C0900 (625.761 meters)
  mslAltitude = *(reinterpret_cast<int32_t*>(data + 42));         //0x0F010900 (590.095 meters)
  speed = *(reinterpret_cast<uint32_t*>(data + 54));              //0x23000000 (35 mm/s = 0.126 km/h)
  heading = *(reinterpret_cast<uint32_t*>(data + 58));            //0x00000000 (0 degrees)
  gForceX = *(reinterpret_cast<int16_t*>(data + 74));             //0xFDFF (-0.003 g)
  gForceY = *(reinterpret_cast<int16_t*>(data + 76));             //0x7100 (0.113 g)
  gForceZ = *(reinterpret_cast<int16_t*>(data + 78));             //0xCE03 (0.974 g)

  headingDegrees = heading / 100000.0; //convert it to a float variable that is needed for the function getCompassDirection
  compass_direction = getCompassDirection(headingDegrees); //generate human readable compass_direction like N, NW, SW etc. from the heading degrees and save them in String 'compass_direction'
  updated_RaceBox_Data_Message = true; //bool is used to determine if updated data for variables in RaceBox Data Message is available (e.g. to print or display them in void loop() )
}

bool connectToRaceBox() {
  NimBLEClient* pClient = nullptr;

  // Check if there's an existing client that matches the address
  if (NimBLEDevice::getClientListSize()) {
    pClient = NimBLEDevice::getClientByPeerAddress(myRaceBox->getAddress());
    if (pClient) {
      if (!pClient->connect(myRaceBox)) {
        // Serial.println("Failed to reconnect. Retrying...");
        return false;
      }
    } else {
      // Create a new client if none matches
      pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(new ClientCallbacks(), false);
      if (!pClient->connect(myRaceBox)) {
        Serial.println(F("Failed to connect."));
        NimBLEDevice::deleteClient(pClient);
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
      return false;
    }
  }

  // Obtain the service and characteristic
  BLERemoteService* pService = pClient->getService(UART_service_UUID);
  if (pService != nullptr) {
    pRemoteCharacteristic = pService->getCharacteristic(TX_characteristic_UUID);
    if (pRemoteCharacteristic != nullptr) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      return true;
    }
  }

  return false;
}

void setup_bluetooth_gps() {
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
  // pScan->start(5, false); // scan for 5 s
  pScan->start(0, false); // scan indefinitely until we stop it manually

}

void print_RaceBox_Data_message_payload_to_serial() {

  unsigned long currentTime = millis();
  if (currentTime - lastOutputTimeSerial >= outputIntervalMs_serial) {

    Serial.println(F("--- Updated Data from RaceBox ---"));
    Serial.println("Date: " + String(day) + "/" + String(month) + "/"  + String(year) + " Time (UTC): " + String(hour) + ":" + String(minute) + ":" + String(second));

    // Output fix status with interpretation
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

    Serial.println("GPS: " + fixStatusText + " Number of Satellites: " + String(numSVs));
    Serial.println("Latitude: " + String(latitude / 1e7, 7) + " Longitude: " + String(longitude / 1e7, 7) + " deg");
    Serial.println("WGS Altitude: " + String(wgsAltitude / 1000.0, 2) + " m" + " MSL Altitude: " + String(mslAltitude / 1000.0, 2) + " m");

    Serial.println("Speed: " + String((speed / 1000.0) / 0.44704, 2) + " mph");

    if ((fixStatusFlags & 0x20) == false) {
      Serial.println(F("Heading NOT valid - may need movement to become valid"));
    } else {
      Serial.print("Heading: " + String(heading / 1e5, 1) + " deg ");
      Serial.print("Heading: " + String(headingDegrees, 1));
      Serial.print(F(" deg, compass direction: "));
      Serial.println(compass_direction);
    }

    Serial.println("G-Force X: " + String(gForceX / 1000.0, 3) + " Y: " + String(gForceY / 1000.0, 3) +  " Z: " + String(gForceZ / 1000.0, 3) + " G");
    Serial.println("Rotation Rate X: " + String(rotRateX / 100.0, 2) + " Y: " + String(rotRateY / 100.0, 2) + " Z: " + String(rotRateZ / 100.0, 2) + " deg/s");

    Serial.println();

  } else {
    Serial.println(F("skipping serial output due to set serial update limitation"));
  }

}

void get_bluetooth_gps_data() {

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

//-----------------------------
// Setup
//-----------------------------

void setup(void) {
  Serial.begin(115200);
  Serial.println("\nSetting up Dashboard");
  setup_can_bus();
  setup_sd_card();
  setup_bluetooth_gps();
  setup_leds();
  setup_display();
  Serial.println("Dashboard Setup Complete\n");

}

//-----------------------------
// Loop
//-----------------------------

void loop(void) {
  long start = micros();

  // lv_timer_handler(); /* let the GUI do its work */
  // Serial.println("Lv timer complete");

  get_bluetooth_gps_data();
//  Serial.println("BLE GPS Data complete");

  get_can_bus_data();
//  Serial.println("Can bus data complete");

  // update_display_data();
  // Serial.println("Update display complete");

  report_loop_duration(start);
}
