#include "NimBLEDevice.h"
#include <SPI.h>
#include <math.h>
#include <mcp2515.h>          
#include <FastLED.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "ui.h"

#define LGFX_USE_V1

#define NUM_LEDS 10
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#define CANBUS_CS 40

struct can_frame canMsg;
MCP2515 mcp2515(CANBUS_CS);

CRGBArray<NUM_LEDS> leds;

SemaphoreHandle_t gui_mutex;

//################### important setting #########################

//example how to define a certain address so that we only connect to a distinguished RaceBox that matches this address:   #define TARGET_DEVICE_ADDRESS "aa:bb:cc:dd:ee:ff"
//replace the actual address below with the one that matches your device (you will see the address printed in the serial output of this code while it is scanning for bluetooth devices on start)

//#define TARGET_DEVICE_ADDRESS "aa:bb:cc:dd:ee:ff"

//alternatively, you may comment the line above like this: //#define TARGET_DEVICE_ADDRESS "aa:bb:cc:dd:ee:ff".
//--> In that case the code will connect to any RaceBox (or to be clear: to any device that advertises a name beginning with 'RaceBox'), as long as the advertised service UUID also matches the requirement.

//##############################################################

//device type, will be set automatically:  0: RaceBox Mini/Mini S, 1: RaceBox Micro - used to handle different battery status decoding between racebox mini/mini s and micro.
int deviceType = -1; //-1: unknown device type as default, the statement 'if (deviceName.rfind("RaceBox Micro", 0) == 0) {' and the following lines in class AdvertisedDeviceCallbacks automatically determines the device type.

NimBLEClient* pClient = nullptr;

// BLE UUIDs
static BLEUUID UART_service_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID RX_characteristic_UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"); //currently not used in this code example
static BLEUUID TX_characteristic_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID NMEA_UART_service_UUID("00001101-0000-1000-8000-00805F9B34FB"); //currently not used in this code example
static BLEUUID NMEA_RX_characteristic_UUID("00001102-0000-1000-8000-00805F9B34FB"); //currently not used in this code example
static BLEUUID NMEA_TX_characteristic_UUID("00001103-0000-1000-8000-00805F9B34FB"); //currently not used in this code example
//static BLEUUID Serial_Number_Characteristic_UUID ("00002a25-0000-1000-8000-00805f9b34fb"); //individual serial number UUID - currently not used in this code example

// Configuration
const int outputFrequencyHzSerial = 1; //in Hz
const unsigned long outputIntervalMs_serial = 1000 / outputFrequencyHzSerial; 

static bool doConnect = false;
static bool bleRequestDisconnect = false;
static bool connected = false;
static bool doScan = false;
static bool updated_RaceBox_Data_Message = false; //used to determine if we have new live data to print
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myRaceBox;

unsigned long lastOutputTimeSerial = 0;
unsigned long lastOutputTimeOLED = 0;

//global variables for live data from RaceBox (at 25Hz): (examples see function void parsePayload)
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

float headingDegrees;
String compass_direction;

class LGFX : public lgfx::LGFX_Device
{

  lgfx::Panel_ILI9488     _panel_instance;
  lgfx::Bus_Parallel8  _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_FT5x06           _touch_instance;


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

      cfg.pin_cs           =   38;
      cfg.pin_rst          =   21;
      cfg.pin_busy         =   -1;

      cfg.memory_width     =   320;
      cfg.memory_height    =   480;

      cfg.panel_width      =   320;
      cfg.panel_height     =   480;
      // cfg.offset_x         =   0;
      // cfg.offset_y         =   0;
      cfg.offset_x         =   0;
      cfg.offset_y         =   0;
      cfg.offset_rotation  =   0;
      cfg.dummy_read_pixel =   8;
      cfg.dummy_read_bits  =   1;
      cfg.readable         = true;
      cfg.invert           = true;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;

      _panel_instance.config(cfg);
    }

    { 
      auto cfg = _light_instance.config();

      cfg.pin_bl = 6;
      cfg.invert = false;
      cfg.freq   = 2000; //set a slower frequency for LED driver CE
      cfg.pwm_channel = 7;

      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    {
      auto cfg = _touch_instance.config();

      cfg.x_min      = 0;
      cfg.x_max      = 320;
      cfg.y_min      = 0;
      cfg.y_max      = 480;
      cfg.pin_int    = -1;
      cfg.bus_shared = true;
      cfg.offset_rotation = 0;

      cfg.i2c_port = 1;
      cfg.i2c_addr = 0x38;
      cfg.pin_sda  = 36;
      cfg.pin_scl  = 37;
      cfg.freq = 100000;

      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if (length >= 80) {
        parsePayload(pData);
    } else {
        Serial.println("payload length is less than 80 bytes. 80 bytes would be expected for a RaceBox data message.");
        Serial.println("For other messages, the payload can be shorter.");
        //to allow that, we still send the data to parsePayload(pData):
        parsePayload(pData);
    }
}

// Callback for when a connection is established
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) {
    connected = true;
    Serial.println("RaceBox Connected!");
  }

  void onDisconnect(NimBLEClient* pClient) {
    connected = false;
    Serial.println("Disconnected from RaceBox!");
    Serial.println("HINT: disconnects can happen if excessive serial output (especially in void parsePayload and functions like void parse_RaceBox_Data_Message_payload etc. delay the code execution.");
    Serial.println("Trying to reconnect...");
    doConnect = true;
  }
};

