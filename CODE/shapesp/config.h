#include <core_version.h>

// Do not replace macros unless running version older than 2.5.0
#if defined(ARDUINO_ESP8266_RELEASE_2_3_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_0) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_1) \
    || defined(ARDUINO_ESP8266_RELEASE_2_4_2)

// Quoting esp8266/Arduino comments:
// "Since __section__ is supposed to be only use for global variables,
// there could be conflicts when a static/inlined function has them in the
// same file as a non-static PROGMEM object.
// Ref: https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Variable-Attributes.html
// Place each progmem object into its own named section, avoiding conflicts"


#define __TO_STR_(A) #A
#define __TO_STR(A) __TO_STR_(A)

#undef PROGMEM

#define PROGMEM __attribute__((section( "\".irom.text." __FILE__ "." __TO_STR(__LINE__) "."  __TO_STR(__COUNTER__) "\"")))

// "PSTR() macro modified to start on a 32-bit boundary.  This adds on average
// 1.5 bytes/string, but in return memcpy_P and strcpy_P will work 4~8x faster"
#undef PSTR
#define PSTR(s) (__extension__({static const char __c[] __attribute__((__aligned__(4))) PROGMEM = (s); &__c[0];}))

#endif

#define DEBUG_ADD_TIMESTAMP 1

#define MAX_SENSORS_CNT 10 // max sensors IDX cnt for MQTT

// defines of devconfig

#define GPIO2_MODE_UNUSED  0
#define GPIO2_MODE_ALARM   1
#define GPIO2_MODE_1WIRE   2
#define GPIO2_MODE_DEBUG   3


typedef struct _ESP_TPRG_S
{
   uint8_t on_dowmask;
   uint8_t on_hour;
   uint8_t on_min;
   uint16_t on_ts;

   uint8_t off_dowmask;
   uint8_t off_hour;
   uint8_t off_min;
   uint16_t  off_ts;

   uint32_t active;
} ESP_TPRG_S;


typedef struct _ESP_CONFIG_S
{
   char     user[21];
   char     pwd[21];
   uint8_t  wifi_mode;
   char     sta_ssid[33];
   char     sta_pwd[65];
   uint8_t  sta_dhcp;
   uint32_t sta_ip;
   uint32_t sta_gw;
   uint32_t sta_subnet;
   char     ap_ssid[33];
   char     ap_pwd[65];
   uint8_t  ap_hidden;
   uint8_t  ap_chan;
   uint32_t ap_ip;
   uint32_t ap_gw;
   uint32_t ap_subnet;

   uint8_t  skip_logon;
} ESP_CONFIG_S;

typedef struct _ESP_MQTT_S
{
   uint32_t idx_relay;
   uint32_t idx_mbtn;
   uint32_t idx_vcc;
   uint32_t idx_status;
   uint32_t idx_mode;
   uint32_t idx_cmd[4];
   uint32_t idx_sens[MAX_SENSORS_CNT]; //72

   uint16_t port;
   uint16_t keepAlive;
   uint8_t  qos;
   uint8_t  retain;      //78

   char     user[21];
   char     pwd[21];

   char     server[65];
   char     clientID[33];
   char     inTopic[65];
   char     outTopic[65];
   char     willTopic[65];
} ESP_MQTT_S; // 413 bytes

typedef struct _ESP_DEV_S
{
   uint8_t type;
   uint8_t en_timer;
   uint8_t en_mqtt;
   uint8_t en_sensors;
   uint8_t scan_i2c;
   uint8_t scan_ds1w;
   uint8_t gpio2_mode;
   uint8_t gpio13_mode;
   // ....
} ESP_DEV_S;


typedef struct settings
{
   uint32_t crc;
   uint32_t size;

   ESP_CONFIG_S wifi;
   ESP_MQTT_S mqtt;
   ESP_TPRG_S tmr[10];
   ESP_DEV_S  dev;
} ESP_SET;


