#include <TinyGPSPlus.h>

// A sample NMEA stream.
const char *gpsStream =
  "$GPRMC,045103.000,A,3014.1984,N,09749.2872,W,0.67,161.46,030913,,,A*7C\r\n"
  "$GPGGA,045104.000,3014.1985,N,09749.2873,W,1,09,1.2,211.6,M,-22.5,M,,0000*62\r\n"
  "$GPRMC,045200.000,A,3014.3820,N,09748.9514,W,36.88,65.02,030913,,,A*77\r\n"
  "$GPGGA,045201.000,3014.3864,N,09748.9411,W,1,10,1.2,200.8,M,-22.5,M,,0000*6C\r\n"
  "$GPRMC,045251.000,A,3014.4275,N,09749.0626,W,0.51,217.94,030913,,,A*7D\r\n"
  "$GPGGA,045252.000,3014.4273,N,09749.0628,W,1,09,1.3,206.9,M,-22.5,M,,0000*6F\r\n";


TinyGPSPlus gps;

void setup()
{
  Serial.begin(115200);
  
  while (*gpsStream)
    if (gps.encode(*gpsStream++))
      displayInfo();
}

void loop()
{
  displayInfo(); 
  delay(5000);
}

void displayInfo()
{
  
  if (gps.location.isValid())
  {
    Serial.print(F("Lat: "));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(" Long: "));
    Serial.println(gps.location.lng(), 6);
  }
  
  if (gps.date.isValid())
  {
    Serial.print(F("Date: "));
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.println(gps.date.year());
  }
  
  if (gps.time.isValid())
  {
    Serial.print(F("Time: "));
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.println(gps.time.second());
  }

  if(gps.speed.isValid())
  {
    Serial.print(F("MPH: "));
    Serial.println(gps.speed.mph());
  }

  if(gps.course.isValid())
  {
    Serial.print(F("Deg: "));
    Serial.println(gps.course.deg());
  }

  if(gps.altitude.isValid())
  {
    Serial.print(F("Miles: "));
    Serial.println(gps.altitude.miles());
  }

   if (gps.satellites.isValid())
  {
    Serial.print(F("Number of Satellite: "));
    Serial.println(gps.satellites.value());
  }

  Serial.println("\n");
}
