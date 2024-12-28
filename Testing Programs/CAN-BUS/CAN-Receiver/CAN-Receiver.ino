// Reference: https://how2electronics.com/interfacing-mcp2515-can-bus-module-with-arduino/
#include <SPI.h>                      // Library for using SPI Communication 
#include <mcp2515.h>                  // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
#include <CAN-BUS-Setup.h> 

struct can_frame canMsg;

MCP2515 mcp2515(CANBUS_CS);
 
void setup() {
  while(!Serial);
  Serial.begin(BAUDRATE);
  Serial.println(F("CAN BUS Receiver"));
  
  mcp2515.reset();
  if(mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ) == MCP2515::ERROR_OK) {
    Serial.println("CAN BUS Initialised");
  } else {
    Serial.println("CAN BUS Not Initialised. Error.");
  }
  mcp2515.setNormalMode();
  
}
 
 
void loop() {
  if (mcp2515.readMessage(&canMsg) != MCP2515::ERROR_NOMSG) {

    Serial.print("CAN Message ID: ");
    Serial.print(canMsg.can_id); // print ID
    Serial.print(" ");
    Serial.print("Message Length: ");
    Serial.print(canMsg.can_dlc); // print DLC
    Serial.print(" ");
    Serial.print("Data: ");
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i]);
      Serial.print(" ");
    }
    Serial.println();      
  } else {
    Serial.println("No Data Recieved");
  }
}