const char *dstring[] PROGMEM = {
   /*
   "FAILED read config!!! Writing defaults.", //0
   "DS3231 found at address 0x68. Setting SyncProvider... \n", //1
   "RTC clock not found! Setting fake millis() SyncProvider... \n", //2
   "Startup at: %s \n", //3
   "User: %s \n", //4
   "Pwd: %s \n", //5
   ".", //6
   "%d \n", //7
   "Connected to %s IP address %s \n", //8
   "AP is %s AP IP address %s \n", //9
   "Server started \n", //10
   // task.h
   "Uptime: %s \n", //11
   "Write config.\n", //12
   "Change user/pwd\n", //13
   "NetMask: %0X \n", //14
   */
   /*
   //ws.ino
   "ws[%s][%u] connect\n", //15
   "ws[%s][%u] disconnect: %u\n", //16
   "ws[%s][%u] error(%u): %s\n", //17
   "ws[%s][%u] pong[%u]: %s\n", //18
   "[WEBSOCKET] Error parsing data\n", //19
   "Hash server: %s \n", //20
   "Hash client: %s \n", //21
   "Auth OK\n", // 22
   "Auth FAIL\n", //23
   "No auth!!!!\n", //24
   */
   /*
   //relay.h
   "on valve start\n", //25
   "off valve start\n", //26
   "on relay\n", //27
   "off relay\n", //28
   "OFF relay\n", //29
   */
   //task_mqtt.h
   /*
   "Init MQTT task.\n", //30
   " Host: %s\n", //31
   " Port: %d\n", //32
   " ClientID: %s\n", //33
   " keepAlive: %d\n", //34
   " User: %s\n", //35
   " Password: %s\n", //36
   " QoS: %d\n", //37
   " Retain: %d\n", //38
   " Will topic: %s\n", //39
   " In topic: %s\n", //40
   " Out topic: %s\n", //41
   "Connected to MQTT.\n", //42
   "Session present: %d\n", //43
   "[DOMOTICZ] Error parsing data\n", //44
   "[DOMOTICZ] Status IGNORED\n", //45
   "[DOMOTICZ] Received value for Relay %u for IDX %u\n", //46
   "SCHEDULE OPEN MQTT\n", //47
   "SCHEDULE CLOSE MQTT\n", //48
   "[DOMOTICZ] Status IGNORED\n", //49
   "[DOMOTICZ] Received value for Button %u for IDX %u\n", //50
   "SCHEDULE AUTO MQTT\n", //51
   "setmqtt and response", //52
   */
   //i2c.ino
//   "[I2C] Device found at address 0x%02X\n", //53
  // "[I2C] No devices found\n", //54
   //task_sens.h
   //"BME280 found at address 0x76. Adding... \n", //55
   //"BME280 found at address 0x77. Adding... \n", //56
   //"I2C-1W bridge found at address 0x18. Adding... \n", //57
   //"found 1-Wire ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \n", //58
   //"CRC is not valid! \n", //59
   //"Add into sensors list! \n", //60
   //"No more addresses. \n", //61
   //"Sensor cnt %d %d\n", //62
   //task_timer
//   "SCHEDULE AUTO\n", // 63
//   "SCHEDULE CLOSE\n", //64
//   "SCHEDULE OPEN\n", //65
//   "settime and response", //66
   //tmerlib
   //"[SSDP] Schema request\n", //67
   //"[SSDP] Started\n", //68
   "last"
};


static const char dstring0[] PROGMEM = "FAILED read config!!! Writing defaults."; //0
static const char dstring1[] PROGMEM = "DS3231 found at address 0x68. Setting SyncProvider... \n"; //1
static const char dstring2[] PROGMEM = "RTC clock not found! Setting fake millis() SyncProvider... \n"; //2
static const char dstring3[] PROGMEM = "Startup at: %s \n"; //3
static const char dstring4[] PROGMEM = "User: %s \n"; //4
static const char dstring5[] PROGMEM = "Pwd: %s \n"; //5
static const char dstring6[] PROGMEM = "."; //6
static const char dstring7[] PROGMEM = "%d \n"; //7
static const char dstring8[] PROGMEM = "Connected to %s IP address %s \n"; //8
static const char dstring9[] PROGMEM = "AP is %s AP IP address %s \n"; //9
static const char dstring10[] PROGMEM = "Server started \n"; //10
// task.h
static const char dstring11[] PROGMEM = "Uptime: %s \n"; //11
static const char dstring12[] PROGMEM = "Write config.\n"; //12
static const char dstring13[] PROGMEM = "Change user/pwd\n"; //13
static const char dstring14[] PROGMEM = "NetMask: %0X \n"; //14