// Scan callback function
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) { //check preconditions for connection to found Bluetooth devices (does it advertise to be Named RaceBox? If set, does it have the correct TARGET_DEVICE_ADDRESS? 
        Serial.print("Advertised BLE Device found: ");
        Serial.println(advertisedDevice->toString().c_str());
    
        // We have found a device with the correct service UUID, set doConnect to true
        if (advertisedDevice->isAdvertisingService(UART_service_UUID)) {
        // Check if the device name starts with "RaceBox"
        std::string deviceName = advertisedDevice->getName();
        if (deviceName.rfind("RaceBox", 0) == 0) { //rfind returns 0 if we find "RaceBox" at the beginning of advertised device name. If you have problems here double check case (other RaceBoxes *could* be named "Racebox" or "racebox")
            
            //setting of device type:  0: RaceBox Mini/Mini S, 1: RaceBox Micro - used to handle different battery status decoding between racebox mini/mini s and micro.
            if (deviceName.rfind("RaceBox Micro", 0) == 0) {               
              deviceType=1;   
            }
            else if (deviceName.rfind("RaceBox Mini", 0) == 0) { //setting of device type:  0: RaceBox Mini/Mini S, 1: RaceBox Micro - used to handle different battery status decoding between racebox mini/mini s and micro.
              deviceType=0;   
            }
            else{
              deviceType=-1;
            }
            
            // If TARGET_DEVICE_ADDRESS is defined, check for a specific address - that is something YOU may or may not define in the beginning of the code - if defined, we will only connect to a specific address. if not set, we'll connect to an RaceBox.
            #ifdef TARGET_DEVICE_ADDRESS
            std::string deviceAddress = advertisedDevice->getAddress().toString();
            if (deviceAddress == TARGET_DEVICE_ADDRESS) {
                Serial.printf("RaceBox found with address %s - matching TARGET_DEVICE_ADDRESS as set in code. Trying to connect...\n", deviceAddress.c_str());
                NimBLEDevice::getScan()->stop();  // Stop scanning
                myRaceBox = advertisedDevice;
                doConnect = true;
            } else {
                Serial.printf("Device name starts with RaceBox but address %s does not match the TARGET_DEVICE_ADDRESS set in code.\n", deviceAddress.c_str());
                Serial.printf("Comment out the definition of TARGET_DEVICE_ADDRESS in code (to connect to any device named RaceBox) or set TARGET_DEVICE_ADDRESS to an address that matches your device.\n\n");
                Serial.printf("##### We are therefore NOT connecting to the device %s although it seems to be a RaceBox #####\n", deviceAddress.c_str());
            }
            #else
            // If no specific address is defined, connect to any device whose name starts with "RaceBox"
            Serial.println("RaceBox found. TARGET_DEVICE_ADDRESS is not set in code (or commented out ), so we connect to any RaceBox that we find.");
            NimBLEDevice::getScan()->stop();  // Stop scanning
            Serial.println("stopped bluetooth scanning.");
            Serial.printf("Connecting to RaceBox with address %s.... \n", advertisedDevice->getAddress().toString().c_str());
            myRaceBox = advertisedDevice;
            doConnect = true;
            #endif
        }
     }
    }

};


//Function to get compass direction from heading
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

//decode battery status
void decodeBatteryStatus(uint8_t batteryStatus) {
    if (deviceType == 0) { //RaceBox Mini or RaceBox Mini S
        bool isCharging = (batteryStatus & 0x80) != 0; //check if the MSB is set (charging status)
        uint8_t batteryLevel = batteryStatus & 0x7F;   //battery level (remaining 7 bits)
        Serial.print("RaceBox Mini/Mini S - "); //interpreting battery status according to datasheet
        Serial.print("Charging Status: ");
        Serial.println(isCharging ? "Charging" : "Not Charging");
        Serial.print("Battery Level: ");
        Serial.print(batteryLevel);
        Serial.println("%");
    } else if (deviceType == 1) { //RaceBox Micro
        Serial.print("RaceBox Micro - "); //interpreting battery status as input voltage (according to datasheet)
        float inputVoltage = batteryStatus / 10.0; //Input voltage must be multiplied by 10, according to datasheet
        Serial.print("Input Voltage: ");
        Serial.print(inputVoltage, 1); //print with one decimal
        Serial.println(" V");
    } else {
        Serial.println("Battery status: Unknown device type");
    }
}

void calculateChecksum(uint8_t* data, uint16_t length, uint8_t& CK_A, uint8_t& CK_B) {
    CK_A = 0;
    CK_B = 0;
    for (int i = 2; i < length - 2; i++) { //start after header bytes and end before checksum bytes
        CK_A += data[i];
        CK_B += CK_A;
    }
}


