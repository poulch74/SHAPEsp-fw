extern "C" {
    #include "spi_flash.h"
}


uint16_t crc16(const uint8_t *msg, int msg_len)
{
   uint16_t crc = 0;
   const uint16_t crc_poly = 0x1021;         /* порождающий многочлен */
   while (msg_len-- > 0)
   {
      crc ^= (uint16_t)*msg++ << 8;
      for (uint8_t j = 8; j > 0; j--) { if (crc&0x8000) crc = crc<<1^crc_poly; else crc<<= 1; }
   }
   return crc;
}

// -----------------------------------------------------------------------------
// Reset
// -----------------------------------------------------------------------------
Ticker _defer_reset;
void reset() { ESP.restart(); }
void deferredReset(unsigned long delay) { _defer_reset.once_ms(delay, reset); }

// SSDP
const char _ssdp_template[] PROGMEM=
    "<?xml version=\"1.0\"?>"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
        "<specVersion>"
            "<major>1</major>"
            "<minor>0</minor>"
        "</specVersion>"
        "<URLBase>http://%s:%u/</URLBase>"
        "<device>"
            "<deviceType>%s</deviceType>"
            "<friendlyName>%s</friendlyName>"
            "<presentationURL>/</presentationURL>"
            "<serialNumber>%u</serialNumber>"
            "<modelName>%s</modelName>"
            "<modelNumber>%s</modelNumber>"
            "<modelURL>%s</modelURL>"
            "<manufacturer>%s</manufacturer>"
            "<manufacturerURL>%s</manufacturerURL>"
            "<UDN>uuid:38323636-4558-4dda-9188-cda0e6%06x</UDN>"
        "</device>"
    "</root>\r\n"
    "\r\n";


void handleSSDP(AsyncWebServerRequest *request)
{
   DEBUG_MSG_P(PSTR("[SSDP] Schema request\n"));

   IPAddress ip = WiFi.localIP();
   uint32_t chipId = ESP.getChipId();

   char response[strlen_P(_ssdp_template) + 100];
   snprintf_P(response, sizeof(response), _ssdp_template,
      ip.toString().c_str(),  // ip
      80,                          // port
      "upnp:rootdevice",                   // device type
      "SHAPEsp",     // friendlyName
      chipId,                             // serialNumber
      "SHAPEsp-async",                           // modelName
      "01",                        // modelNumber
      "http://www.google.com",                        // modelURL
      "SHAPEsp",                        // manufacturer
      "",                                 // manufacturerURL
      chipId                              // UUID
      );
   request->send(200, "text/xml", response);
}

void ssdpSetup()
{
   SSDP.setDeviceType("upnp:rootdevice");
   SSDP.setSchemaURL("description.xml");
   SSDP.setHTTPPort(80);
   SSDP.setName("SHAPEsp");
   SSDP.setSerialNumber(String(ESP.getChipId()));
   SSDP.setURL("/");
   SSDP.setModelName("SHAPEsp-async");
   SSDP.setModelNumber("01");
   SSDP.setModelURL("http://www.google.com");
   SSDP.setManufacturer("SHAPEsp");
   SSDP.setManufacturerURL("");
   SSDP.begin();

   DEBUG_MSG_P(PSTR("[SSDP] Started\n"));
}


void handleNotFound(AsyncWebServerRequest *request)
{
   request->send(404, "text/plain", "Not found");
}

#include "index_html.h"

void handleIndex(AsyncWebServerRequest *request)
{
   AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_htm, index_htm_sz);
   response->addHeader("Content-Encoding", "gzip");
//        response->addHeader("Last-Modified", _last_modified);
   response->addHeader("X-XSS-Protection", "1; mode=block");
   response->addHeader("X-Content-Type-Options", "nosniff");
   response->addHeader("X-Frame-Options", "deny");
   request->send(response);
}