//ws.ino
static const char dstring15[] PROGMEM = "ws[%s][%u] connect\n"; //15
static const char dstring16[] PROGMEM = "ws[%s][%u] disconnect: %u\n"; //16
static const char dstring17[] PROGMEM = "ws[%s][%u] error(%u): %s\n"; //17
static const char dstring18[] PROGMEM = "ws[%s][%u] pong[%u]: %s\n"; //18
static const char dstring19[] PROGMEM = "[WEBSOCKET] Error parsing data\n"; //19
static const char dstring20[] PROGMEM = "Hash server: %s \n"; //20
static const char dstring21[] PROGMEM = "Hash client: %s \n"; //21
static const char dstring22[] PROGMEM = "Auth OK\n"; // 22
static const char dstring23[] PROGMEM = "Auth FAIL\n"; //23
static const char dstring24[] PROGMEM = "No auth!!!!\n"; //24

//relay.h
static const char dstring25[] PROGMEM = "on valve start\n"; //25
static const char dstring26[] PROGMEM = "off valve start\n"; //26
static const char dstring27[] PROGMEM = "on relay\n"; //27
static const char dstring28[] PROGMEM = "off relay\n"; //28
static const char dstring29[] PROGMEM = "OFF relay\n"; //29


//task_mqtt.h
static const char dstring30[] PROGMEM = "Init MQTT task.\n"; //30
static const char dstring31[] PROGMEM = " Host: %s\n"; //31
static const char dstring32[] PROGMEM = " Port: %d\n"; //32
static const char dstring33[] PROGMEM = " ClientID: %s\n"; //33
static const char dstring34[] PROGMEM = " keepAlive: %d\n"; //34
static const char dstring35[] PROGMEM = " User: %s\n"; //35
static const char dstring36[] PROGMEM = " Password: %s\n"; //36
static const char dstring37[] PROGMEM = " QoS: %d\n"; //37
static const char dstring38[] PROGMEM = " Retain: %d\n"; //38
static const char dstring39[] PROGMEM = " Will topic: %s\n"; //39
static const char dstring40[] PROGMEM = " In topic: %s\n"; //40
static const char dstring41[] PROGMEM = " Out topic: %s\n"; //41
static const char dstring42[] PROGMEM = "Connected to MQTT.\n"; //42
static const char dstring43[] PROGMEM = "Session present: %d\n"; //43
static const char dstring44[] PROGMEM = "[DOMOTICZ] Error parsing data\n"; //44
static const char dstring45[] PROGMEM = "[DOMOTICZ] Status IGNORED\n"; //45
static const char dstring46[] PROGMEM = "[DOMOTICZ] Received value for Relay %u for IDX %u\n"; //46
static const char dstring47[] PROGMEM = "SCHEDULE OPEN MQTT\n"; //47
static const char dstring48[] PROGMEM = "SCHEDULE CLOSE MQTT\n"; //48
static const char dstring49[] PROGMEM = "[DOMOTICZ] Status IGNORED\n"; //49
static const char dstring50[] PROGMEM = "[DOMOTICZ] Received value for Button %u for IDX %u\n"; //50
static const char dstring51[] PROGMEM = "SCHEDULE AUTO MQTT\n"; //51
static const char dstring52[] PROGMEM = "setmqtt and response"; //52


   //i2c.ino
static const char dstring53[] PROGMEM = "[I2C] Device found at address 0x%02X\n"; //53
static const char dstring54[] PROGMEM = "[I2C] No devices found\n"; //54


//task_sens.h
static const char dstring55[] PROGMEM = "BME280 found at address 0x76. Adding... \n"; //55
static const char dstring56[] PROGMEM = "BME280 found at address 0x77. Adding... \n"; //56
static const char dstring57[] PROGMEM = "I2C-1W bridge found at address 0x18. Adding... \n"; //57
static const char dstring58[] PROGMEM = "found 1-Wire ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \n"; //58
static const char dstring59[] PROGMEM = "CRC is not valid! \n"; //59
static const char dstring60[] PROGMEM = "Add into sensors list! \n"; //60
static const char dstring61[] PROGMEM = "No more addresses. \n"; //61
static const char dstring62[] PROGMEM = "Sensor cnt %d %d\n"; //62

//task_timer
static const char dstring63[] PROGMEM = "SCHEDULE AUTO\n"; // 63
static const char dstring64[] PROGMEM = "SCHEDULE CLOSE\n"; //64
static const char dstring65[] PROGMEM = "SCHEDULE OPEN\n"; //65
static const char dstring66[] PROGMEM = "settime and response"; //66


//tmerlib
static const char dstring67[] PROGMEM = "[SSDP] Schema request\n"; //67
static const char dstring68[] PROGMEM = "[SSDP] Started\n"; //68

static const char dstring69[] PROGMEM = "Change dev settings\n"; //6