void parsePayload(uint8_t* data) {  //handle incoming data. This function is triggered with each incoming packet. Note that any serial output here will directly be sent when the function runs and can interfere with output from void loop, as this function runs when data packets come in.
//Serial.println("++++++++++++++ received data packet ++++++++++++++"); //any excessive serial output in the parsing functions can lead to disconnects. remember that when experiencing unexplainable disconnects!
/* Explanation of how we interpret the data that we got from racebox:
 * The data buffer (named 'data' here) is a pointer (indicated by the *) to the beginning of a data payload in memory. generally spoken,
 * uint8_t* data is a pointer to a byte array (buffer) that holds the raw data received from our RaceBox via bluetooth low energy (BLE).
 * This buffer contains a sequence of bytes that contain the different values, in case of the live data it contains values like GPS coordinates, speed, accelerations and so on.
 * The datasheet (provided by the developers) has a detailed description which byte represents which value.
 * each time we receive such a data packet, we need do go through it and basically disassemble it to cut out the pieces of information that we need. 
 * we do not necessarily need to use all information, we also could pick out just one value that is of interest for us.
 * But for convenience, i added all of them, at least for the data packet that contains live sensor data.
 * Interpreting Bytes:
 * we need to extract values from the data buffer by interpreting different byte sequences as different data types (e.g., uint16_t, uint32_t). This is done by using pointer arithmetic and type casting.
 * Pointer Arithmetic:
 * You use pointer arithmetic to "jump" to the correct position in the buffer. For example, if a data field starts at a certain offset from the beginning of the buffer, you add that offset to 
 * the base pointer (data). That's what we find in the racebox datasheet.
 * Type Casting:
 * The reinterpret_cast is used to cast the pointer to the appropriate type (uint16_t, uint32_t, etc.) to interpret the bytes at that position correctly.
 * because I already defined the variables globally, we now just assign the new values to them. 
 * 
 * Example using words: *data contains:  Header2bMessageClassId2bPayloadlenght2biTOW4bYear2bMonth1bDay1b...... etc etc
 * which we want to divide like this:    Header2b MessageClassId2b Payloadlenght2b  iTOW4b   Year2b   Month1b   Day1b  ......etc etc
 *                                       2 bytes      2 bytes         2 bytes       4 bytes  2 bytes  2 bytes   1 byte ...... etc etc
 *                                       
 * if we were interested in iTOW, that data content would begin at the seventh byte, but as we start counting at 0, it is offset 6:                                      
 * some examples picked from datasheet:
 * Offset    Contents         Field                           Decoded
 * ------------------------------------------------------------------------
 * 6         A0 E7 0C 07      iTOW                            118286240
 * 29        0B               Number of SpaceVehicles (SVs)   11 satellites
 * 78        CE 03            GForceZ                         0.974 G
 * 
 * lets assume we are interested in the number of satellites that racebox sees.
 * that means that in our 80 byte long message, the number of satellites that racebox "sees" is at byte position 29 (see example excerpt from datasheet above).
 * they call it 'number of space vehicles' so we also name the variable similarly 'numSVs'. We now "pick" out the byte at position 29 and assign it to a variable of appropiate type:
 * numSVs = *(reinterpret_cast<uint8_t*>(data + 29));     
 * As counting starts at 0, the last byte containing live data is at position 79 (gForceZ).
 * 
 * generally, this function 'void parsePayload' is called each time we receive new data from our RaceBox,
 * So if the data comes in 25 times per second, we update the variables just as often (if that kind of data message is implemented).
 */

    //check for correct frame start - may need to be removed or changed for other data than RaceBox Data Message!
    if (data[0] != 0xB5 || data[1] != 0x62) {
        Serial.println("Invalid frame start of payload data - check may need to be removed or changed for other data than RaceBox Data Message!");
        return;
    }

    
//Extract data from payload, at first we need the payloadLength to calculate the checksum
                                                                    //examples for data content, mostly copied from RaceBox datasheet:
    header = *(reinterpret_cast<uint16_t*>(data));                  //0xB5 0x62 (that are the two header identification bytes, according to RaceBox datasheet: The first 2 bytes are the frame start - always 0xB5 and 0x62.)
    messageClass = *(reinterpret_cast<uint8_t*>(data + 2));         //expecting 0xFF for a RaceBox data message
    messageId = *(reinterpret_cast<uint8_t*>(data + 3));            //expecting 0x01 for a RaceBox data message (equals 0x1)
    payloadLength = *(reinterpret_cast<uint16_t*>(data + 4));       //e.g. 0x50 0x00 (80 bytes for live data - attention: other data is larger (up to 509 bytes), divided into multiple packets and needs to be reassembled from multiple packets - refer to datasheet)


    //validate the length of the packet
    uint16_t packetLength = 6 + payloadLength + 2; //header (6 bytes) + payload + checksum (2 bytes)
    if (packetLength > 512) { //double check if 5
        Serial.print("Received packet size exceeds maximum allowed size (512 bytes). ");
        Serial.print("Packet length is ");
        Serial.print(packetLength);
        Serial.println(" bytes.");
        return;
    } else { //if packetLength is within allowed limits, print out some info on the data packet:
        //Serial.println("Payload length is " + String(payloadLength) + " bytes");
        //Serial.println("Expected payload length according to datasheet: 0 - 504 bytes. For a RaceBox Data Message payload length is 80 bytes.");
        //Serial.println("Packet length (including checksum) is " + String(packetLength) + " bytes");
    }
    
    //validate checksum
    uint8_t CK_A, CK_B;
    calculateChecksum(data, packetLength, CK_A, CK_B);
    if (data[packetLength - 2] != CK_A || data[packetLength - 1] != CK_B) {
        Serial.println("*** Checksum validation of incoming data package failed. ***");
        return;
    } else {
        //Serial.println("Checksum validation successful.");
    }
    //Serial.println();

    //print message class and message ID - used to determine the type of message. A RaceBox Data Message has messageClass 0xFF and messageId 0x01.
//    Serial.print("Message Class: 0x");
//    Serial.println(messageClass, HEX);
//    Serial.print("Message ID: 0x");
//    Serial.println(messageId, HEX);

    //check if the message class and ID match the expected values for a live data packet
    if (messageClass == 0xFF || messageId == 0x01) {//in case we receive live data (standard on start of RaceBox) and interpret it accordingly
      //Serial.println("the received message has messageClass 0xFF and messageId 0x01, this is a (valid) RaceBox Data Message. Parsing payload.");
      parse_RaceBox_Data_Message_payload(data); //sending variable data to this function to interpret it
      //outputting received data (this will be triggered each time a payload is parsed, so be aware that it may delay data update rate if e.g. printing a lot of info to serial takes longer than it takes for the next data to arrive.)
//    print_RaceBox_Data_message_payload_to_oled(); 
//      
//      //####### be aware that those serial outputs below are sent each time a payload is parsed (i.e. up to 25x per second) like an interrupt, so the serial output from this function can overlay/interfere with output from void loop ####### 
//    print_RaceBox_Data_message_payload_to_serial(); //<-- ATTENTION! see comment above
//      //if you move this function to loop, you may want to comment out all serial outputs from void parsePayload.
    }
    
    //examples how to handle other received messages;
    else if (messageClass == 0xFF || messageId == 0x21) {//History Data Message
      Serial.println("the received message has messageClass 0xFF and messageId 0x21, this is a (valid) History Data Message message. Parsing payload NOT yet implemented.");
      //parse_History_Data_Message_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    else if (messageClass == 0xFF || messageId == 0x22) {//Standalone Recording Status
      Serial.println("the received message has messageClass 0xFF and messageId 0x22, this is a (valid) Standalone Recording Status message. Parsing payload NOT yet implemented.");
      //parse_standalone_Recording_Status_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    else if (messageClass == 0xFF || messageId == 0x23) {//Recorded Data Download
      Serial.println("the received message has messageClass 0xFF and messageId 0x23, this is a (valid) Recorded Data Download message. Parsing payload NOT yet implemented.");
      //parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    else if (messageClass == 0xFF || messageId == 0x26) {//Standalone Recording State Change Message
      Serial.println("the received message has messageClass 0xFF and messageId 0x26, this is a (valid) Standalone Recording State Change Message. Parsing payload NOT yet implemented.");
      //parse_Recorded_Data_payload(data); //sending variable data to this function to interpret it  (function not yet implemented)
    }
    
//    else if (messageClass == 0x_something_else_1 || messageId == 0x_something_else_2){
//      //handle other message class(es) like this
//    }

    else{ //in case we receive different data (with different message class or message IDs as implemented above, we would need to handle it differently, or even assemble multiple messages that may have ben split.
      Serial.print("unknown message class and message ID found (it may be other data?): ");
      Serial.print("Message Class: 0x");
      Serial.print(messageClass, HEX);
      Serial.print(", Message ID: 0x");
      Serial.println(messageId, HEX);       
      Serial.println("Ignoring packet. This is not a known/implemented data packet. Interpreting the payload for this kind of packet is not yet implemented.");
      return;
    }

    

}


