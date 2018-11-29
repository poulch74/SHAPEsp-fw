const char defcfg[] PROGMEM= 
"{"
"\"user\":\"root\","
"\"pwd\":\"esp8266\","
"\"wifi_mode\":0,"
"\"sta_ssid\":\"CH-Home\","
"\"sta_pwd\":\"chps74qwerty\","
"\"sta_dhcp\":1,"
"\"sta_ip\":\"192.168.12.11\","
"\"sta_gw\":\"192.168.12.1\","
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


void LoadConfig()
{
   ESP_CFG cfg;
   memset(&cfg,0,sizeof(ESP_CFG));
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_CFG); i++) cfg.b[i] = EEPROM.read(2048+i);
   EEPROM.end();
   uint16_t c_crc = crc16(&(cfg.b[2]),sizeof(ESP_CFG)-2);
   if(cfg.s.crc!=c_crc)
   {
      String buf(defcfg);
      config.parse(buf.c_str());
      //config = &jsonBuffer.parseObject(buf);
      DEBUG_MSG("[CONFIG] CRC Error parsing data. Load defaults\n");
   }
   else
   {
      DEBUG_MSG("[CONFIG] 1PARSE Error parsing data. %s\n",cfg.s.payload);
      //config = &jsonBuffer.parseObject(cfg.s.payload);
      config.parse((const char *)cfg.s.payload);
      if (!config.root().success())
      { 
         DEBUG_MSG("[CONFIG] PARSE Error parsing data. Load defaults\n");
         DEBUG_MSG("[CONFIG] PARSE Error parsing data. %s\n",cfg.s.payload);
         config.clear();
         String buf(defcfg);
         config.parse(buf.c_str());
      }
   }

   for (auto kv : config.root())
   {
      DEBUG_MSG("[CONFIG]  %s : %s\n", kv.key, kv.value.as<String>().c_str());
   }
   // test prints
   //DEBUG_MSG("[CONFIG] User: %s\n", (*config)["user"].as<String>().c_str());
   //DEBUG_MSG("[CONFIG] Pwd: %s\n", (*config)["pwd"].as<String>().c_str());
}

void SaveConfig()
{
   ESP_CFG cfg;
   memset(&cfg,0,sizeof(ESP_CFG));
   DEBUG_MSG("[CONFIG] Lenght of config: %d\n", config.root().measureLength());
   config.root().printTo(cfg.s.payload);
   DEBUG_MSG("[CONFIG] PARSE parsing data. %s\n",cfg.s.payload);
   cfg.s.crc = crc16(&(cfg.b[2]),sizeof(ESP_CFG)-2);
   EEPROM.begin(4096);    
   for(uint16_t i=0; i<sizeof(ESP_CFG); i++) { EEPROM.write(2048+i,cfg.b[i]); }
   EEPROM.end();   
}