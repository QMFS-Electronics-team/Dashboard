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
 * Gear Sensor
 * Translates to Gear Number
 */

int digits_seven_segment_gear_display[8] = {0,1,2,3,4,5,6,7};
int gearValueMapping[7][4] = {{LOW,LOW,LOW,LOW},{LOW,LOW,LOW,HIGH},{},{},{},{},{}};
int gearSensorValue; // DON'T KNOW WHAT PIN THIS IS...
int priorGearValue; // Holds the prior value of the gear

int pinA_SSD = 69;
int pinB_SSD = 68;
int pinC_SSD = 67;
int pinD_SSD = 66;

int gearNumber = 0;
int gearSensorValue = 0;
bool gearChange;

void setup() 
{ 
  // When Number is value x that value will be printed out on the screen.
}

void loop()
{
  gearSensorValue = getGearValue();
  if(gearSensorValue != priorGearValue) // If there is a change in the gear value
  {
    updateGearValueSevenSegmentDisplay;
    updateGearValueSevenSegmentDisplay(number, gearChange);
    priorGearValue = gearSensorValue;
  }
}


void updateGearValueSevenSegmentDisplay(int number, bool gearChange)
{
    pinA_SSD = gearValueMapping[number][0];
    pinB_SSD = gearValueMapping[number][1];
    pinC_SSD = gearValueMapping[number][2];
    pinD_SSD = gearValueMapping[number][3];    
}

int getGearValue()
{
  int someValue = 100;// Just Some Random Value for Now
  return someValue;
}
