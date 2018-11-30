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
"\"skip_logon\":0"
"}";


bool LoadConfig()
{
   ESP_CFG cfg;
   memset(&cfg,0,sizeof(ESP_CFG));
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_CFG); i++) cfg.payload[i] = EEPROM.read(2048+i);
   EEPROM.end();

   DEBUG_MSG("[CONFIG] PARSE parsing data. %s\n",(const char *)cfg.payload);
   //config = &jsonBuffer.parseObject(cfg.s.payload);
   config.parse((const char *)cfg.payload);
   if (!config.root().success())
   { 
      DEBUG_MSG("[CONFIG] PARSE Error parsing data!\n");
      return false;
   }

   for (auto kv : config.root())
   {
      DEBUG_MSG("[CONFIG]  %s : %s\n", kv.key, kv.value.as<String>().c_str());
   }
   // test prints
   return true;
}

void SaveConfig(bool def)
{
   ESP_CFG cfg;
   memset(&cfg,0,sizeof(ESP_CFG));
   if(def)
   {
      String buf(defcfg);
      config.clear();
      config.parse(buf.c_str());
   }
   DEBUG_MSG("[CONFIG] Lenght of config: %d\n", config.root().measureLength());
   config.root().printTo((char *)cfg.payload,sizeof(cfg.payload));
   DEBUG_MSG("[CONFIG] PARSE parsing data. %s\n",(const char *)cfg.payload);
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_CFG); i++) { EEPROM.write(2048+i,cfg.payload[i]); }
   EEPROM.end();
}