# IDE Setup
This markdown file gives instructions on how to setup an IDE for software development for the dashboard. 

There are two options for IDEs
- VSCode
- Arduino IDE

## VSCode

### Install VSCode
Follow this [link](https://code.visualstudio.com/download) for the installer for VSCode.

If you want to install via the commandline - 
- macOS `brew install visual-studio-code`
- Linux (Ubuntu) `sudo snap install --classic code`

For development VSCode `1.96.2` was used.

### Install Arduino Extension for VS Code

Install the extension `Arduino: vsciot-vscode.vscode-arduino` or `vscode-arduino.vscode-arduino-community` from the extensions tab.

## Arduino IDE

### Install Arduino IDE

Follow this [link](https://www.arduino.cc/en/software) for the installer for Arduino IDE

If you want to install via the commandline - 
- macOS `brew install arduino-ide`
- Linux `sudo apt install arduino-ide`

For this project VSCode `2.3.4` was used.

## Setup Instructions for Both VSCode and Arduino IDE

### Install ESP32 Core 
This is the ESP32 package file that is kep under the Arduino15 folder

For this project ESP32 Core Version: `2.0.11` has been used

The board manager on VSCode (Arduino extension) or on Arduino IDE can be used.

To insall via command line:
- macOS `brew install arduino-cli` or Linux `sudo apt install arduino-cli`
- `arduino-cli core install esp32:esp32@2.0.11`

### Edit pins_arduino.h

This project uses custom SPI pins, pins_arduino.h needs to be edited and can be found with the file paths provided below based on your platform.

- macOS ~/Library/Arduino15/packages/esp32/hardware/esp32/<version>/variants/esp32s3
- Windows C:\Users\<Username>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\<version>\variants\esp32s3
- Linux ~/.arduino15/packages/<platform_name>/hardware/<core_name>/<version>/cores/<core_name>/Arduino.h

Edit the file's MOSI, MISO and SCK lines to the following:
```
static const uint8_t MOSI  = 15;
static const uint8_t MISO  = 7;
static const uint8_t SCK   = 16;
```

### Libraries to Install

`FastLED                  3.6.0`\
`LovyanGFX                1.1.7`\
`NimBLE-Arduino           1.4.2`\
`lvgl                     8.3.8`\
`autowp-mcp2515           1.2.1`

Use the Library Manager to install the correct version of these libraries.

This can also be done via `arduino-cli` like so 

`arduino-cli lib install <library>@<version>`

Additionally place the header files used in this project (`Libraries/DashboardProjectHeaderFiles`) under `~/Documents/Arduino/libraries`

In the lvgl library place the `lv_conf.h` found in `Libraries/lv_conf.h` of this repository into `Documents/Arduino/Libraries/lvgl/src/`.

At this point you are ready to compile the code.