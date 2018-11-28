const char defcfg[] = 
"{"
"\"user\":\"root\","
"\"pwd\":\"esp8266\""
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
      config = &jsonBuffer.parseObject(defcfg);
      DEBUG_MSG("[CONFIG] Error parsing data. Load defaults\n");
   }
   else
   {
      config = &jsonBuffer.parseObject(cfg.s.payload);
      if (!config->success())
      { 
         DEBUG_MSG("[CONFIG] Error parsing data. Load defaults\n");
         jsonBuffer.clear();
         config = &jsonBuffer.parseObject(defcfg);
      }
   }

   // test prints
   DEBUG_MSG("[CONFIG] User: %s\n", (*config)["user"].as<String>().c_str());
   DEBUG_MSG("[CONFIG] Pwd: %s\n", (*config)["pwd"].as<String>().c_str());
}