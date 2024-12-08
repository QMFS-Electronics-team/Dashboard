
#include <SPI.h>
#include <mcp2515.h>
#include <ProductionDefinitions.h>

struct can_frame canMsg; 
MCP2515 mcp2515(CANBUS_CS_PIN);
String outputString;

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
 
void setup() {
  Serial.begin(SERIAL_BAUDRATE);     
  SPI.begin();                       
   
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
}


void loop() {
  
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.println("Reading CAN BUS Message");
    outputString = "CAN Message ID: " + String(canMsg.can_id, HEX)  + " Message Length: " + String(canMsg.can_dlc, HEX) + " Data: ";
    
    switch(canMsg.can_id) {
      case 0:
        rpm = canMsg.data[0] * 100;
        Serial.println(rpm);
        tps = canMsg.data[1];
        water_temp = canMsg.data[2];
        break;
      case 1:
        kph = canMsg.data[2];
        break;
      case 2:
        oil_temp = canMsg.data[1];
        battery_voltage = canMsg.data[2];
        break;
      case 3:
        gear = canMsg.data[0];
        break;
      case 4:
        bps = canMsg.data[0];
        break;
      default:
        break;
    }
    
    for (int i = 0; i < canMsg.can_dlc; i++)  {
      outputString += String(canMsg.data[i], HEX);
      outputString += " ";
    }
    outputString += "\n";
    
    Serial.print(outputString);
    Serial.println(F(""));
    
  } else {
    Serial.println("No CAN BUS Data");
  }
  
}