/*

template<typename T> String getSetting(const String& key, T defValue)
{
   JsonObject& cfg = config.root();
   if (!cfg.containsKey(key)) {cfg[key] = defValue;}
   return cfg[key];

}

template<typename T> String getSetting(const String& key, unsigned int index, T defValue)
{
    return getSetting(key+String(index), defValue);
}

String getSetting(const String& key)
{
    return getSetting(key, "");
}


template<typename T> void setSetting(const String& key, T value)
{
   JsonObject& cfg = config.root();
   cfg[key] = value;
}

template<typename T> void setSetting(const String& key, unsigned int index, T value)
{
   setSetting(key+String(index), value);
}


bool LoadConfig()
{
   ESP_CFG cfg;
   memset(&cfg,0,sizeof(ESP_CFG));
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_CFG); i++) cfg.payload[i] = EEPROM.read(2048+i);
   EEPROM.end();

   DEBUG_MSG("[CONFIG] PARSE parsing data. %s\n",(const char *)cfg.payload);
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
   
   //if(def)
   //{
   //   String buf(defcfg);
   //   config.clear();
   //   config.parse(buf.c_str());
   //}
   DEBUG_MSG("[CONFIG] Lenght of config: %d\n", config.root().measureLength());
   config.root().printTo((char *)cfg.payload,sizeof(cfg.payload));
   DEBUG_MSG("[CONFIG] PARSE parsing data. %s\n",(const char *)cfg.payload);
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_CFG); i++) { EEPROM.write(2048+i,cfg.payload[i]); }
   EEPROM.end();
}
*/