/*
bool ReadConfig()
{
  memset(&cfg,0,sizeof(ESP_CONFIG));
  EEPROM.begin(4096);
  for(uint16_t i=0; i<sizeof(ESP_CONFIG); i++) cfg.b[i] = EEPROM.read(i);//rtc.eeprom_read(i);
  EEPROM.end();
  uint16_t c_crc = crc16(&(cfg.b[2]),sizeof(ESP_CONFIG)-2);
  if(cfg.s.crc!=c_crc) return false;
  return true;
}
*/

bool ReadConfig()
{
  uint8_t *bptr = (uint8_t *)&cfg;
  memset(&cfg,0,sizeof(ESP_SET));
  EEPROM.begin(4096);
  for(uint16_t i=0; i<sizeof(ESP_SET); i++) bptr[i] = EEPROM.read(i);//rtc.eeprom_read(i);
  EEPROM.end();
  if(cfg.size>sizeof(ESP_SET)) cfg.size = sizeof(ESP_SET);
  uint32_t c_crc = crc16(&(bptr[4]),cfg.size-4);
  if(cfg.crc!=c_crc) return false;
  return true;
}



void WriteConfig(bool def, bool clrtmr)
{
   if(def)
   {
      memset(&cfg,0,sizeof(ESP_SET));

      snprintf(cfg.wifi.user,21,"root");
      snprintf(cfg.wifi.pwd,21,"esp8266");

      cfg.wifi.wifi_mode = 0; // 0 ap 1 sta+ap 2 sta
      snprintf(cfg.wifi.sta_ssid,33,"CH-Home");
      snprintf(cfg.wifi.sta_pwd,65,"chps74qwerty");
      cfg.wifi.sta_dhcp = 0;
      cfg.wifi.skip_logon = 0;
      cfg.wifi.sta_ip = IPAddress(192,168,137,88);
      cfg.wifi.sta_gw = IPAddress(192,168,137,1);
      cfg.wifi.sta_subnet = IPAddress(255,255,255,0);
      sprintf(cfg.wifi.ap_ssid,"");
      sprintf(cfg.wifi.ap_pwd,"");
      cfg.wifi.ap_hidden = 0; // 1 - hidden
      cfg.wifi.ap_chan = 6;
      cfg.wifi.ap_ip = IPAddress(192,168,137,88);
      cfg.wifi.ap_gw = IPAddress(192,168,137,1);
      cfg.wifi.ap_subnet = IPAddress(255,255,255,0);
      cfg.wifi.sysl_ip = IPAddress(192,168,137,1);
      cfg.wifi.sysl_ena = 1;
      String mac = WiFi.macAddress();
      String hostname = "SHAPEsp_"+mac.substring(12,14)+mac.substring(15);
      snprintf(cfg.wifi.hostname,33,hostname.c_str());

      snprintf(cfg.mqtt.user,20,"");
      snprintf(cfg.mqtt.pwd,20,"");
      snprintf(cfg.mqtt.server,64,"localhost");
      snprintf(cfg.mqtt.clientID,32,"");
      snprintf(cfg.mqtt.inTopic,64,"domoticz/in");
      snprintf(cfg.mqtt.outTopic,64,"domoticz/out");
      snprintf(cfg.mqtt.willTopic,64,"domoticz/out");
      cfg.mqtt.port = 1883;
      cfg.mqtt.keepAlive = 15;
      cfg.mqtt.qos = 0;
      cfg.mqtt.retain = 0;

      // cfg.dev 0 by default but
      cfg.dev.adc_coef = 1563;
   }

   if(clrtmr)
   {
      memset(&(cfg.tmr),0,10*sizeof(ESP_TPRG_S));
   }

   uint8_t *bptr =(uint8_t *)&cfg;
   cfg.size = sizeof(ESP_SET);
   cfg.crc = crc16(&bptr[4],cfg.size-4);

   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_SET); i++) { EEPROM.write(i,bptr[i]); }
   EEPROM.end();
}

