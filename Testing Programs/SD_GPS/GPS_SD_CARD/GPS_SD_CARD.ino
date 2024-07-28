
// Libraries
#include <SPI.h>
#include <SD.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// Objects
const int TXPin = 35;                   // T Green Wire
const int RXPin = 34;                   // R Red Wire
const int chipSelect = 5;               // SD Card CS
const uint32_t GPSBaud = 9600;          // Default baud is 9600
TinyGPSPlus gps;                        // The TinyGPS++ object
SoftwareSerial gpsSerial(TXPin, RXPin); // The serial interface to the GPS device

void setup() {
  // Setup Serial
  Serial.begin(115200);

  // Setup GPS
  gpsSerial.begin(GPSBaud);
  Serial.println(F("Arduino - GPS module"));

  // Setup SD Card
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  File file = SD.open("/gpsdata.txt", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing.");
    return;
  }
  file.println("Time, Latitude, Longitude");
  file.close();
}

void loop() {
  if (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid()) {
        Serial.print(F("- latitude: "));
        Serial.println(gps.location.lat());

        Serial.print(F("- longitude: "));
        Serial.println(gps.location.lng());

        Serial.print(F("- altitude: "));
        if (gps.altitude.isValid())
          Serial.println(gps.altitude.meters());
        else
          Serial.println(F("INVALID"));
      } else {
        Serial.println(F("- location: INVALID"));
      }

      Serial.print(F("- speed: "));
      if (gps.speed.isValid()) {
        Serial.print(gps.speed.kmph());
        Serial.println(F(" km/h"));
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.print(F("- GPS date&time: "));
      if (gps.date.isValid() && gps.time.isValid()) {
        Serial.print(gps.date.year());
        Serial.print(F("-"));
        Serial.print(gps.date.month());
        Serial.print(F("-"));
        Serial.print(gps.date.day());
        Serial.print(F(" "));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        Serial.println(gps.time.second());
      } else {
        Serial.println(F("INVALID"));
      }

      Serial.println();


      // Write GPS data to SD card
      File file = SD.open("/gpsdata.txt", FILE_APPEND);
      if (file) {
        file.print("Lattitude: ");
        file.print(gps.location.lat(), 6);
        file.print(", Longitude: ");
        file.println(gps.location.lng(), 6);
        
        file.print("Altitude: ");
        file.println(gps.altitude.meters());

        file.print("Speed: ");
        file.print(gps.speed.kmph());
        file.println(" km/h");

        file.print(gps.date.year());
        file.print(F("-"));
        file.print(gps.date.month());
        file.print(F("-"));
        file.print(gps.date.day());
        file.print(F(" "));
        file.print(gps.time.hour());
        file.print(F(":"));
        file.print(gps.time.minute());
        file.print(F(":"));
        file.println(gps.time.second());
        
        file.close();
        Serial.println("Data Appended to File");
      } else {
        Serial.println("Failed to open file for appending.");
      }
      delay(1000); // Wait for a second
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS data received: check wiring"));
  } else {
  }
}
