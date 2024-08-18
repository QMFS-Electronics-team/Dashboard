# Dashboard System Design


## Evaluation Kit Electronic Devices
The below table are for components used for evaluating the feasibility of the Dashboard System.

| Device               | Schematic Available | Purchase Link | Schematic Source | Reference Guides |
|----------------------|---------------------|---------------|------------------|------------------|
| ESP32 | | [Amazon](https://www.amazon.co.uk/Freenove-ESP32-WROVER-Bluetooth-Compatible-Tutorials/dp/B09BC5CNHM/ref=sr_1_3?crid=DL80XOHVR1E2&keywords=esp32+freenove+wrover&qid=1695242108&sprefix=esp+32+freenove+wrov%2Caps%2C233&sr=8-3) |                  |                  |
| TFT Display | | [Amazon](https://www.amazon.co.uk/CNBTR-Serial-Module-ILI9341-Support/dp/B01EHH5H3Q/ref=sr_1_5?crid=15GZHEMCL7IDM&keywords=2.8%22+inch+tft+lcd+240x320&qid=1695242969&sprefix=2.8+inch+tft+lcd+240x320%2Caps%2C89&sr=8-5)      |                  |                  |
| Micro SD Card Reader | | [EBay](https://www.ebay.co.uk/itm/265832101025?hash=item3de4d3f4a1:g:8w4AAOSwjdVi45sK&amdata=enc%3AAQAIAAABADXaDNdAkm%2BE%2BS9MCNpA8KmDrsAcr2rU4MvfG3YlDG62jKaL3QGSC9QMQWbZSIOhs2%2FO9Lkf9B2LIof%2BZMyf%2BQzyyRtA3KMLL%2BBmS4XT2S9TW%2B2dmpc795TyY4iddq2QwBgq0Q1uid3%2BJ8dUpd1Nqo1qiHCD7%2BUj%2BG8ZYAbXqpssGDLQjQkgGzBLenM0CepicRdsaEIcP8LzpTCK39PyP7cEQecCSDZ7HwjdxAlrCb%2BkdV9kO740egU0MYalxkQsUvqyLuvM8eJ%2BBvjbT3iv%2B%2Beo5Lk4UYaKvCxmucYOP9sZ10Is8nx4wu2FisEPxRVD0LSwj1qkrSSFRODwruWE3%2Fo%3D%7Ctkp%3ABk9SR6Ky5sXWYg)      |                  |                  |
| 3 Axis Gyro + Accelerometer| | [EBay](https://www.ebay.co.uk/itm/124642169193?hash=item1d05409169:g:hkgAAOSw7dZgCwdG&amdata=enc%3AAQAIAAAA0OzaEi3mOqHJvyv4dvd%2F8qfO8S0qOpn%2FDwA746INdyIA%2FdAzMSw6EukrmrzkzGlZio6gfn9ScNNGaox61FgfABfJMdPgYu0KXAtVn5tHgcjNOt3Gkw51XfGACEeEIGRrtBDGUymCPwDOGP28h9yHWsKvKo5MmWRhO09ITcIv%2Fy7QyeRCSypXy8tiPuVBhJW9q64w%2FmGctFEKDzZnPP6Pa5tvy0H2MzGs%2Bf3Fk3L0CIjbbz%2BRhiJkghjmVSn1LrE6AtmYrOkp%2BJI3M1F08nudT78%3D%7Ctkp%3ABFBMnO73xdZi)      |                  |                  |
| GPS | | [Amazon](https://www.amazon.co.uk/Beitian-HMC5883-Compass-Glonass-Antenna/dp/B07RHJ2NN5/ref=sr_1_1?crid=1OV81NRJ1GWM6&keywords=DIYmalls+BN-880&qid=1695242508&sprefix=diymalls+bn-880%2Caps%2C72&sr=8-1) |                  |                  |
| CAN Bus Module | | [Amazon](https://www.amazon.co.uk/ALAMSCN-MCP2515-Receiver-Compatible-Raspberry/dp/B091DXBT6F/ref=sr_1_7?crid=NVLJTZ3FJIJL&keywords=mcp2515&qid=1695242647&sprefix=alamscn+4pcs+mcp2515%2Caps%2C273&sr=8-7) |                  |                  |
| RGB LEDs | | [EBay](https://www.ebay.co.uk/itm/126010077289?hash=item1d56c93069:g:JMoAAOSwyvJkrQVZ&amdata=enc%3AAQAIAAAA8JLVYpBUR2wWPel8B3e58c4Wz1ysuXTZ7Eod5I0wfizlKxO%2F%2B2Q34Pzm8E2V55fuI9oWa%2B9c1d3rfm9BfHjeVw9CcydXsumm%2BnVXzZDUI7o1ecw85UeLP%2Bg6pqcevH7ysv3EVCnk7qtfdtKbLnrApEKwoE%2F4%2BbpmxlWO%2B6dlBT6ySK6QwZBJ5fCjAHYKBfRU80knJ6E8i8oa1JgW28OL4Wjgz%2BchIXLWQzXmPFINlGe0Af1l8WvpojTDCJGQKJLO%2BDJfrbUmb4Iup1NdVl1Du9P897WapQYKvvq1j4J9XRBlceujV8hcG2tbBiTGqOuIrw%3D%3D%7Ctkp%3ABk9SR6SjhcbWYg) |                  |                  |

## Evalution Kit Wiring Table

### Abbreviation Key


### TFT Display (ILI9341 2.8" 240x320) to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** | **Device Specific** |
|----------------|---------------|------------|---------------------|
|     5V         |     5V        |   RED      |         NO          |
|     GND        |     GND       |   BLACK    |         NO          |
|     CS         |     G0        |   WHITE    |         YES         |
|     RESET      |     G15       |   WHITE    |         YES         |
|     DC         |     G3        |   WHITE    |         YES         |
|     SDI (MOSI) |     G23       |   WHITE    |         NO          |
|     SCK        |     G18       |   WHITE    |         NO          |
|     LED        |     5V        |   RED      |         NO          |
|     SDO (MISO) |     G19       |   WHITE    |         NO          |


### TFT Display (ILI9486 3.5" 480x320) to ESP 32

Note this is currently used without any other devices connected to the ESP 32

| **Device Pin** | **Prototype Board** | **ESP32 Pin** | **Colour** |
|----------------|---------------------|---------------|------------|
|     RESET      |     D19             |     X         |            |
|     3.3V       |     D18             |     3.3V      |            |
|     5V         |     D17             |     5V        |            |
|     GND        |     D16             |     GND       |            |
|     GND        |     D15             |     GND       |            |
|     X          |     D14             |     X         |            |
|     LCD_RD     |     D0              |     G2        |            |
|     LCD_WR     |     D1              |     G4        |            |
|     LCD_RS     |     D2              |     G18       |            |
|     LCD_CS     |     D3              |     G33       |            |
|     LCD_RST    |     D5              |     G32       |            |
|     F_CS       |     D6              |     X         |            |
|     X          |     A15             |     X         |            |
|     X          |     A14             |     X         |            |
|     SD_SCK     |     A13             |     X         |            |
|     SD_D0      |     A12             |     X         |            |
|     SD_D1      |     A11             |     X         |            |
|     LCD_SS     |     A10             |     X         |            |
|     LCD_D1     |     A9              |     G13       |            |
|     LCD_D0     |     A8              |     G12       |            |
|     LCD_D7     |     A7              |     G14       |            |
|     LCD_D6     |     A6              |     G27       |            |
|     LCD_D5     |     A5              |     G5        |            |
|     LCD_D4     |     A4              |     G21       |            |
|     LCD_D3     |     A3              |     G25       |            |
|     LCD_D2     |     A2              |     G26       |            |
|     X          |     A1              |     X         |            |
|     X          |     A0              |     X         |            |


### Micro SD Card to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** | **Device Specific** |
|----------------|---------------|------------|---------------------|
|     5V         |     5V        |   RED      |        NO           |
|     GND        |     GND       |   BLACK    |        NO           |
|     MISO       |     G19       |   YELLOW   |        NO           |
|     MOSI       |     G23       |   YELLOW   |        NO           |
|     SCK        |     G18       |   YELLOW   |        NO           |
|     CS         |     G5        |   YELLOW   |        YES          |


### 3 Axis Gyro to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** | **Device Specific** |
|----------------|---------------|------------|---------------------|
|     5V         |     5V        |   RED      |        NO           |
|     GND        |     GND       |   BLACK    |        NO           |
|     SCL        |     G22       |   BLUE     |        NO           |
|     SDA        |     G21       |   BLUE     |        NO           |


### GPS to ESP32

Note the numbers are from left to right based on the barcode sticker on the GPS module.


!["GPS Module"](/images/BN-880-GPS.PNG)

| **Device Pin** | **ESP32 Pin** | **Colour** | **Device Specific** |
|----------------|---------------|------------|---------------------|
|   D - SDA      |     G21       |   BLACK    |        NO           |
|   G - GND      |     GND       |   WHITE    |        NO           |
|   T - TX       |     G35       |   GREEN    |        YES          |
|   R - RX       |     G34       |   RED      |        YES          |
|   V - 5V       |     5V        |   YELLOW   |        NO           |
|   C - SCL      |     G22       |   BLUE     |        NO           |


### RGB LEDs to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** | **Device Specific** |
|----------------|---------------|------------|---------------------|
|     5V         |     5V        |   RED      |        NO           |
|     GND        |     GND       |   BLACK    |        NO           |
|     Din        |     G4        |   GREEN    |        YES          |


### CAN Bus Module to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** | **Device Specific** |
|----------------|---------------|------------|---------------------|
|     5V         |     5V        |   RED      |        NO           |
|     GND        |     GND       |   BLACK    |        NO           |
|     CS         |     G2        |   WHITE    |        YES          |
|     MISO       |     G19       |   BLUE     |        NO           |
|     MOSI       |     G23       |   BLUE     |        NO           |
|     SCK        |     G18       |   WHITE    |        NO           |
|     INT        |     GX        |   N/A      |        X            |


### ESP32 Pin Utilisation

| **Device Pin** | **Used** | **Usage** |
| -------------- | -------- | --------- |
|     3.3V       |    NO    |   N/A     |
|     EN         |    NO    |   N/A     | 
|     G36/VP     |    NO    |   N/A     | 
|     G39/VN     |    NO    |   N/A     | 
|     G34        |    YES   |   GPS RX  |
|     G35        |    YES   |   GPS TX  |
|     G32        |    NO    |   N/A     |
|     G33        |    NO    |   N/A     |
|     G25        |    NO    |   N/A     |
|     G26        |    NO    |   N/A     |
|     G27        |    NO    |   N/A     |
|     G14        |    NO    |   N/A     |
|     G12        |    NO    |   N/A     |
|     GND        |    YES   |   Power   |
|     G13        |    NO    |   N/A     |
|     G9/SD2     |    NO    |   N/A     |
|     G10/SD3    |    NO    |   N/A     |
|     G11/CMD    |    NO    |   N/A     |
|     5V         |    YES   |   Power   |
|     5V         |    YES   |   Power   |
|     GND        |    YES   |   Power   |
|     G23        |    YES   |   MOSI    |
|     G22        |    YES   |   SCL     |
|     G1/TX      |    NO    |   N/A     |
|     G3/RX      |    YES   |   TFT DC  |
|     G21        |    YES   |   SDA     |
|     GND        |    YES   |   Power   |
|     G19        |    YES   |   MISO    |
|     G18        |    YES   |   SCK     |
|     G5         |    YES   |   SD CS   |
|     GND        |    YES   |   Power   |
|     GND        |    YES   |   Power   |
|     G4         |    YES   |   LED Din |
|     G0         |    YES   |   TFT CS  |
|     G2         |    YES   |   CAN CS  |
|     G15        |    YES   |   TFT RST |
|     G8/SD1     |    NO    |   N/A     |
|     G7/SD0     |    NO    |   N/A     |
|     G6/CLK     |    NO    |   N/A     |
|     GND        |    YES   |   Power   |

