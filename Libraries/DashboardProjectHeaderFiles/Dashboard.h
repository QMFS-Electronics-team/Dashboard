#define LGFX_USE_V1
#define SERIAL_BAUDRATE 115200

// PIN DEFINITIONS
#define RGB 	       4
#define BUZZER 	       5
#define CANBUS_CS      40
#define SD_CS 		   41
#define BPS            GPIO_NUM_1
#define NETURAL_DETECT GPIO_NUM_42
#define SD_DETECT_GPIO GPIO_NUM_46

// CUSTOM SPI PIN DEFINITIONS (corresponding to arduino_pin.h)
// macOS (~/Library/Arduino15/packages/esp32/hardware/esp32/<version>/variants/esp32s3)
// Windows (C:\Users\<YourUsername>\AppData\Local\Arduino15\packages\esp32\hardware\esp32\<version>\variants\esp32s3)
// Linux (~/.arduino15/packages/<platform_name>/hardware/<core_name>/<version>/cores/<core_name>/Arduino.h)
#define CUSTOM_MISO 7
#define CUSTOM_MOSI 15
#define CUSTOM_SCLK 16

// LED DEFINITION
#define NUM_RPM_LEDS           10
#define LED_DEFAULT_BRIGHTNESS 20
#define MAX_RPM				   12000

// Buzzer
#define SILIENCE_BUZZER 1

// SCREEN DEFINITIONS
#define SCREEN_WIDTH 		   480
#define SCREEN_HEIGHT 		   320
#define SCREEN_COLOUR_DIVISOR  10
#define TFT_DEFAULT_BRIGHTNESS 100
#define DISPLAY_ROTATION       1

// Preferences
#define BPS_PCB_EN                   0 // When 1, the BPS is read from the PCB Pin and not the CAN BUS
#define NETURAL_DETECT_PCB_EN        0 // When 1, the Netural is determined by the PCB Pin and not the CAN BUS
#define ECU_TYPE 					 1 // 1 For S60 ECU, 0 For Standard ECU
#define CAN_BUS_SERIAL_OUTPUT_EN     1
#define BLE_GPS_SERIAL_OUTPUT_EN     1
#define CAN_BUS_SD_CARD_LOGGING_EN 	 1
#define GPS_SD_CARD_LOGGING_EN       1

// CAN BUS DEFINITIONS (Standard ECU)
#define CANBUS_DATA_COUNT 				6
#define PID_ECU_RESPONSE 				0x7E8
#define PID_ENGINE_RPM 					0x0C
#define PID_THROTTLE 					0x11
#define PID_COOLANT_TEMP 				0x05
#define PID_ENGINE_OIL_TEMP 			0x5C
#define PID_TRANSMISSION_ACTUAL_GEAR 	0xA4
#define PID_CONTROL_MODULE_VOLTAGE 		0x42 // Battery voltage
// #define PID_INTAKE_TEMP 0x0F

// CAN BUS DEFINITIONS (S60 ECU)
#define PID_2000 0x2000
#define PID_2001 0x2001
#define PID_2002 0x2002
#define PID_2003 0x2003
#define PID_2004 0x2004
#define PID_2005 0x2005

// Delays
#define DISPLAY_INIT_DELAY           350
#define LOADING_SCREEN_DELAY         10
#define CANBUS_START_DELAY           1000
#define RESTART_DELAY                1000
#define UPSHIFT_BLINK_DELAY          200
#define LED_ANIMATION_DELAY          250
#define BUZZER_TONE_DELAY            100
#define GPS_CONNECTION_COUNTER_DELAY 250

// GPS Module Bluetooth UUID
#define BLE_UART_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define BLE_TX_UUID   "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define GPS_SERIAL_FREQUENCY 8

// Timeout
#define TIMEOUT 500