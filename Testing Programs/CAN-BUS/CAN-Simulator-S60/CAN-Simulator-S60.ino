// Reference: https://how2electronics.com/interfacing-mcp2515-can-bus-module-with-arduino/
#include <SPI.h>                      // Library for using SPI Communication 
#include <mcp2515.h>                  // Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)
#include <CAN-BUS-Setup.h>

struct can_frame canMsg2000;
struct can_frame canMsg2001;
struct can_frame canMsg2002;
struct can_frame canMsg2003;
struct can_frame canMsg2004;
struct can_frame canMsg2005;

byte higher_byte; 
byte lower_byte;

MCP2515 mcp2515(CANBUS_CS);
 
int send_delay = MILLISECOND/SEND_FREQUENCY;

int rpm, tps, water_temp = 0;       // Packet 2000 - [0] RPM, [1] Throttle Position Sensor, [2] Water Temperature
int kph = 0;                        // Packet 2001 - [2] Speed reported by ECU
int oil_temp, battery_voltage = 0;  // Packet 2002 - [1] Oil Temperature, [2] Battery Voltage
int gear = 0;                       // Packet 2003 - [0] Gear
int bps = 0;  

void setup() {
  while(!Serial);
  Serial.begin(BAUDRATE);
  Serial.println(F("CAN BUS S60 Simulator"));
  
  mcp2515.reset();
  if(mcp2515.setBitrate(CAN_1000KBPS, MCP_8MHZ) == MCP2515::ERROR_OK) {
    Serial.println("CAN BUS Initialised");
  } else {
    Serial.println("CAN BUS Not Initialised. Error.");
  }
  mcp2515.setNormalMode();
}


void sendPacket2000() {
  canMsg2000.can_id  = 0x2000| CAN_EFF_FLAG;
  canMsg2000.can_dlc = 8;
 
  // RPM
  rpm = get_rpm();
  higher_byte = (rpm >> 8) & 0xFF;
  lower_byte = rpm & 0xFF;
  canMsg2000.data[0] = higher_byte;
  canMsg2000.data[1] = lower_byte;

  // TPS %
  tps = get_tps();
  higher_byte = (tps >> 8) & 0xFF;
  lower_byte = tps & 0xFF;
  canMsg2000.data[2] = higher_byte;
  canMsg2000.data[3] = lower_byte;

  // Water Temp C
  water_temp = get_water_temp();
  higher_byte = (water_temp >> 8) & 0xFF;
  lower_byte = water_temp & 0xFF;
  canMsg2000.data[4] = higher_byte;
  canMsg2000.data[5] = lower_byte;

  // Air Temp C
  canMsg2000.data[6] = 0x00;
  canMsg2000.data[7] = 0x00;

  Serial.println(F("Sending Packet 2000"));
  mcp2515.sendMessage(&canMsg2000);
}

void sendPacket2001() {
  canMsg2001.can_id  = 0x2001 | CAN_EFF_FLAG;
  canMsg2001.can_dlc = 8;
  
  // MAP Kpa
  canMsg2001.data[0] = 0x00;
  canMsg2001.data[1] = 0x00;

  // Lambda x 1000
  canMsg2001.data[2] = 0x00;
  canMsg2001.data[3] = 0x00;
  
  // KPH x 10
  canMsg2001.data[4] = 0x00;
  canMsg2001.data[5] = 0x00;

  // Oil P Kpa
  canMsg2001.data[6] = 0x00;
  canMsg2001.data[7] = 0x00;

  Serial.println(F("Sending Packet 2001"));
  mcp2515.sendMessage(&canMsg2001);
}

