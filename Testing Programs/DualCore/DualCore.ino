// This program is an example program using dual cores on the ESP32-S3

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SD.h>
#include <SPI.h>

#define BAUDRATE 115200
#define DELAY 2500
#define SDA 21
#define SCL 20
#define SD_CS 5

Adafruit_MPU6050 mpu;
float accel_x, accel_y, accel_z; // Acceleration
float gyro_x, gyro_y, gyro_z;    // Gyro
float temperature;               // Temperature

File dataFile; 
String dataString;
QueueHandle_t dataQueue;


/*----- Setup -----*/

void setup() {
  Serial.begin(BAUDRATE);
  while(!Serial);
  setup_mpu();
  setup_sd();
  create_rtos_tasks();
}

void setup_mpu() {
  Wire.begin(SDA, SCL);
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1);
  }
  Serial.println("MPU6050 Found");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void setup_sd() {
  if(!SD.begin(SD_CS)) {
    Serial.println("SD Card INIT Failed");
    while(1);
  } else {
    Serial.println("SD Card Initialised");
  }
}

void create_rtos_tasks() {
  dataQueue = xQueueCreate(10, sizeof(String));
  if (dataQueue == NULL) {
    Serial.println("Failed to Create Queue");
    while (1);
  }
  xTaskCreatePinnedToCore(readSensorDataTask, "ReadSensorData", 4096, NULL, 1, NULL, 0); // Core 0
  xTaskCreatePinnedToCore(sdWriteTask, "WriteData", 4096, NULL, 1, NULL, 1);             // Core 1
}


/*----- Get MPU Data -----*/

void readSensorDataTask(void *parameter) {
  sensors_event_t accel, gyro, temp;
  while(true) {
    mpu.getEvent(&accel, &gyro, &temp);

    accel_x = accel.acceleration.x;
    accel_y = accel.acceleration.y; 
    accel_z = accel.acceleration.z;

    gyro_x = gyro.gyro.x;
    gyro_y = gyro.gyro.y;
    gyro_z = gyro.gyro.z;

    temperature = temp.temperature;

    // ACCEL X, ACCEL Y, ACCEL Z, GYRO X, GYRO Y, GYRO Z, TEMPERATURE
    dataString = String(accel_x) + "," + String(accel_y) + "," + String(accel_z) + ","; // Acceleration
    dataString += String(gyro_x) + "," + String(gyro_y) + "," + String(gyro_z) + ",";    // Gyro
    dataString += String(temperature);

    // Add data to the queue
    if (xQueueSend(dataQueue, &dataString, portMAX_DELAY) != pdTRUE) {
      Serial.println("Queue full!");
    }

    vTaskDelay(100 / portTICK_PERIOD_MS); // Sample every 100ms
  }
}


/*----- Write to SD Card -----*/

void sdWriteTask(void *parameter) {
  while(true) {

    if (xQueueReceive(dataQueue, &dataString, portMAX_DELAY) == pdTRUE) {
      dataFile = SD.open("/mpu6050_data.csv", FILE_APPEND);
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
        Serial.println("Data written to SD card: " + dataString);
      } else {
        Serial.println("Failed to open file for writing.");
      }
    }

  }
}


/*----- Loop -----*/

void loop() {
}