//functions to interpret payload of different messages:


void parse_RaceBox_Data_Message_payload(uint8_t* data){ //function to handle payload of a RaceBox Data Message
    //writing updated values (from payload) to the variables:
    iTOW = *(reinterpret_cast<uint32_t*>(data + 6));                //e.g 0xA0 0xE7 0x0C 0x07
    year = *(reinterpret_cast<uint16_t*>(data + 10));               //e.g 0xE6 0x07 (2022) or 0xE8 0x07 (2024)
    month = *(reinterpret_cast<uint8_t*>(data + 12));               //0x01 (january) or 0x08 (august)
    day = *(reinterpret_cast<uint8_t*>(data + 13));                 //0x0A (10th) or 0x08 (8th)
    hour = *(reinterpret_cast<uint8_t*>(data + 14));                //0x08 (08 o'clock)
    minute = *(reinterpret_cast<uint8_t*>(data + 15));              //0x33 (51 min)
    second = *(reinterpret_cast<uint8_t*>(data + 16));              //0x08 (08 seconds)
    validityFlags = *(reinterpret_cast<uint8_t*>(data + 17));       //0x37 (Date/Time valid)
    timeAccuracy = *(reinterpret_cast<uint32_t*>(data + 18));       //0x19000000 (25 ns)
    nanoseconds = *(reinterpret_cast<uint32_t*>(data + 22));        //0x2AAD4D0E (239971626 ns = 0.239 seconds)
    fixStatus = *(reinterpret_cast<uint8_t*>(data + 26));           //0x03 (3D Fix)
    fixStatusFlags = *(reinterpret_cast<uint8_t*>(data + 27));      //0x01 (GNSS Fix OK)
    dateTimeFlags = *(reinterpret_cast<uint8_t*>(data + 28));       //0xEA (Date/Time Confirmed)
    numSVs = *(reinterpret_cast<uint8_t*>(data + 29));              //0x0B (11 satellites)
    longitude = *(reinterpret_cast<int32_t*>(data + 30));           //0xC693E10D (23.2887238 degrees)
    latitude = *(reinterpret_cast<int32_t*>(data + 34));            //0x3B376F19 (42.6719035 degrees)
    wgsAltitude = *(reinterpret_cast<int32_t*>(data + 38));         //0x618C0900 (625.761 meters)
    mslAltitude = *(reinterpret_cast<int32_t*>(data + 42));         //0x0F010900 (590.095 meters)
    horizontalAccuracy = *(reinterpret_cast<uint32_t*>(data + 46)); //0x9C030000 (0.924 meters)
    verticalAccuracy = *(reinterpret_cast<uint32_t*>(data + 50));   //0x2C070000 (1.836 meters)
    speed = *(reinterpret_cast<uint32_t*>(data + 54));              //0x23000000 (35 mm/s = 0.126 km/h)
    heading = *(reinterpret_cast<uint32_t*>(data + 58));            //0x00000000 (0 degrees)
    speedAccuracy = *(reinterpret_cast<uint32_t*>(data + 62));      //0xD0000000 (208 mm/s = 0.704 km/h)
    headingAccuracy = *(reinterpret_cast<uint32_t*>(data + 66));    //0x88A9DD00 (145.26856 degrees)
    pdop = *(reinterpret_cast<uint16_t*>(data + 70));               //0x2C01 (3)
    latLonFlags = *(reinterpret_cast<uint8_t*>(data + 72));         //0x00 (Coordinates valid)
    batteryStatus = *(reinterpret_cast<uint8_t*>(data + 73));       //has to be interpreted depending on if it is a RaceBox micro or mini, see my function void decodeBatteryStatus
    gForceX = *(reinterpret_cast<int16_t*>(data + 74));             //0xFDFF (-0.003 g)
    gForceY = *(reinterpret_cast<int16_t*>(data + 76));             //0x7100 (0.113 g)
    gForceZ = *(reinterpret_cast<int16_t*>(data + 78));             //0xCE03 (0.974 g)
    
    headingDegrees = heading / 100000.0; //convert it to a float variable that is needed for the function getCompassDirection
    compass_direction = getCompassDirection(headingDegrees); //generate human readable compass_direction like N, NW, SW etc. from the heading degrees and save them in String 'compass_direction'
    updated_RaceBox_Data_Message = true; //bool is used to determine if updated data for variables in RaceBox Data Message is available (e.g. to print or display them in void loop() )
}