/*

bool ReadTmrPrg()
{
   memset(&prg,0,sizeof(ESP_TPRG));
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_TPRG); i++) prg.b[i] = EEPROM.read(512+i);//rtc.eeprom_read(512+i);
   EEPROM.end();
   uint16_t c_crc = crc16(&(prg.b[2]),sizeof(ESP_TPRG)-2);
   if(prg.ta.crc!=c_crc) return false;
   return true;
}

void SaveTmrPrg(bool def)
{
   if(def) { memset(&prg,0,sizeof(ESP_TPRG)); }
   prg.ta.crc = crc16(&(prg.b[2]),sizeof(ESP_TPRG)-2);
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_TPRG); i++) { EEPROM.write(512+i,prg.b[i]);}
   EEPROM.end();
}

bool ReadMqttSettings()
{
   memset(&mqttset,0,sizeof(ESP_MQTT));
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_MQTT); i++) mqttset.b[i] = EEPROM.read(1024+i);
   EEPROM.end();
   uint16_t c_crc = crc16(&(mqttset.b[2]),sizeof(ESP_MQTT)-2);
   if(mqttset.s.crc!=c_crc) return false;
   return true;
}

void SaveMqttSettings(bool def)
{
   if(def)
   {
      memset(&mqttset,0,sizeof(ESP_MQTT));
      snprintf(mqttset.s.user,20,"");
      snprintf(mqttset.s.pwd,20,"");
      snprintf(mqttset.s.server,64,"localhost");
      snprintf(mqttset.s.clientID,32,"");
      snprintf(mqttset.s.inTopic,64,"domoticz/in");
      snprintf(mqttset.s.outTopic,64,"domoticz/out");
      snprintf(mqttset.s.willTopic,64,"domoticz/out");
      mqttset.s.port = 1883;
      mqttset.s.keepAlive = 15;
      mqttset.s.qos = 0;
      mqttset.s.retain = 0;
   }

   mqttset.s.crc = crc16(&(mqttset.b[2]),sizeof(ESP_MQTT)-2);
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_MQTT); i++) { EEPROM.write(1024+i,mqttset.b[i]); }
   EEPROM.end();
}
*/
String FmtMqttMessage(int idx, int nvalue, const char *svalue)
{
   char buf[128];
   snprintf(buf, sizeof(buf),
            "{\"command\":\"udevice\",\"idx\":%d,\"RSSI\":%d,\"Battery\":%d,\"nvalue\":%d,\"svalue\":\"%s\"}",
            idx, abs(rssi/10), battery, nvalue, svalue
           );
   return String(buf);
}




extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

String getCoreVersion() {
    String version = ESP.getCoreVersion();
    #ifdef ARDUINO_ESP8266_RELEASE
        if (version.equals("00000000")) {
            version = String(ARDUINO_ESP8266_RELEASE);
        }
    #endif
    version.replace("_", ".");
    return version;
}

String getCoreRevision() {
    #ifdef ARDUINO_ESP8266_GIT_VER
        return String(ARDUINO_ESP8266_GIT_VER);
    #else
        return String("");
    #endif
}

unsigned int info_bytes2sectors(size_t size) { return (int) (size + SPI_FLASH_SEC_SIZE - 1) / SPI_FLASH_SEC_SIZE; }

unsigned long info_ota_space() { return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000; }

unsigned long info_filesystem_space() { return ((uint32_t)&_SPIFFS_end - (uint32_t)&_SPIFFS_start); }

unsigned long info_eeprom_space() {
    return EEPROM.length();
}

void _info_print_memory_layout_line(const char * name, unsigned long bytes, bool reset) {
    static unsigned long index = 0;
    if (reset) index = 0;
    if (0 == bytes) return;
    unsigned int _sectors = info_bytes2sectors(bytes);
    DEBUG_MSG_P(PSTR("[INIT] %-20s: %8lu bytes / %4d sectors (%4d to %4d)\n"), name, bytes, _sectors, index, index + _sectors - 1);
    index += _sectors;
}

