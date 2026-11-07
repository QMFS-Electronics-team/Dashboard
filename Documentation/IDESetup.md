# IDE Setup
This markdown file gives instructions on how to setup an IDE for software development for the Dashboard. 

There are two options for IDEs
- VSCode
- Arduino IDE

## VSCode

### Install VSCode
At the time of development VSCode `1.96.2` was used.

Follow this [link](https://code.visualstudio.com/download) for the installer for VSCode.

If you want to install via the commandline - 

#### macOS 
    
    brew install visual-studio-code

#### Linux (Ubuntu)
    
    sudo snap install --classic code

or follow this [guide](https://code.visualstudio.com/docs/setup/linux) if you want to install with `apt`

### Install Arduino Extension for VS Code

From the extensions tab install the extension Arduino extension `vscode-arduino.vscode-arduino-community`

Open up the repository with VSCode. Once the `.vscode` directory is generated, navigate to `arduino.json`. Append the following lines in addition to what is alredy present. The `Output` folder containes the build and having this folder speeds up the build time as it servers as a cache.

```
{
    "output": "Arduino_Output",
    "arduino.enableCache": true,
    "arduino.logLevel": "info"
}
```

## Arduino IDE

### Install Arduino IDE

For this project Arduino IDE `2.3.4` was used.

Follow this [link](https://www.arduino.cc/en/software) for the installer for Arduino IDE

If you want to install via the commandline - 

#### macOS
    
    brew install arduino-ide

#### Linux
    
    sudo apt install arduino-ide

### Setup ESP Exception Decode
This is useful for debugging when you have a stack trace from ESP32. Follow this [link](https://github.com/dankeboy36/esp-exception-decoder) for instructions.

## Setup Instructions for Both VSCode and Arduino IDE

### Install ESP32 Core 
This is the ESP32 package file that is kep under the Arduino15 folder

For this project ESP32 Core Version: `2.0.11` has been used

The board manager on VSCode (Arduino extension) or on Arduino IDE can be used.

To insall via command line:

#### macOS

    brew install arduino-cli

#### Linux
    
    sudo apt install arduino-cli

Then run

    arduino-cli core install esp32:esp32@2.0.11

### Edit pins_arduino.h

This project uses custom SPI pins, `pins_arduino.h` needs to be edited and can be found with the file paths provided below based on your platform.

#### macOS

    ~/Library/Arduino15/packages/esp32/hardware/esp32/<version>/variants/esp32s3

#### Windows

    C:\Users\<Username>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\<version>\variants\esp32s3

#### Linux 

    ~/.arduino15/packages/esp32/hardware/esp32/2.0.11/variants/esp32s3/

Edit the file's MOSI, MISO and SCK lines to the following:

```
static const uint8_t MOSI  = 15;
static const uint8_t MISO  = 7;
static const uint8_t SCK   = 16;
```

or replace the file with the one found in this repository `Libraries/BoardSupport/pins_arduino/pins_arduino.h`

### Libraries to Install

#### Libraries for Dashboard Program

    FastLED                  3.6.0
    LovyanGFX                1.1.7
    NimBLE-Arduino           1.4.2
    lvgl                     8.3.8
    autowp-mcp2515           1.2.1

#### Libraries for Testing Programs

    Adafruit BusIO           1.16.1
    Adafruit HMC5883 Unified 1.2.3
    Adafruit MPU6050         2.2.6
    Adafruit Unified Sensor  1.1.14
    EspSoftwareSerial        8.1.0
    GFX Library for Arduino  1.4.1
    TinyGPSPlus              1.0.3 

Use the Library Manager to install the correct version of these libraries.

This can also be done via `arduino-cli` like so 

`arduino-cli lib install <library>@<version>`

To install all of the dependences in one go use the following command

Libraries for the Dashboard Program - 

    arduino-cli lib install "FastLED@3.6.0" "LovyanGFX@1.1.7" "NimBLE-Arduino@1.4.2" "lvgl@8.3.8" "autowp-mcp2515@1.2.1"

Libraries for the testing Programs - 

    arduino-cli lib install "Adafruit BusIO@1.16.1" "Adafruit HMC5883 Unified@1.2.3" "GFX Library for Arduino@1.4.1" "EspSoftwareSerial@8.1.0" "TinyGPSPlus@1.0.3" "Adafruit MPU6050@2.2.6" "Adafruit Unified Sensor@1.1.14"

### Custom Libraries for Required Project

Additionally place the header files used in this project (`Libraries/DashboardLibraries/DashboardProjectHeaderFiles`) under `~/Documents/Arduino/libraries/`

In the lvgl library place the `lv_conf.h` found in `Libraries/DashboardLibraries/lv_conf.h` of this repository into `~/Documents/Arduino/libraries/lvgl/src/`.

### Compile

Setup the board config as per the following settings:

    Board: ESP32S3 Dev Module (esp32)
    Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
    Upload Speed: 115200

At this point you are ready to compile the code.

From Arduino IDE you can compile by clicking on the verify button.

From VSCode enter `CMD+SHIFT+P` and enter `Arduino: verify`

From the commandline you can use the following command from the directory `Dashboard_Program/`

    arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=8M,PartitionScheme=huge_app,PSRAM=enabled --output-dir ../build Dashboard_Program.ino

### Upload

From Arduino IDE you can compile by clicking on the upload button.

From VSCode enter `CMD+SHIFT+P` and enter `Arduino CLI: Upload`

From the commandline you can use the following command from the the directory `Dashboard_Program/`

    arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32s3:FlashSize=8M,PartitionScheme=huge_app,PSRAM=enabled --input-dir ../build Dashboard_Program.ino