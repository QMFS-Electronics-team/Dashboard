// Reference: https://how2electronics.com/interfacing-mcp2515-can-bus-module-with-arduino/
#include <SPI.h>                      // Library for using SPI Communication 
#include <mcp2515.h>                  // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
#include <S60SimulatorDefinitions.h>  // Preprocessor for this code

struct can_frame canMsg2000;
struct can_frame canMsg2001;
struct can_frame canMsg2002;
struct can_frame canMsg2003;
struct can_frame canMsg2004;
struct can_frame canMsg2005;

MCP2515 mcp2515(CANBUS_CS);

void setup() {
  while(!Serial);
  Serial.begin(BAUDRATE);
  Serial.println(F("CAN BUS S60 Simulator"));
  
  mcp2515.reset();
  if(mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ) == MCP2515::ERROR_OK) {
    Serial.println("CAN BUS Initialised");
  } else {
    Serial.println("CAN BUS Not Initialised. Error.");
  }
  mcp2515.setNormalMode();
}


void sendPacket2000() {
  canMsg2000.can_id  = 0x2000;
  canMsg2000.can_id |= CAN_EFF_FLAG;
  canMsg2000.can_dlc = 4;
  canMsg2000.data[0] = get_rpm();         // RPM
  canMsg2000.data[1] = get_tps();         // TPS %
  canMsg2000.data[2] = get_water_temp();  // Water Temp C
  canMsg2000.data[3] = 0x20;              // Air Temp C

  Serial.println(F("Sending Packet 2000"));
  mcp2515.sendMessage(&canMsg2000);
}

void sendPacket2001() {
  canMsg2001.can_id  = 0x2001;
  canMsg2001.can_id |= CAN_EFF_FLAG;
  canMsg2001.can_dlc = 4;
  canMsg2001.data[0] = 0x01; // MAP Kpa
  canMsg2001.data[1] = 0x01; // Lambda x 1000
  canMsg2001.data[2] = 0x06; // KPH x 10
  canMsg2001.data[3] = 0x02; // Oil P Kpa

  Serial.println(F("Sending Packet 2001"));
  mcp2515.sendMessage(&canMsg2001);
}

void sendPacket2002() {
  canMsg2002.can_id  = 0x2002;
  canMsg2002.can_id |= CAN_EFF_FLAG;
  canMsg2002.can_dlc = 4;
  canMsg2002.data[0] = 0x03;                  // Fuel P Kpa
  canMsg2002.data[1] = get_oil_temp();        // Oil Temp C
  canMsg2002.data[2] = get_battery_voltage(); // Volts x 10
  canMsg2002.data[3] = 0x20;                  // Fuel Con. L/100Km x 10

  Serial.println(F("Sending Packet 2002"));
  mcp2515.sendMessage(&canMsg2002);
}

void sendPacket2003() {
  canMsg2003.can_id  = 0x2003;
  canMsg2003.can_id |= CAN_EFF_FLAG;
  canMsg2003.can_dlc = 4;
  canMsg2003.data[0] = get_gear(); // Gear
  canMsg2003.data[1] = 0x02;       // Advance Degx10
  canMsg2003.data[2] = 0x03;       // Injection ms x 100
  canMsg2003.data[3] = 0x02;       // Fuel Con. L/Hr x 10

  Serial.println(F("Sending Packet 2003"));
  mcp2515.sendMessage(&canMsg2003);
}

void sendPacket2004() {
  canMsg2004.can_id  = 0x2004;
  canMsg2004.can_id |= CAN_EFF_FLAG;
  canMsg2004.can_dlc = 4;
  canMsg2004.data[0] = get_bps();  // Ana1 mV - BPS
  canMsg2004.data[1] = 0x02;       // Ana2 mV
  canMsg2004.data[2] = 0x03;       // Ana3 mV
  canMsg2004.data[3] = 0x05;       // Cam Advance x 10

  Serial.println(F("Sending Packet 2004"));
  mcp2515.sendMessage(&canMsg2004);
}

void sendPacket2005() {
  canMsg2005.can_id  = 0x2005;
  canMsg2005.can_id |= CAN_EFF_FLAG;
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
  sendPacket2000();
  sendPacket2001();
  sendPacket2002();
  sendPacket2003();
  sendPacket2004();
  sendPacket2005();
  
  logOutput();
}
