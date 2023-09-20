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

### TFT Display to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** |
|----------------|---------------|------------|
|     5V         |     5V        |   RED      |
|     GND        |     GND       |   BLACK    |
|     CS         |     G22       |   ORANGE   |
|     RESET      |     G15       |   ORANGE   |
|     DC         |     G21       |   ORANGE   |
|     SDI (MOSI) |     G23       |   ORANGE   |
|     SCK        |     G18       |   ORANGE   |
|     LED        |     5V        |   RED      |
|     SDO (MISO) |     G19       |   ORANGE   |

### Micro SD Card to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** |
|----------------|---------------|------------|
|     5V         |     5V        |   RED      |
|     GND        |     GND       |   BLACK    |
|     MISO       |     G19       |   YELLOW   |
|     MOSI       |     G23       |   YELLOW   |
|     SCK        |     G18       |   YELLOW   |
|     CS         |     G5        |   YELLOW   |

### 3 Axis Gyro to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** |
|----------------|---------------|------------|
|     5V         |     5V        |   RED      |
|     GND        |     GND       |   BLACK    |
|     SDA        |     G21       |   BLUE     |
|     SCL        |     G22       |   BLUE     |

### GPS to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** |
|----------------|---------------|------------|
|     5V         |     5V        |   RED      |
|     GND        |     GND       |   BLACK    |
|     SDA        |     G21       |   BLUE     |
|     SCL        |     G22       |   BLUE     |

### RGB LEDs to ESP32

| **Device Pin** | **ESP32 Pin** | **Colour** |
|----------------|---------------|------------|
|     5V         |     5V        |   RED      |
|     GND        |     GND       |   BLACK    |
|     Din        |     G4        |   GREEN    |

### CAN Bus Module

| **Device Pin** | **ESP32 Pin** | **Colour** |
|----------------|---------------|------------|
|                |               |            |
|                |               |            |
|                |               |            |
|                |               |            |
|                |               |            |
|                |               |            |