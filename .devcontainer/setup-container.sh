export DEBIAN_FRONTEND=noninteractive

# Install Arduino CLI and dependencies

# Setup pins_arduino.h
cp ./Libraries/BoardSupport/pins_arduino/pins_arduino.h ~/.arduino15/packages/esp32/hardware/esp32/2.0.11/variants/esp32s3/pins_arduino.h

# Setup Custom Libraries
cp -r Libraries/DashboardLibraries/DashboardProjectHeaderFiles ~/Arduino/libraries/
cp Libraries/DashboardLibraries/lv_conf.h ~/Arduino/libraries/lvgl/src/

# Enable serial
sudo usermod -aG dialout "$(id -un)" || true