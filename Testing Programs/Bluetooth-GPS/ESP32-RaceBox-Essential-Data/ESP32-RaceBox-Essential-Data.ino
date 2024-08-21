// Reference: https://github.com/lademeister/ESP32-RaceBox
#include "NimBLEDevice.h"

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

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (length >= 80) {
    parsePayload(pData);
  } else {
    Serial.println("payload length is less than 80 bytes. 80 bytes would be expected for a RaceBox data message.");
    Serial.println("For other messages, the payload can be shorter.");
    parsePayload(pData);
  }
}

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

class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      Serial.print("Advertised BLE Device found: ");
      Serial.println(advertisedDevice->toString().c_str());

      if (advertisedDevice->isAdvertisingService(UART_service_UUID)) {
        // Check if the device name starts with "RaceBox"
        std::string deviceName = advertisedDevice->getName();
        if (deviceName.rfind("RaceBox", 0) == 0) { //rfind returns 0 if we find "RaceBox" at the beginning of advertised device name. If you have problems here double check case (other RaceBoxes *could* be named "Racebox" or "racebox")

          // If no specific address is defined, connect to any device whose name starts with "RaceBox"
          Serial.println("RaceBox found. TARGET_DEVICE_ADDRESS is not set in code (or commented out ), so we connect to any RaceBox that we find.");
          NimBLEDevice::getScan()->stop();  // Stop scanning
          Serial.println("stopped bluetooth scanning.");
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
    Serial.println("Invalid frame start of payload data - check may need to be removed or changed for other data than RaceBox Data Message!");
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
    Serial.print("Received packet size exceeds maximum allowed size (512 bytes). ");
    Serial.print("Packet length is ");
    Serial.print(packetLength);
    Serial.println(" bytes.");
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
    Serial.print("unknown message class and message ID found (it may be other data?): ");
    Serial.print("Message Class: 0x");
    Serial.print(messageClass, HEX);
    Serial.print(", Message ID: 0x");
    Serial.println(messageId, HEX);
    Serial.println("Ignoring packet. This is not a known/implemented data packet.");
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
        Serial.println("Failed to reconnect. Retrying...");
        return false;
      }
    } else {
      // Create a new client if none matches
      pClient = NimBLEDevice::createClient();
      pClient->setClientCallbacks(new ClientCallbacks(), false);
      if (!pClient->connect(myRaceBox)) {
        Serial.println("Failed to connect.");
        NimBLEDevice::deleteClient(pClient);
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

void setup() {
  Serial.begin(115200);
  Serial.flush();

  Serial.println("Scanning for Bluetooth devices.");
  Serial.println();
  Serial.println("Scan Results:");
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

    Serial.println("--- Updated Data from RaceBox ---");
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

    Serial.println("Speed: " + String((speed / 1000.0)/0.44704, 2) + " mph");
    

    if((fixStatusFlags & 0x20) == false) {
      Serial.println("Heading NOT valid - may need movement to become valid");
    } else {
      Serial.print("Heading: " + String(heading / 1e5, 1) + " deg ");
      Serial.print("Heading: " + String(headingDegrees, 1));
      Serial.print(" deg, compass direction: ");
      Serial.println(compass_direction);
    }

    Serial.println("G-Force X: " + String(gForceX / 1000.0, 3) + " Y: " + String(gForceY / 1000.0, 3) +  " Z: " + String(gForceZ / 1000.0, 3) + " G");
    Serial.println("Rotation Rate X: " + String(rotRateX / 100.0, 2) + " Y: " + String(rotRateY / 100.0, 2) + " Z: " + String(rotRateZ / 100.0, 2) + " deg/s");

    Serial.println();

  } else {
    Serial.println("skipping serial output due to set serial update limitation");
  }

}

void loop() {

  if (doConnect) {
    if (connectToRaceBox()) {
      Serial.println("successfully connected to RaceBox.");
      Serial.println();
      NimBLEDevice::getScan()->stop();
    } else {
      Serial.println("Failed to connect to RaceBox. Reattempting BLE connection...");
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

  delay(5000); // Delay output to make serial output more readable

}
