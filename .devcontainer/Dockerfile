FROM mcr.microsoft.com/devcontainers/base:resolute

RUN apt update && apt install -y --no-install-recommends arduino-cli python3 python3-serial

RUN arduino-cli update
RUN arduino-cli core install esp32:esp32@2.0.11
RUN arduino-cli lib install "FastLED@3.6.0" "LovyanGFX@1.1.7" "NimBLE-Arduino@1.4.2" "lvgl@8.3.8" "autowp-mcp2515@1.2.1"
RUN arduino-cli lib install "Adafruit BusIO@1.16.1" "Adafruit HMC5883 Unified@1.2.3" "GFX Library for Arduino@1.4.1" "EspSoftwareSerial@8.1.0" "TinyGPSPlus@1.0.3" "Adafruit MPU6050@2.2.6" "Adafruit Unified Sensor@1.1.14"
