// Reference: https://how2electronics.com/interfacing-mcp2515-can-bus-module-with-arduino/
#include <SPI.h>          // Library for using SPI Communication 
#include <mcp2515.h>      // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)

struct can_frame canMsg2000;
struct can_frame canMsg2001;
struct can_frame canMsg2002;
struct can_frame canMsg2003;
struct can_frame canMsg2004;
struct can_frame canMsg2005;

#define KNOB_ONE    A0
#define KNOB_TWO    A1
#define KNOB_THREE  A2
#define KNOB_FOUR   A3
#define KNOB_FIVE   A4
#define KNOB_SIX    A5
#define KNOB_SEVEN  A6

MCP2515 mcp2515(10);

void setup() {
  while (!Serial);
  Serial.begin(115200);
  SPI.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); // Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();

  Serial.println(F("CAN-Simulator-S60"));
}

void sendPacket2000(int water_temp, int tps, int rpm) {
  canMsg2000.can_id  = 0x2000;
  canMsg2000.can_dlc = 4;
  canMsg2000.data[0] = rpm;         // RPM
  canMsg2000.data[1] = tps;         // TPS %
  canMsg2000.data[2] = water_temp;  // Water Temp C
  canMsg2000.data[3] = 0x20;        // Air Temp C

  Serial.println(F("Sending Packet 2000"));
  mcp2515.sendMessage(&canMsg2000);
}

void sendPacket2001() {
  canMsg2001.can_id  = 0x2001;
  canMsg2001.can_dlc = 4;
  canMsg2001.data[0] = 0x01; // MAP Kpa
  canMsg2001.data[1] = 0x01; // Lambda x 1000
  canMsg2001.data[2] = 0x06; // KPH x 10
  canMsg2001.data[3] = 0x02; // Oil P Kpa

  Serial.println(F("Sending Packet 2001"));
  mcp2515.sendMessage(&canMsg2001);
}

void sendPacket2002(int oil_temp, int battery_voltage) {
  canMsg2002.can_id  = 0x2002;
  canMsg2002.can_dlc = 4;
  canMsg2002.data[0] = 0x03;            // Fuel P Kpa
  canMsg2002.data[1] = oil_temp;        // Oil Temp C
  canMsg2002.data[2] = battery_voltage; // Volts x 10
  canMsg2002.data[3] = 0x20;            // Fuel Con. L/100Km x 10

  Serial.println(F("Sending Packet 2002"));
  mcp2515.sendMessage(&canMsg2002);
}

void sendPacket2003(int gear) {
  canMsg2003.can_id  = 0x2003;
  canMsg2003.can_dlc = 4;
  canMsg2003.data[0] = gear; // Gear
  canMsg2003.data[1] = 0x02; // Advance Degx10
  canMsg2003.data[2] = 0x03; // Injection ms x 100
  canMsg2003.data[3] = 0x02; // Fuel Con. L/Hr x 10

  Serial.println(F("Sending Packet 2003"));
  mcp2515.sendMessage(&canMsg2003);
}

void sendPacket2004(int bps) {
  canMsg2004.can_id  = 0x2004;
  canMsg2004.can_dlc = 4;
  canMsg2004.data[0] = bps;  // Ana1 mV - BPS
  canMsg2004.data[1] = 0x02; // Ana2 mV
  canMsg2004.data[2] = 0x03; // Ana3 mV
  canMsg2004.data[3] = 0x05; // Cam Advance x 10

  Serial.println(F("Sending Packet 2004"));
  mcp2515.sendMessage(&canMsg2004);
}

void sendPacket2005() {
  canMsg2005.can_id  = 0x2005;
  canMsg2005.can_dlc = 4;
  canMsg2005.data[0] = 0x10; // Cam Targ x 10
  canMsg2005.data[1] = 0x10; // CAM PWM x 10
  canMsg2005.data[2] = 0x00; // Crank Errors
  canMsg2005.data[3] = 0x00; // CAM Errors

  Serial.println(F("Sending Packet 2005"));
  mcp2515.sendMessage(&canMsg2005);
}


// Packet 2000
int get_rpm() {
  return int(map(analogRead(KNOB_SIX), 0, 1023, 30, 0));
}

int get_tps() {
  return int(map(analogRead(KNOB_FOUR), 0, 1023, 100, 0));
}

int get_water_temp() {
  return int(map(analogRead(KNOB_SEVEN), 0, 1023, 100, 0));
}

// Packet 2002
int get_oil_temp() {
  return int(map(analogRead(KNOB_TWO), 0, 1023, 100, 0));
}

int get_battery_voltage() {
  return int(map(analogRead(KNOB_ONE), 0, 1023, 15, 0));
}

// Packet 2003
int get_gear() {
 return int(map(analogRead(KNOB_FIVE), 0, 1023, 5, 0));
}

// Packet 2004
int get_bps() {
  return int(map(analogRead(KNOB_THREE), 0, 1023, 100, 0));
}

void logOutput() {
  Serial.println(F(""));
  Serial.print(F("Water Temp - "));
  Serial.println(get_water_temp());
  Serial.print(F("TPS - "));
  Serial.println(get_tps());
  Serial.print(F("RPM - "));
  Serial.println(get_rpm());
  Serial.print(F("Gear - "));
  Serial.println(get_gear());
  Serial.print(F("Oil Temp - "));
  Serial.println(get_oil_temp());
  Serial.print(F("Battery Voltage - "));
  Serial.println(get_battery_voltage());
  Serial.print(F("BPS - "));
  Serial.println(get_bps());
  Serial.println(F(""));
}

void loop() {
  sendPacket2000(get_water_temp(), get_tps(), get_rpm());
  sendPacket2001();
  sendPacket2002(get_oil_temp(), get_battery_voltage());
  sendPacket2003(get_gear());
  sendPacket2004(get_bps());
  sendPacket2005();
  
  logOutput();
}
