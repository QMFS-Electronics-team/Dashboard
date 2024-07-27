// Reference: https://how2electronics.com/interfacing-mcp2515-can-bus-module-with-arduino/
#include <SPI.h>          // Library for using SPI Communication 
#include <mcp2515.h>      // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
 
struct can_frame canMsg2000;
struct can_frame canMsg2001;
struct can_frame canMsg2002;
struct can_frame canMsg2003;
struct can_frame canMsg2004;
struct can_frame canMsg2005;
 
MCP2515 mcp2515(10); 
 
void setup() {
  while (!Serial);
  Serial.begin(9600);
  SPI.begin();               // Begin SPI communication
 
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();

  Serial.println("CAN-Simulator-S60");
}

void sendPacket2000() {
  canMsg2000.can_id  = 0x2000;
  canMsg2000.can_dlc = 4;               
  canMsg2000.data[0] = 0x25;   // RPM         
  canMsg2000.data[1] = 0x50;   // TPS %         
  canMsg2000.data[2] = 0x30;   // Water Temp C   
  canMsg2000.data[3] = 0x20;   // Air Temp C

  Serial.println("Sending Packet 2000");
  mcp2515.sendMessage(&canMsg2000);
}

void sendPacket2001() {
  canMsg2001.can_id  = 0x2001;
  canMsg2001.can_dlc = 4;               
  canMsg2001.data[0] = 0x01; // MAP Kpa         
  canMsg2001.data[1] = 0x01; // Lambda x 1000       
  canMsg2001.data[2] = 0x06; // KPH x 10  
  canMsg2001.data[3] = 0x02; // Oil P Kpa

  Serial.println("Sending Packet 2001");
  mcp2515.sendMessage(&canMsg2001);
}

void sendPacket2002() {
  canMsg2002.can_id  = 0x2002;
  canMsg2002.can_dlc = 4;               
  canMsg2002.data[0] = 0x03; // Fuel P Kpa         
  canMsg2002.data[1] = 0x50; // Oil Temp C       
  canMsg2002.data[2] = 0x01; // Volts x 10  
  canMsg2002.data[3] = 0x20; // Fuel Con. L/100Km x 10

  Serial.println("Sending Packet 2002");
  mcp2515.sendMessage(&canMsg2002);
}

void sendPacket2003() {
  canMsg2003.can_id  = 0x2003;
  canMsg2003.can_dlc = 4;               
  canMsg2003.data[0] = 0x05; // Gear
  canMsg2003.data[1] = 0x02; // Advance Degx10   
  canMsg2003.data[2] = 0x03; // Injection ms x 100
  canMsg2003.data[3] = 0x02; // Fuel Con. L/Hr x 10

  Serial.println("Sending Packet 2003");
  mcp2515.sendMessage(&canMsg2003);
}

void sendPacket2004() {
  canMsg2004.can_id  = 0x2004;
  canMsg2004.can_dlc = 4;               
  canMsg2004.data[0] = 0x01; // Ana1 mV
  canMsg2004.data[1] = 0x02; // Ana2 mV
  canMsg2004.data[2] = 0x03; // Ana3 mV
  canMsg2004.data[3] = 0x05; // Cam Advance x 10

  Serial.println("Sending Packet 2004");
  mcp2515.sendMessage(&canMsg2004);
}

void sendPacket2005() {
  canMsg2005.can_id  = 0x2005;
  canMsg2005.can_dlc = 4;               
  canMsg2005.data[0] = 0x10; // Cam Targ x 10
  canMsg2005.data[1] = 0x10; // CAM PWM x 10
  canMsg2005.data[2] = 0x00; // Crank Errors
  canMsg2005.data[3] = 0x00; // CAM Errors

  Serial.println("Sending Packet 2005");
  mcp2515.sendMessage(&canMsg2005);
  
}

void loop() {
  sendPacket2000();
  delay(1000);
  sendPacket2001();
  delay(1000);
  sendPacket2002();
  delay(1000);
  sendPacket2003();
  delay(1000);
  sendPacket2004();
  delay(1000);
  sendPacket2005();
  delay(1000);
  
}