void sendPacket2002() {
  canMsg2002.can_id  = 0x2002 | CAN_EFF_FLAG;
  canMsg2002.can_dlc = 8;
  
  // Fuel P Kpa
  canMsg2002.data[0] = 0x00;
  canMsg2002.data[1] = 0x00;                  
  
  // Oil Temp C
  oil_temp = get_oil_temp();
  higher_byte = (oil_temp >> 8) & 0xFF;
  lower_byte = oil_temp & 0xFF;        
  canMsg2002.data[2] = higher_byte;
  canMsg2002.data[3] = lower_byte;
  
  // Volts x 10
  battery_voltage = get_battery_voltage();
  higher_byte = (battery_voltage >> 8) & 0xFF;
  lower_byte = battery_voltage & 0xFF;        
  canMsg2002.data[4] = higher_byte;
  canMsg2002.data[5] = lower_byte;
  
  // Fuel Con. L/100Km x 10
  canMsg2002.data[6] = 0x00;
  canMsg2002.data[7] = 0x00;                  

  Serial.println(F("Sending Packet 2002"));
  mcp2515.sendMessage(&canMsg2002);
}

void sendPacket2003() {
  canMsg2003.can_id  = 0x2003 | CAN_EFF_FLAG;
  canMsg2003.can_dlc = 8;

  // Gear
  gear = get_gear();
  higher_byte = (gear >> 8) & 0xFF;
  lower_byte = gear & 0xFF;
  canMsg2003.data[0] = higher_byte;
  canMsg2003.data[1] = lower_byte;

  // Advance Degx10
  canMsg2003.data[2] = 0x00;
  canMsg2003.data[3] = 0x00;

  // Injection ms x 100
  canMsg2003.data[4] = 0x00;
  canMsg2003.data[5] = 0x00;       
  
  // Fuel Con. L/Hr x 10
  canMsg2003.data[6] = 0x00;
  canMsg2003.data[7] = 0x00;       

  Serial.println(F("Sending Packet 2003"));
  mcp2515.sendMessage(&canMsg2003);
}

void sendPacket2004() {
  canMsg2004.can_id  = 0x2004 | CAN_EFF_FLAG;
  canMsg2004.can_dlc = 8;

  // Ana1 mV - BPS
  bps = get_bps();
  higher_byte = (bps >> 8) & 0xFF;
  lower_byte = bps & 0xFF;
  canMsg2004.data[0] = higher_byte;
  canMsg2004.data[1] = lower_byte;
  
  // Ana2 mV
  canMsg2004.data[2] = 0x00;
  canMsg2004.data[3] = 0x00;
  
  // Ana3 mV
  canMsg2004.data[4] = 0x00;
  canMsg2004.data[5] = 0x00;

  // Cam Advance x 10
  canMsg2004.data[6] = 0x00;
  canMsg2004.data[7] = 0x00;

  Serial.println(F("Sending Packet 2004"));
  mcp2515.sendMessage(&canMsg2004);
}

void sendPacket2005() {
  canMsg2005.can_id  = 0x2005 | CAN_EFF_FLAG;
  canMsg2005.can_dlc = 8;
  
  // Cam Targ x 10
  canMsg2005.data[0] = 0x00; 
  canMsg2005.data[1] = 0x00; 
  
  // CAM PWM x 10
  canMsg2005.data[2] = 0x00;
  canMsg2005.data[3] = 0x00; 
  
  // Crank Errors
  canMsg2005.data[4] = 0x00;
  canMsg2005.data[5] = 0x00; 

  // CAM Errors
  canMsg2005.data[6] = 0x00;
  canMsg2005.data[7] = 0x00; 

  Serial.println(F("Sending Packet 2005"));
  mcp2515.sendMessage(&canMsg2005);
}


// Packet 2000
int get_rpm() {
   return int(map(analogRead(KNOB_SIX), 0, 1023, 12000, 0));
}

int get_tps() {
  return int(map(analogRead(KNOB_FOUR), 0, 1023, 100, 0));
}

int get_water_temp() {
  return int(map(analogRead(KNOB_ONE), 0, 1023, 100, 0));
}

// Packet 2002
int get_oil_temp() {
  return int(map(analogRead(KNOB_TWO), 0, 1023, 100, 0));
}

int get_battery_voltage() {
  return int(map(analogRead(KNOB_SEVEN), 0, 1023, 15, 0));
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
  delay(send_delay);
}