////function to connect to RaceBox via BLE
//bool connectToRaceBox() {
//  NimBLEClient* pClient = nullptr;
//
//  if (NimBLEDevice::getClientListSize()) {
//    pClient = NimBLEDevice::getClientByPeerAddress(myRaceBox->getAddress());
//    if (pClient) {
//      if (!pClient->connect(myRaceBox)) {
//        Serial.println("Failed to reconnect. Retrying...");
//        Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
//        return false;
//      }
//    } else {
//      pClient = NimBLEDevice::createClient();
//      pClient->setClientCallbacks(new ClientCallbacks(), false);
//      if (!pClient->connect(myRaceBox)) {
//        Serial.println("Failed to connect.");
//        NimBLEDevice::deleteClient(pClient);
//        //Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
//        return false;
//      }
//    }
//  } else {
//    pClient = NimBLEDevice::createClient();
//    pClient->setClientCallbacks(new ClientCallbacks(), false);
//    if (!pClient->connect(myRaceBox)) {
//      Serial.println("Failed to connect.");
//      NimBLEDevice::deleteClient(pClient);
//      Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
//      return false;
//    }
//  }
//
//  //obtain the service and characteristic
//  BLERemoteService* pService = pClient->getService(UART_service_UUID);
//  if (pService != nullptr) {
//    pRemoteCharacteristic = pService->getCharacteristic(TX_characteristic_UUID);
//    if (pRemoteCharacteristic != nullptr) {
//      pRemoteCharacteristic->registerForNotify(notifyCallback);
//      Serial.println("DEBUG: connectToRaceBox() will now return true and exit.");
//      return true;
//    }
//  }
//  Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
//  return false;
//}

// Function to connect to RaceBox via BLE
bool connectToRaceBox() {

  // Check if there's an existing client that matches the address
  if (NimBLEDevice::getClientListSize()) {
    pClient = NimBLEDevice::getClientByPeerAddress(myRaceBox->getAddress());
    if (pClient) {
      if (!pClient->connect(myRaceBox)) {
        Serial.println("Failed to reconnect. Retrying...");
        //Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
        return false;
      }
    } else {
      // Create a new client if none matches
      pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(new ClientCallbacks(), false);
      if (!pClient->connect(myRaceBox)) {
        Serial.println("Failed to connect.");
        NimBLEDevice::deleteClient(pClient);
        //Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
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
      //Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
      return false;
    }
  }

  //obtain the service and characteristic
  BLERemoteService* pService = pClient->getService(UART_service_UUID);
  if (pService != nullptr) {
    pRemoteCharacteristic = pService->getCharacteristic(TX_characteristic_UUID);
    if (pRemoteCharacteristic != nullptr) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      Serial.println("DEBUG: connectToRaceBox() will now return true and exit.");
      return true;
    }
  }

  //if service or characteristic is not found, return false
  //Serial.println("DEBUG: connectToRaceBox() will now return false and exit.");
  return false;
}


