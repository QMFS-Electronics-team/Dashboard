//-------------------------------------
// Definitions for Integrated Programs
//-------------------------------------

// Display
#define TFT_SCK               18
#define TFT_MOSI              23
#define TFT_MISO              19
#define TFT_CS                0
#define TFT_DC                3
#define TFT_RESET             15

// GUI Default Values
#define TEXT_SIZE             3
#define BACKGROUND_COLOUR     BLACK
#define FONT_COLOUR           ORANGE
#define START_COLUMN          20
#define HORIZONTAL            3
#define COLUMN_OFFSET         220

// Rows for Information
#define RPM_ROW               10
#define MPH_ROW               40
#define GEAR_ROW              70
// #define FUEL_ROW              70
#define TPS_ROW               100
#define WATER_TEMP_ROW        130
#define OIL_TEMP_ROW          160
#define BATTERY_VOLTAGE_ROW   190

// LEDs
#define LED_PIN               4
#define NUM_LEDS              30
#define BRIGHTNESS            64
#define MAX_SHIFT_RPM         3000
#define COLOR_ORDER           GRB
#define LED_TYPE              WS2811

// GPS
#define TXPIN                 35
#define RXPIN                 34
#define GPSBAUD               9600
#define GPSCPUTIME            20

// SD Card
#define SDCS                  5

// CAN BUS
#define MCPCS                 2
#define CANBUSCPUTIME         20

// Serial
#define BAUDRATE              115200