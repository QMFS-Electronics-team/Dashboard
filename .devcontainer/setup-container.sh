export DEBIAN_FRONTEND=noninteractive

# Install Arduino CLI and dependencies
# sudo apt update
# sudo apt install -y arduino-cli python3 python3-serial

# Install Board Support Package
# arduino-cli core install esp32:esp32@2.0.11

# Setup pins_arduino.h
cp ./Libraries/BoardSupport/pins_arduino/pins_arduino.h ~/.arduino15/packages/esp32/hardware/esp32/2.0.11/variants/esp32s3/pins_arduino.h

# Install Libraries
# arduino-cli update
# arduino-cli lib install "FastLED@3.6.0" "LovyanGFX@1.1.7" "NimBLE-Arduino@1.4.2" "lvgl@8.3.8" "autowp-mcp2515@1.2.1"
# arduino-cli lib install "Adafruit BusIO@1.16.1" "Adafruit HMC5883 Unified@1.2.3" "GFX Library for Arduino@1.4.1" "EspSoftwareSerial@8.1.0" "TinyGPSPlus@1.0.3" "Adafruit MPU6050@2.2.6" "Adafruit Unified Sensor@1.1.14"

# Setup Custom Libraries
cp -r Libraries/DashboardLibraries/DashboardProjectHeaderFiles ~/Arduino/libraries/
cp Libraries/DashboardLibraries/lv_conf.h ~/Arduino/libraries/lvgl/src/

# Enable serial
sudo usermod -aG dialout "$(id -un)" || true