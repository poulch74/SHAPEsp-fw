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
   uint8_t   on_dowmask;
   uint8_t   on_hour;
   uint8_t   on_min;
   uint16_t  on_ts;

   uint8_t   off_dowmask;
   uint8_t   off_hour;
   uint8_t   off_min;
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
   uint32_t sysl_ip;
   uint8_t  sysl_ena;
   char     hostname[33];

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
   uint8_t  type;
   uint8_t  en_timer;
   uint8_t  en_mqtt;
   uint8_t  en_sensors;
   uint8_t  scan_i2c;
   uint8_t  scan_ds1w;
   uint8_t  gpio2_mode;
   uint8_t  gpio13_mode;
   uint32_t adc_coef;
   uint16_t sdelay;
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