static LGFX tft;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ SCREEN_WIDTH * SCREEN_HEIGHT / 10 ];

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
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {

  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;

  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void setup(void)
{

  Serial.begin( 115200 ); /* prepare for possible serial debug */ 

  gui_mutex = xSemaphoreCreateMutex();
  if (gui_mutex == NULL) {
    // Handle semaphore creation failure
    Serial.println("semaphore creation failure");
    return;
  }

  FastLED.addLeds<NEOPIXEL, 4>(leds, NUM_LEDS); 
  FastLED.setBrightness(20);
  //reset 
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
  
//  leds[0] = CRGB::Green;
//  leds[1] = CRGB::Yellow;
//  leds[2] = CRGB::Red;
//  leds[3] = CRGB::Green;
//  leds[4] = CRGB::Yellow;
//  leds[5] = CRGB::Red;
//  leds[6] = CRGB::Green;
//  leds[7] = CRGB::Yellow;
//  leds[8] = CRGB::Red;
//  leds[9] = CRGB::Green;
//  delay(500);
//  FastLED.show();

  delay(250);
  leds[0] = CRGB::Green;
  leds[9] = CRGB::Green;
  FastLED.show();
  delay(250);
  leds[1] = CRGB::Green;
  leds[8] = CRGB::Green;
  FastLED.show();
  delay(250);
  leds[2] = CRGB::Yellow;
  leds[7] = CRGB::Yellow;
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


  leds[0] = CRGB::Green;
  leds[1] = CRGB::Green;
  leds[2] = CRGB::Yellow;
  leds[3] = CRGB::Red;
  leds[4] = CRGB::Red;
  leds[5] = CRGB::Red;
  leds[6] = CRGB::Red;
  leds[7] = CRGB::Yellow;
  leds[8] = CRGB::Green;
  leds[9] = CRGB::Green;
  FastLED.show();


  //buzzer
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for a second
  digitalWrite(5, LOW);    // turn the LED off by making the voltage LOW          
  delay(200);  

  
  
  xTaskCreatePinnedToCore(display_task,
                          "loading_task",
                          1024 * 10, // The size of the task stack specified as the number of bytes
                          NULL,
                          3,  // The priority at which the task should run.
                          NULL,
                          1); // The core to which the task is pinned to, or tskNO_AFFINITY if the task has no core affinity.
                        
  xTaskCreatePinnedToCore(display_update_task,
                          "loading_task",
                          1024 * 3,
                          NULL,
                          2,  
                          NULL,
                          1);

  xTaskCreatePinnedToCore(ble_task,
                          "ble_task",
                          1024 * 10,  
                          NULL,
                          1,  
                          NULL,
                          1);

  xTaskCreatePinnedToCore(sensor_task,
                          "sensor_task",
                          1024 * 5,  
                          NULL,
                          1,  
                          NULL,
                          1);


//  xTaskCreatePinnedToCore(simulation_task,
//                          "simulation_task",
//                          4000,
//                          NULL,
//                          0,
//                          NULL,
//                          1);

  
}

void ui_reset() {
  
  lv_label_set_text(ui_MainScreen_Label_LabelRPM, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelGear, "N");
  lv_label_set_text(ui_MainScreen_Label_LabelSpeed, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelGForce, "0");
  lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Awaiting BLE");

}

void upshifting_blink(){
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

void display_task(void *pvParameters) {

  String LVGL_Arduino = "LVGL Arduino ";
  LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  
  Serial.println( LVGL_Arduino );
  
  tft.init();
  tft.setRotation(1);

  //max brightness at 255
  tft.setBrightness(100);

  lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
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
  //ui_reset();

  //assign callback functions

  //restart button
  lv_obj_add_event_cb(ui_SettingScreen_Button_ButtonRestart, ui_event_SettingScreen_Button_ButtonRestart,LV_EVENT_PRESSED, NULL);
  //BLE disconnect button
  lv_obj_add_event_cb(ui_SettingScreen_Button_ButtonBLEDisconnect, ui_event_SettingScreen_Button_ButtonBLEDisconnect,LV_EVENT_PRESSED, NULL);
  //LED brightness
  lv_obj_add_event_cb(ui_SettingScreen_Slider_SliderLEDBrightness, ui_event_SettingScreen_Slider_SliderLEDBrightness, LV_EVENT_VALUE_CHANGED, NULL);
  //Disp brightness
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
    vTaskDelay(pdMS_TO_TICKS(1));  // Adjust as needed
    
  }
  
}

void display_update_task(void *pvParameters) {

  delay(1000); //wait for display init
  
   //loading screen is first initiated screen
  
  lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, 0, LV_ANIM_OFF);

  delay(2000);

  int count_value = 0;

  for(int i=0;i<100;i++){
    delay(30);
    lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, count_value, LV_ANIM_OFF);
    count_value++;
  }

  lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Connecting..");
  
  lv_scr_load(ui_MainScreen);

  count_value = 0;

  double gX = 0.0;
  double gY = 0.0;
  double gZ = 0.0;
  double g_mag = 0.0;
  
  for(int i=0;i<100;i++){
    delay(30);

    lv_bar_set_value(ui_MainScreen_Bar_BarRPM, map(count_value, 0, 12000, 0, 100), LV_ANIM_OFF);
    lv_label_set_text(ui_MainScreen_Label_LabelRPM, String(count_value).c_str());
    count_value+=100;
    
  }

  while (1) {
    // if racebox connected
    if(connected){

      gX = gForceX / 1000.0;
      gY = gForceY / 1000.0;
      gZ = gForceZ / 1000.0;
      g_mag = sqrt(gX * gX + gY * gY + gZ * gZ);
     
      lv_label_set_text(ui_MainScreen_Label_LabelSpeed, String((speed / 1000.0)*2.23694, 1).c_str()); // conversion to m/s to mph
      lv_label_set_text_fmt(ui_MainScreen_Label_LabelGPSTrack, "GPS Fix: %i", numSVs); // no. of connected satelites
      lv_label_set_text(ui_MainScreen_Label_LabelGForce, String(gX, 2).c_str()); // G force resultant

      
    }else{

      if(bleRequestDisconnect){
        lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Disconnected");
      }else{
        lv_label_set_text(ui_MainScreen_Label_LabelGPSTrack, "GPS: Connecting..");
      }
      
      }
    vTaskDelay(10);

  }

}

void ble_task(void *pvParameters) {
  
  Serial.println("starting scanning for advertisements of Bluetooth devices.");
  
  #ifdef TARGET_DEVICE_ADDRESS
   Serial.printf("Waiting for a specific RaceBox with TARGET_DEVICE_ADDRESS %s to appear (as set in code)...\n", TARGET_DEVICE_ADDRESS); //notice the Serial.printf to be able to print TARGET_DEVICE_ADDRESS to serial.
  #else
   Serial.println("Waiting for any RaceBox to appear (no specific TARGET_DEVICE_ADDRESS is set)...");
  #endif
  
  Serial.println();
  Serial.println("---------------------------------------------  scan results  --------------------------------------------------------");
  Serial.println();
  
  NimBLEDevice::init("ESP32_RaceBox_Client");
  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  pScan->start(0, false); //scan indefinitely until we stop it manually

  while (1) {

    if (doConnect) { //if we have requested to connect to a RaceBox
    //Serial.println("DEBUG: doConnect = true (in void loop) - trying to (re)connect...");
    if (connectToRaceBox()) {
      Serial.println("successfully connected to RaceBox.");
      Serial.println();
      //stop scanning for Bluetooth devices, now that we're connected to our RaceBox
        NimBLEDevice::getScan()->stop();
    } else {
      Serial.println("Failed to connect to RaceBox. Reattempting BLE connection...");
      //NimBLEDevice::getScan()->start(0, false); //scan indefinitely (0) until we stop it manually  //-->this is a simple restart with known scanning parameters
      //better approch: make sure scanning setup is correct, although slightly more complex:
      NimBLEScan* pScan = NimBLEDevice::getScan();
      pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
      pScan->setInterval(45);
      pScan->setWindow(15);
      pScan->setActiveScan(true);
      pScan->start(0, false); //scan indefinitely until we stop it manually
    }
    doConnect = false;
    }
    if (connected) { //if we are connected to RaceBox
      //add your code here
      interpret_serial_input(); //this function listens to serial console inputs from your computer (when a RaceBox is connected, due to the check 'if (connected)'. Sending 1, 2 or 3 will start functions (currently only empty function prototypes are implemented to give a starting point)
      if(updated_RaceBox_Data_Message==true){ //if we have received updated values from a RaceBox data message
        print_RaceBox_Data_message_payload_to_serial();
        updated_RaceBox_Data_Message=false; //reset bool
      }
    }
    else{
      //do something else if not connected to RaceBox
      interpret_serial_input(); //just for debugging purposes, we also interpret serial input here. Can be removed if we only want to start functions that e.g. send data to RaceBox, but can be useful for development so that the functions are called even if no racebox is connected.
    }

  }
}

