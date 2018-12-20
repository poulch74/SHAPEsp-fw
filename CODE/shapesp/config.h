#define DEBUG_ADD_TIMESTAMP 1

#define DBGSERIAL Serial
// or Serial1

#define MAX_SENSORS_CNT 10 // max sensors IDX cnt for MQTT

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


typedef struct settings
{
   uint32_t crc;
   uint32_t size;

   ESP_CONFIG_S wifi;
   ESP_MQTT_S mqtt;
   ESP_TPRG_S tmr[10];
} ESP_SET;


/*
#pragma pack(push,1)
typedef struct __ESP_CFG_
{
   uint8_t payload[2048];
} ESP_CFG;
#pragma pack(pop)

const char defcfg[] PROGMEM=
"{"
"\"user\":\"root\","
"\"pwd\":\"esp8266\","
"\"wifi_mode\":0,"
"\"sta_ssid\":\"CH-Home\","
"\"sta_pwd\":\"chps74qwerty\","
"\"sta_dhcp\":0,"
"\"sta_ip\":\"192.168.137.88\","
"\"sta_gw\":\"192.168.137.1\","
"\"sta_subnet\":\"255.255.255.0\","
"\"ap_ssid\":\"esp8266\","
"\"ap_pwd\":\"esp8266\","
"\"ap_hidden\":0,"
"\"ap_chan\":5,"
"\"ap_ip\":\"192.168.4.1\","
"\"ap_gw\":\"192.168.4.1\","
"\"ap_subnet\":\"255.255.255.0\","
"\"skip_logon\":0,"
"\"mqtt_user\":\"\","
"\"mqtt_pwd\":\"\","
"\"mqtt_server\":\"localhost\","
"\"mqtt_clientID\":\"\","
"\"mqtt_inTopic\":\"domoticz/in\","
"\"mqtt_outTopic\":\"domoticz/out\","
"\"mqtt_willTopic\":\"domoticz/out\","
"\"mqtt_port\":1883,"
"\"mqtt_keepAlive\":15,"
"\"mqtt_qos\":0,"
"\"mqtt_retain\":0,"
"\"mqtt_relay\":0,"
"\"mqtt_mbtn\":0,"
"\"mqtt_vcc\":0,"
"\"mqtt_status\":0,"
"\"mqtt_mode\":0,"
"\"mqtt_sens0\":0,"
"\"mqtt_sens1\":0,"
"\"mqtt_sens2\":0"
"}";


template<typename T> void setSetting(const String& key, T value);
template<typename T> void setSetting(const String& key, unsigned int index, T value);
template<typename T> String getSetting(const String& key, T defValue);
template<typename T> String getSetting(const String& key, unsigned int index, T defValue);


struct JsonBundle {
  public:
    void parse(const char* json) { _jsonVariant = _jsonBuffer.parseObject(json); }
    void clear() { _jsonBuffer.clear(); }
    JsonObject& root() { return _jsonVariant; }

  private:
    DynamicJsonBuffer _jsonBuffer;
    JsonVariant _jsonVariant;
};


JsonBundle config;

*/
