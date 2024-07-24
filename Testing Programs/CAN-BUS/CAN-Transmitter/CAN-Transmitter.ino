// Reference: https://how2electronics.com/interfacing-mcp2515-can-bus-module-with-arduino/
#include <SPI.h>          // Library for using SPI Communication 
#include <mcp2515.h>      // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
 
struct can_frame canMsg;
 
MCP2515 mcp2515(10); 
 
void setup()
{
  while (!Serial);
  Serial.begin(9600);
  SPI.begin();               // Begin SPI communication
 
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();

  Serial.println("CAN-Transmit");
}
 
int h = 100;
int t = 200;
 
void loop()
{
  
//  h += 1;
//  t += 1; 
//
//  if (h > 200) {
//    h = 100;
//  }
//
//  if (t > 300) {
//    t = 200;
//  }
 
  canMsg.can_id  = 0x036;           // CAN id as 0x036
  canMsg.can_dlc = 8;               // CAN data length as 8
  canMsg.data[0] = h;               // h data in [0]
  canMsg.data[1] = t;               // t data in [1]
  canMsg.data[2] = 0x00;            // Rest all with 0
  canMsg.data[3] = 0x00;
  canMsg.data[4] = 0x00;
  canMsg.data[5] = 0x00;
  canMsg.data[6] = 0x00;
  canMsg.data[7] = 0x00;

  Serial.println(h);
  Serial.println(t);
  Serial.println("Sending message");
  mcp2515.sendMessage(&canMsg);     // Send the CAN message
  
}
