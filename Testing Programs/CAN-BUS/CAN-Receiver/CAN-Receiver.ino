// Reference: https://how2electronics.com/interfacing-mcp2515-can-bus-module-with-arduino/
#include <SPI.h>              // Library for using SPI Communication 
#include <mcp2515.h>          // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
 
struct can_frame canMsg;
 
MCP2515 mcp2515(2);                 // SPI CS Pin 2
 
 
void setup()
{
  Serial.begin(9600);                // Begins Serial Communication at 9600 baudrate
  SPI.begin();                       // Begins SPI communication
   
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Set CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();                   // Set CAN at normal mode
}
 
 
void loop() {
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {

    Serial.print("CAN Message ID: ");
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" ");
    Serial.print("Message Length: ");
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");
    Serial.print("Data: ");
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i],HEX);
      Serial.print(" ");
    }

    Serial.println();      
  }
}
