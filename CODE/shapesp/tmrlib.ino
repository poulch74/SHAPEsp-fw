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

void reset() {
    ESP.restart();
}

void deferredReset(unsigned long delay)
{
    _defer_reset.once_ms(delay, reset);
}

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
   DEBUG_MSG("[SSDP] Schema request\n");

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

   DEBUG_MSG("[SSDP] Started\n");
}


void handleNotFound(AsyncWebServerRequest *request)
{
   request->send(404, "text/plain", "Not found");
}

#include "index_html.h"

void handleIndex(AsyncWebServerRequest *request)
{
   //AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/index.html.gz","text/html");
   AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_htm, (size_t)8363);
   response->addHeader("Content-Encoding", "gzip");
//        response->addHeader("Last-Modified", _last_modified);
   response->addHeader("X-XSS-Protection", "1; mode=block");
   response->addHeader("X-Content-Type-Options", "nosniff");
   response->addHeader("X-Frame-Options", "deny");
   request->send(response);
}


bool is_auth(AsyncWebServerRequest *request)
{
    /*
   if(cfg.s.skip_logon)
   {
      do
      {
         IPAddress remip = server.client().remoteIP();
         DbgPrint(("CONNECT FROM : ")); DbgPrintln((remip.toString()));

         IPAddress locip = server.client().localIP();
         DbgPrint(("My IP : ")); DbgPrintln((locip.toString()));

         IPAddress gw(cfg.s.sta_gw[0],cfg.s.sta_gw[1],cfg.s.sta_gw[2],cfg.s.sta_gw[3]);
         IPAddress mask(cfg.s.sta_subnet[0],cfg.s.sta_subnet[1],cfg.s.sta_subnet[2],cfg.s.sta_subnet[3]);
//         if((remip[0]==gw[0])&&(remip[1]==gw[1])&&(remip[2]==gw[2])&&(remip[3]==gw[3]))
//         {
//            break;
//         }
//         else
         {
            for(int i=0;i<4;i++) {locip[i]&=mask[i]; remip[i]&=mask[i];}
            if((remip[0]==locip[0])&&(remip[1]==locip[1])&&(remip[2]==locip[2])&&(remip[3]==locip[3]))
            return true; // skip logon
         }
      } while(0);         
      DbgPrint(("Remote client!!!"));
   }
*/
/*
   DbgPrintln(("Enter is_authentified"));
   if (request->hasHeader("Cookie")){   
      DbgPrint(("Found cookie: "));
      String cookie = request->header("Cookie");
      DbgPrintln((cookie));
      if (cookie.indexOf("ESPSESSIONID=") != -1) {
         String si = cookie.substring(cookie.indexOf('=')+1);
         long lsi = si.toInt();
         if(lsi==session_id)
         {
            DbgPrintln(("Authentification Successful"));
            return true;
         }
      }
   }
   DbgPrintln(("Authentification Failed"));
   return false; 
*/
}

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

void WriteConfig(bool def)
{
   if(def)
   {
      memset(&cfg,0,sizeof(ESP_CONFIG));

      snprintf(cfg.s.user,21,"root");
      snprintf(cfg.s.pwd,21,"esp8266");

      cfg.s.wifi_mode = 0; // 0 ap 1 sta+ap 2 sta
      snprintf(cfg.s.sta_ssid,33,"HomeAP");
      snprintf(cfg.s.sta_pwd,65,"HomeAPpwd");
      cfg.s.sta_dhcp = 1;
      cfg.s.skip_logon = 0;
      cfg.s.sta_ip[0] = 192; cfg.s.sta_ip[1] = 168; cfg.s.sta_ip[2] = 0; cfg.s.sta_ip[3] = 10;
      cfg.s.sta_gw[0] = 192; cfg.s.sta_gw[1] = 168; cfg.s.sta_gw[2] = 0; cfg.s.sta_gw[3] = 1;
      cfg.s.sta_subnet[0] = 255; cfg.s.sta_subnet[1] = 255; cfg.s.sta_subnet[2] = 255; cfg.s.sta_subnet[3] = 0;
      sprintf(cfg.s.ap_ssid,"");
      sprintf(cfg.s.ap_pwd,"");
      cfg.s.ap_hidden = 0; // 1 - hidden
      cfg.s.ap_chan = 6;
      cfg.s.ap_ip[0] = 192; cfg.s.ap_ip[1] = 168; cfg.s.ap_ip[2] = 4; cfg.s.ap_ip[3] = 1;
      cfg.s.ap_gw[0] = 192; cfg.s.ap_gw[1] = 168; cfg.s.ap_gw[2] = 4; cfg.s.ap_gw[3] = 1;
      cfg.s.ap_subnet[0] = 255; cfg.s.ap_subnet[1] = 255; cfg.s.ap_subnet[2] = 255; cfg.s.ap_subnet[3] = 0;
   }    
   cfg.s.crc = crc16(&(cfg.b[2]),sizeof(ESP_CONFIG)-2);
   EEPROM.begin(4096);  
   for(uint16_t i=0; i<sizeof(ESP_CONFIG); i++) { EEPROM.write(i,cfg.b[i]);/*rtc.eeprom_write(i,cfg.b[i]);*/ }
   EEPROM.end();
}

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
   for(uint16_t i=0; i<sizeof(ESP_TPRG); i++) { EEPROM.write(512+i,prg.b[i]);/*rtc.eeprom_write(512+i,prg.b[i]);*/ }
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
