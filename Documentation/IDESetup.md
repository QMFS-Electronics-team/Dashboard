# IDE Setup

This markdown file gives instructions on how to setup an IDE for software development for the Dashboard.

## IDE Setup

There are two options for IDEs

- VSCode
- Arduino IDE

### VSCode

#### Install VSCode

At the time of development VSCode `1.147.0` was used.

Here is the [installer](https://code.visualstudio.com/download) for the VSCode.

If you want to install via the commandline -

##### macOS - VSCode Commandline Install

```bash
brew install visual-studio-code
```

##### Linux (Ubuntu) - VSCode Commandline Install

```bash
    sudo snap install --classic code
```

or follow this [guide](https://code.visualstudio.com/docs/setup/linux) if you want to install with `apt`

#### VSCode Devcontainer

If you want to use the devcontainer make sure you have Docker installed on your computer. `CMD+SHIFT+P` or `CTL+SHIFT+P` and select `reopen in container`. Everything should be configured for you automatically.

##### Windows VSCode Devcontainer COM Port Setup

If you are using the dev container on Windows make sure you can access the com ports on the machine.

From a powershell window.

```powershell
install usbipd 'winget install usbipd' # Identify your COM device
```

Run a powershell windows as admin then run the following commands

```powershell
usbipd list
usbipd bind --busid 2-3 # Replace the number with your busid
usbipd attach --wsl --busid 2-3
```

##### Limitations of Devcontainer

With the Arduino Community Edition extension in the devcontiner it is not possible to select the serial port from the IDE. It is recommended to use the commandline to upload if using the devcontainer.

#### Install Recommended Extensions for VSCode

Install the recommended extensions for VSCode, use the following shortcut - `CTRL + SHIFT + X` or `CMD + SHIFT + X`, then install recommended extensions.

Open up the repository with VSCode. Once the `.vscode` directory is generated, navigate to `arduino.json`. Append the following lines in addition to what is already present. The `Output` folder contains the build and having this folder speeds up the build time as it servers as a cache.

```json
{
    "output": "Arduino_Output",
    "arduino.enableCache": true,
    "arduino.logLevel": "info"
}
```

### Arduino IDE

### Install Arduino IDE

For this project Arduino IDE `2.3.11` was used.

Here is the [installer](https://www.arduino.cc/en/software) for Arduino IDE

If you want to install via the commandline -

#### macOS - Arduino IDE Install

```bash
brew install arduino-ide
```

#### Linux - Arduino IDE Install

```bash
sudo apt install arduino-ide
```

### Setup ESP Exception Decode

ESP Exception Decoder is a useful debugging tool when you have a stack trace from ESP32. Follow [this](https://github.com/dankeboy36/esp-exception-decoder) for instructions.

## Dependency Setup

### Install ESP32 Core

For this project, ESP32 Core Version: `2.0.11` has been used and the ESP32 package file that is kept under the Arduino15 folder.

This version of the ESP32 Core can be installed via the board manager on VSCode (Arduino extension) or on Arduino IDE.

To install via command line arduino-cli can be used. Note that for this project arduino-cli `1.6.0` has been used.

#### macOS - Install arduino-cli

```bash
brew install arduino-cli
```

#### Linux - Install arduino-cli

```bash
sudo apt install arduino-cli
```

Then run

```bash
arduino-cli core install esp32:esp32@2.0.11
```

### Edit pins_arduino.h

This project uses custom SPI pins, `pins_arduino.h` needs to be edited and can be found with the file paths provided below based on your platform.

#### macOS - Path to pins_arduino.h

`~/Library/Arduino15/packages/esp32/hardware/esp32/<version>/variants/esp32s3`

#### Windows - Path to pins_arduino.h

`C:\Users\<Username>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\<version>\variants\esp32s3`

#### Linux - Path to pins_arduino.h

`~/.arduino15/packages/esp32/hardware/esp32/2.0.11/variants/esp32s3/`

Edit the file's MOSI, MISO and SCK lines to the following:

```C++
static const uint8_t MOSI  = 15;
static const uint8_t MISO  = 7;
static const uint8_t SCK   = 16;
```

or replace the file with the one found in this repository `Libraries/BoardSupport/pins_arduino/pins_arduino.h`

### Libraries to Install

#### Libraries for Dashboard Program

```plain
FastLED                  3.6.0
LovyanGFX                1.1.7
NimBLE-Arduino           1.4.2
lvgl                     8.3.8
autowp-mcp2515           1.2.1
```

#### Libraries for Testing Programs

```plain
Adafruit BusIO           1.16.1
Adafruit HMC5883 Unified 1.2.3
Adafruit MPU6050         2.2.6
Adafruit Unified Sensor  1.1.14
EspSoftwareSerial        8.1.0
GFX Library for Arduino  1.4.1
TinyGPSPlus              1.0.3 
```

Use the Library Manager to install the correct version of these libraries.

This can also be done via `arduino-cli` like so

`arduino-cli lib install <library>@<version>`

To install all of the dependences in one go use the following command

Libraries for the Dashboard Program -

```bash
arduino-cli lib install "FastLED@3.6.0" "LovyanGFX@1.1.7" "NimBLE-Arduino@1.4.2" "lvgl@8.3.8" "autowp-mcp2515@1.2.1"
```

Libraries for the testing Programs -

```bash
arduino-cli lib install "Adafruit BusIO@1.16.1" "Adafruit HMC5883 Unified@1.2.3" "GFX Library for Arduino@1.4.1" "EspSoftwareSerial@8.1.0" "TinyGPSPlus@1.0.3" "Adafruit MPU6050@2.2.6" "Adafruit Unified Sensor@1.1.14"
```

### Custom Libraries for Required Project

Additionally place the header files used in this project (`Libraries/DashboardLibraries/DashboardProjectHeaderFiles`) under `~/Documents/Arduino/libraries/`

In the lvgl library place the `lv_conf.h` found in `Libraries/DashboardLibraries/lv_conf.h` of this repository into `~/Documents/Arduino/libraries/lvgl/src/`.

## Usage - Compile and Uploading

### Compile

Setup the board config as per the following settings:

```plain
    Board: ESP32S3 Dev Module (esp32)
    Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
    Upload Speed: 115200
```

At this point you are ready to compile the code.

From Arduino IDE you can compile by clicking on the verify button.

From VSCode enter `CMD+SHIFT+P` and enter `Arduino: verify`

From the commandline you can use the following command from the directory `Dashboard_Program/`

```bash
    arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=8M,PartitionScheme=huge_app,PSRAM=enabled --output-dir ../build --build-path ../Arduino_Output Dashboard_Program.ino
```

### Upload

From Arduino IDE you can compile by clicking on the upload button.

From VSCode enter `CMD+SHIFT+P` and enter `Arduino CLI: Upload`

From the commandline you can use the following command from the the directory `Dashboard_Program/`

```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32s3:FlashSize=8M,PartitionScheme=huge_app,PSRAM=enabled,UploadSpeed=115200 --input-dir ../build Dashboard_Program.ino
```