void sensor_task(void *pvParameters) {
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Set CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                   // Set CAN at normal mode

  while (1) {

    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");
    
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i],HEX);
      Serial.print(" ");
    }

    Serial.println();      
     }
  

  }

}

void simulation_task(void *pvParameters) {

  lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, 0, LV_ANIM_OFF);

  delay(2000);

  int count_value = 0;

  for(int i=0;i<100;i++){
    delay(50);
    lv_bar_set_value(ui_LoadingScreen_Bar_loadingBar, count_value, LV_ANIM_OFF);
    count_value++;
  }

  lv_scr_load(ui_MainScreen);

  //lv_scr_load_anim(ui_MainScreen, LV_SCR_LOAD_ANIM_FADE_ON, 250, 0, true);

  //LED RPM simulation
  
  leds[0] = CRGB::Green;
  delay(500);
  FastLED.show();
  leds[1] = CRGB::Green;
  delay(500);
  FastLED.show();
  leds[2] = CRGB::Green;
  delay(500);
  FastLED.show();
  leds[3] = CRGB::Yellow;
  delay(500);
  FastLED.show();
  leds[4] = CRGB::Yellow;
  delay(500);
  FastLED.show();
  leds[5] = CRGB::Yellow;
  delay(500);
  FastLED.show();
  leds[6] = CRGB::Red;
  delay(500);
  FastLED.show();
  leds[7] = CRGB::Red;
  delay(500);
  FastLED.show();
  leds[8] = CRGB::Red;
  delay(500);
  FastLED.show();
  leds[9] = CRGB::Red;
  delay(500);
  FastLED.show();
  
  for(int i = 0; i<10; i++){
    upshifting_blink();
  }

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

  leds[0] = CRGB::Green;
  delay(250);
  FastLED.show();
  leds[1] = CRGB::Green;
  delay(250);
  FastLED.show();
  leds[2] = CRGB::Green;
  delay(250);
  FastLED.show();
  leds[3] = CRGB::Yellow;
  delay(250);
  FastLED.show();
  leds[4] = CRGB::Yellow;
  delay(250);
  FastLED.show();
  leds[5] = CRGB::Yellow;
  delay(250);
  FastLED.show();
  leds[6] = CRGB::Red;
  delay(250);
  FastLED.show();
  leds[7] = CRGB::Red;
  delay(250);
  FastLED.show();
  leds[8] = CRGB::Red;
  delay(250);
  FastLED.show();
  leds[9] = CRGB::Red;
  delay(250);
  FastLED.show();

  for(int i = 0; i<10; i++){
    upshifting_blink();
  }

int rpm = 0;
int speed_value = 0;
int gear = 1;
int tps = 0; 
int bps = 0;
int g_force = 0;

//init all values from main screen with data to erase artifacts

  // Elements on Display 

  for(int i =0; i<20; i++){
    
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
  lv_label_set_text(ui_MainScreen_Label_LabelRPM, String(rpm).c_str());
  
  // Gear
  lv_label_set_text(ui_MainScreen_Label_LabelGear, String(gear).c_str());
  
  // Speed
  lv_label_set_text(ui_MainScreen_Label_LabelSpeed, String(speed_value).c_str());

 // G-Force
  lv_label_set_text(ui_MainScreen_Label_LabelGForce, String(g_force).c_str());

  delay(500);

  }

  //delay(30000);



  //ESP.restart();


  while (1) {

    vTaskDelay(10);

  }
}

static void ui_event_SettingScreen_Slider_SliderLEDBrightness(lv_event_t * event)
{
  lv_obj_t * slider = lv_event_get_target(event);
  FastLED.setBrightness(map((int)lv_slider_get_value(slider), 0, 100, 0, 255));
  FastLED.show();
}

static void ui_event_SettingScreen_Slider_SliderDisplayBrightness(lv_event_t * event)
{
  lv_obj_t * slider = lv_event_get_target(event);
  tft.setBrightness(map((int)lv_slider_get_value(slider), 0, 100, 5, 255));
}


static void ui_event_SettingScreen_Button_ButtonRestart(lv_event_t * event)
{
    delay(1000);
    ESP.restart();
}

static void ui_event_SettingScreen_Button_ButtonBLEDisconnect(lv_event_t * event)
{ 
    connected = false;
    bleRequestDisconnect = true;
    
    size_t numClients = NimBLEDevice::getClientListSize();
    if (numClients > 0) {
        std::list<NimBLEClient*> *clientList = NimBLEDevice::getClientList();
        for (auto it = clientList->begin(); it != clientList->end(); it++) {
            if ((*it)->isConnected()) {
                (*it)->disconnect();
            }
        }
    }
    NimBLEDevice::deinit();
   
}

// Do not use loop to perform any functions
void loop(void)
{

  /*
   * WARNING:
   * DO NOT POPULATE FUNCTION
  */

}

