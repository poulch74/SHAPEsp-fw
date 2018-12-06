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