void _info_print_memory_layout_line(const char * name, unsigned long bytes) {
    _info_print_memory_layout_line(name, bytes, false);
}


void info()
{
   //DEBUG_MSG_P(buf,PSTR("[INIT] %s %s\n"), (char *) APP_NAME, (char *) APP_VERSION);
   //DEBUG_MSG_P(buf,PSTR("[INIT] %s\n"), (char *) APP_AUTHOR);
   //DEBUG_MSG_P(buf,PSTR("[INIT] %s\n\n"), (char *) APP_WEBSITE);
   DEBUG_MSG_P(PSTR("[INIT] CPU chip ID: 0x%06X\n"), ESP.getChipId());
   DEBUG_MSG_P(PSTR("[INIT] CPU frequency: %u MHz\n"), ESP.getCpuFreqMHz());
   DEBUG_MSG_P(PSTR("[INIT] SDK version: %s\n"), ESP.getSdkVersion());
   DEBUG_MSG_P(PSTR("[INIT] Core version: %s\n"), getCoreVersion().c_str());
   DEBUG_MSG_P(PSTR("[INIT] Core revision: %s\n"), getCoreRevision().c_str());


   // -------------------------------------------------------------------------

   FlashMode_t mode = ESP.getFlashChipMode();

   DEBUG_MSG_P(PSTR("[INIT] Flash chip ID: 0x%06X\n"), ESP.getFlashChipId());
   DEBUG_MSG_P(PSTR("[INIT] Flash speed: %u Hz\n"), ESP.getFlashChipSpeed());
   DEBUG_MSG_P(PSTR("[INIT] Flash mode: %s\n\n"), mode == FM_QIO ? "QIO" : mode == FM_QOUT ? "QOUT" : mode == FM_DIO ? "DIO" : mode == FM_DOUT ? "DOUT" : "UNKNOWN");

   _info_print_memory_layout_line("Flash size (CHIP)", ESP.getFlashChipRealSize(), true);
   _info_print_memory_layout_line("Flash size (SDK)", ESP.getFlashChipSize(), true);
   _info_print_memory_layout_line("Reserved", 1 * SPI_FLASH_SEC_SIZE, true);
   _info_print_memory_layout_line("Firmware size", ESP.getSketchSize());
   _info_print_memory_layout_line("Max OTA size", info_ota_space());
   _info_print_memory_layout_line("SPIFFS size", info_filesystem_space());
   _info_print_memory_layout_line("EEPROM size", info_eeprom_space());
   _info_print_memory_layout_line("Reserved", 4 * SPI_FLASH_SEC_SIZE);
   DEBUG_MSG_P(PSTR("\n"));


    // -------------------------------------------------------------------------

   FSInfo fs_info;
   bool fs = SPIFFS.info(fs_info);
   if (fs)
   {
      DEBUG_MSG_P(PSTR("[INIT] SPIFFS total size: %8u bytes / %4d sectors\n"), fs_info.totalBytes, fs_info.totalBytes/SPI_FLASH_SEC_SIZE);
      DEBUG_MSG_P(PSTR("[INIT]        used size:  %8u bytes\n"), fs_info.usedBytes);
      DEBUG_MSG_P(PSTR("[INIT]        block size: %8u bytes\n"), fs_info.blockSize);
      DEBUG_MSG_P(PSTR("[INIT]        page size:  %8u bytes\n"), fs_info.pageSize);
      DEBUG_MSG_P(PSTR("[INIT]        max files:  %8u\n"), fs_info.maxOpenFiles);
      DEBUG_MSG_P(PSTR("[INIT]        max length: %8u\n\n"), fs_info.maxPathLength);
   }
   else
   {
      DEBUG_MSG_P(PSTR("[INIT] No SPIFFS partition\n\n"));
   }
}