void print_RaceBox_Data_message_payload_to_serial(){
  //serial print the received data:
    unsigned long currentTime = millis();
    if (currentTime - lastOutputTimeSerial >= outputIntervalMs_serial) { //limits the amount how often we print current values to serial

        // Serial output with correct formatting - HINT: the serial output as well as excessive updating of the OLED will take time and can hinder fast operation (e.g. reading in at 25hz), so output should be limited
        Serial.println();
        Serial.println("--- updated values from RaceBox Data Message available: ---");
        Serial.println("--------------------------------------------------------------------------------------------------");
        Serial.println("iTOW: " + String(iTOW) + " ms");
        Serial.println("Year: " + String(year));
        Serial.println("Month: " + String(month));
        Serial.println("Day: " + String(day));
        
        //Serial.println("Time (UTC): " + String(hour) + ":" + String(minute) + ":" + String(second));
        char timeString[9];  // Buffer to store the formatted time string
        sprintf(timeString, "%02d:%02d:%02d", hour, minute, second); //build a time string that always has the time format 00:00:00
        Serial.println("Time (UTC): " + String(timeString));
        
        //output fix status with interpretation
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
        
        Serial.println("Satellites: " + String(numSVs));
        Serial.println("Latitude: " + String(latitude / 1e7, 7) + " deg"); //we need to divide the latitude by 10^7 because the datasheet states that it is transmitted with a factor of 10^7
        Serial.println("Longitude: " + String(longitude / 1e7, 7) + " deg");
        Serial.println("WGS Altitude: " + String(wgsAltitude / 1000.0, 2) + " m");
        Serial.println("MSL Altitude: " + String(mslAltitude / 1000.0, 2) + " m");
        Serial.println("Horizontal Accuracy: " + String(horizontalAccuracy / 1000.0, 2) + " m");
        Serial.println("Vertical Accuracy: " + String(verticalAccuracy / 1000.0, 2) + " m");
        Serial.println("Speed Accuracy: " + String(speedAccuracy / 1000.0, 2) + " m/s");
        Serial.println("Speed: " + String(speed / 1000.0, 2) + " m/s");
        Serial.println("Speed: " + String(speed*3.6 / 1000.0, 2) + " km/h");
        Serial.print("Heading Accuracy: " + String(headingAccuracy / 1e5, 1) + " deg");
        Serial.println(" (heading " + String((fixStatusFlags & 0x20) ? "valid)" : "NOT valid - may need movement to become valid)"));
        //Serial.print("Heading: " + String(heading / 1e5, 1) + " deg");
        Serial.print("Heading: ");
        Serial.print(headingDegrees, 1); //heading (one decimal)
        Serial.print(" deg, compass direction: ");
        Serial.println(compass_direction); //magnetic compass direction (e.g., "N", "NO")
        Serial.println("PDOP: " + String(pdop / 100.0, 2));
        Serial.println("G-Force X: " + String(gForceX / 1000.0, 3) + " G");
        Serial.println("G-Force Y: " + String(gForceY / 1000.0, 3) + " G");
        Serial.println("G-Force Z: " + String(gForceZ / 1000.0, 3) + " G");
        Serial.println("Rot Rate X: " + String(rotRateX / 100.0, 2) + " deg/s");
        Serial.println("Rot Rate Y: " + String(rotRateY / 100.0, 2) + " deg/s");
        Serial.println("Rot Rate Z: " + String(rotRateZ / 100.0, 2) + " deg/s");


        
        //print fix status flags
//        Serial.println("Fix Status Flags (Hex): " + String(fixStatusFlags, HEX));
//        Serial.println("Fix Status Flags (Binary): " + String(fixStatusFlags, BIN));

        //print fix status flags with interpretation
        Serial.println("Fix Status Flags Interpretation:");
        Serial.println("  Bit 0: Valid Fix: " + String((fixStatusFlags & 0x01) ? "Yes" : "No"));
        Serial.println("  Bit 1: Differential Corrections Applied: " + String((fixStatusFlags & 0x02) ? "Yes" : "No"));
        Serial.println("  Bits 4..2: Power State: " + String((fixStatusFlags >> 2) & 0x07));
        Serial.println("  Bit 5: Valid Heading: " + String((fixStatusFlags & 0x20) ? "Yes" : "No"));
        Serial.println("  Bits 7..6: Carrier Phase Range Solution: " + String((fixStatusFlags >> 6) & 0x03));
        Serial.println();
                
        //Serial.println("Battery Status: " + String(batteryStatus)); //needs a function for interpretation, which is depending on device type:
        decodeBatteryStatus(batteryStatus); //a separate decoding funtion is a better solution, as there are differences in interpretation depending if it is a racebox mini, mini s oder micro
        Serial.println();
        
    }
    else{
      Serial.println("skipping serial output due to set serial update limitation");
    }
    Serial.println("--------------------------------------------------------------------------------------------------");
    Serial.println();
}


void interpret_serial_input(){ //read from serial console input to control starting of functions
// Check if data is available in the Serial buffer
//Serial.println("ready to receive serial messages from console");
  if (Serial.available() > 0) {
    // Read the incoming byte
    char incomingByte = Serial.read();

    // Respond based on the received byte
    switch (incomingByte) {
      case '1':
        Serial.println("received the message '1'. Starting function1...");
        function1();
        break;
      case '2':
        Serial.println("received the message '2'. Starting function2...");
        function2();
        break;
      case '3':
        Serial.println("received the message '3'. Starting function3...");
        function3();
        break;
      default:
        Serial.println("Invalid input. Please enter '1', '2', or '3'.");
        break;
    }
  }
}


// Function prototypes that can be started with serial console input (see above) - e.g. functions to start recording, stop recording or request data
void function1(){
  Serial.println("### Function1 started by input '1' on serial console. Function1 not yet implemented. ###");
  Serial.println("delaying delaying a second for readability of this message in serial console...");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  delay(1000);
}
// Function declarations that can be started with serial console input
void function2(){
  Serial.println("### Function2 started by input '2' on serial console. Function2 not yet implemented. ###");
  Serial.println("delaying delaying a second for readability of this message in serial console...");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  delay(1000);
}
// Function declarations that can be started with serial console input
void function3(){
  Serial.println("### Function3 started by input '3' on serial console. Function3 not yet implemented. ###");
  Serial.println("delaying a second for readability of this message in serial console...");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  Serial.println("***********************************************************************");
  delay(1000);
}
