/* 
 List Of Components
 CAN BUS
 RPM 
 Shift lights 
 Gear
 Temperatures 
*/


// Gear Seven Segment Display
/*
 *  
 * 
 * 
 */


/*
 * 
 * 
 * 
 * Gear Sensor
 * Translates to Gear Number
 */

int digits_seven_segment_gear_display[8] = {0,1,2,3,4,5,6,7};
int gearValueMapping[7][4] = {{},{},{},{},{},{},{}};
int gearSensorValue;


int pinA_SSD = 69;
int pinB_SSD = 68;
int pinC_SSD = 67;
int PinD_SSD = 66;


int number = 0;


void setup() 
{ 
  // When Number is value x that value will be printed out on the screen.
  pinA_SSD = gearValueMapping[number][0];
  pinB_SSD = gearValueMapping[number][1];
  pinC_SSD = gearValueMapping[number][2];
  pinD_SSD = gearValueMapping[number][3];
}

void loop()
{
  

}
