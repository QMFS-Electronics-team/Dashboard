#include <TinyGPSPlus.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// A sample NMEA stream.
const char *gpsStream =
  "$GPRMC,045103.000,A,3014.1984,N,09749.2872,W,0.67,161.46,030913,,,A*7C\r\n"
  "$GPGGA,045104.000,3014.1985,N,09749.2873,W,1,09,1.2,211.6,M,-22.5,M,,0000*62\r\n"
  "$GPRMC,045200.000,A,3014.3820,N,09748.9514,W,36.88,65.02,030913,,,A*77\r\n"
  "$GPGGA,045201.000,3014.3864,N,09748.9411,W,1,10,1.2,200.8,M,-22.5,M,,0000*6C\r\n"
  "$GPRMC,045251.000,A,3014.4275,N,09749.0626,W,0.51,217.94,030913,,,A*7D\r\n"
  "$GPGGA,045252.000,3014.4273,N,09749.0628,W,1,09,1.3,206.9,M,-22.5,M,,0000*6F\r\n";

TinyGPSPlus gps;
String outputString; 

// Directory Functions
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}


// File Functions
void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
//    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
//        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}


// Setup Functions
void setup_sd_card(){
    
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    Serial.printf("SD Setup Complete!");

    listDir(SD, "/", 0);

    if (!SD.exists("/gps-data/gps-data.txt")) {
      Serial.println(F("Creating GPS File"));
      createDir(SD, "/gps-data");    
      writeFile(SD, "/gps-data/gps-data.txt", "Start of GPS Data\n");
    } else {
      Serial.println(F("GPS File Exists"));
      appendFile(SD, "/gps-data/gps-data.txt", "Start of New GPS Data\n");
    }
    
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
}


void displayInfo()
{
  
  if (gps.location.isValid())
  {
    Serial.print(F("Lat: "));
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(" Long: "));
    Serial.println(gps.location.lng(), 6);

    outputString = "Lat: " + String(gps.location.lat(), 6)  + " Long: " + String(gps.location.lng(), 6) + "\n";
    appendFile(SD, "/gps-data/gps-data.txt", outputString.c_str());
  }
  
  if (gps.date.isValid())
  {
    Serial.print(F("Date: "));
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.println(gps.date.year());

    outputString = "Date: " + String(gps.date.month()) + "/" + String(gps.date.day()) + "/" + String(gps.date.year()) + "\n";
    appendFile(SD, "/gps-data/gps-data.txt", outputString.c_str());
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

    outputString = "Time: " + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + "\n";
  }

  if(gps.speed.isValid())
  {
    Serial.print(F("MPH: "));
    Serial.println(gps.speed.mph());

    outputString += "MPH: " + String(gps.speed.mph()) + "\n";
  }

  if(gps.course.isValid())
  {
    Serial.print(F("Deg: "));
    Serial.println(gps.course.deg());

    outputString += "Deg: " + String(gps.course.deg()) + "\n";
  }

  if(gps.altitude.isValid())
  {
    Serial.print(F("Miles: "));
    Serial.println(gps.altitude.miles());

    outputString += "Miles: " + String(gps.altitude.miles()) + "\n";
  }

   if (gps.satellites.isValid())
  {
    Serial.print(F("Number of Satellite: "));
    Serial.println(gps.satellites.value());

    outputString += "Number of Satellite: " + String(gps.satellites.value()) + "\n" + "\n";
  }

  appendFile(SD, "/gps-data/gps-data.txt", outputString.c_str());

  Serial.println("");
}

void setup()
{
  Serial.begin(115200);
  
  while (*gpsStream)
    if (gps.encode(*gpsStream++))
      displayInfo();

  setup_sd_card();
}

void loop()
{
  displayInfo(); 
  delay(5000);
}
