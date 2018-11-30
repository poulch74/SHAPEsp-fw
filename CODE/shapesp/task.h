extern ESP_MQTT mqttset;

class TestTask1 : public EspTask
{
public:
   TestTask1() : EspTask() { snprintf(uptime,32,"00d:00h:00m"); }
   void doTask(int evt)
   {
      if(evt == EVT_60SEC)
      {
         time_t t = getUptime();
         int day = (t/86400);
         int hr =  (t/3600)%24;
         int min = (t%3600)/60;
         snprintf(uptime,32,"%dd:%02dh:%02dm",day,hr,min);
         DEBUG_MSG("Uptime: %s \n",uptime);
         return;
      }

      uint16_t adc = analogRead(A0);
      vcc = adc*15.63/1024.0; //1000 15.98
      heap = ESP.getFreeHeap();
      rssi = WiFi.RSSI();
      int vcc10 = (int)(vcc*10.0);
      battery = 10 + (vcc10-minbat)*80/(maxbat-minbat);
      if(battery>100) battery = 100;
   }

   void doMqttTask(int evt, std::vector<String> &payload)
   {
      if(evt == EVT_MQTT)
      {
         if(mqttset.s.idx_vcc)
         {
            String buf = FmtMqttMessage(mqttset.s.idx_vcc, 0, String(vcc,1).c_str());
            payload.push_back(buf);
         }
      }
   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      //DbgPrintln(("sendTask1"));
      root["status_wifimode"] = String((wifimode ? "SoftAP":"Station"));
      if(wifimode)
      {
            root["status_wifiip"] = WiFi.softAPIP().toString();
            root["status_wifissid"] = softAPname;
         }
         else
         {
            root["status_wifiip"] = WiFi.localIP().toString();
            root["status_wifissid"] = WiFi.SSID();
         }
      time_t t = now();
      root["status_date"] = strDate(t);
      root["status_time"] = strTime(t);
      root["status_uptime"] = String(uptime);
      root["status_voltage"] = String(vcc,2);
      root["status_heap"] = heap;
      root["status_wifirssi"] = rssi;
   }

private:
   double vcc;
   uint32_t heap;
   char uptime[32];
};

TestTask1 task1;


extern JsonBundle config;

class TestTask4 : public EspTask
{
public:
   TestTask4() : EspTask() {}
   void doTask(int evt) {DEBUG_MSG("DoTask4\n");}
   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      JsonObject& cfg = config.root();

      String cmd = iroot["cmd"];

      DEBUG_MSG("sendTask4\n");

      if(cmd=="setwifi")
      {
         char buf[65];
         snprintf(buf, 33 ,iroot["wifi_ssid"].as<const char*>());
         cfg["sta_ssid"] = String(buf);
         snprintf(buf,65, iroot["wifi_pwd"].as<const char*>());
         cfg["sta_pwd"] = String(buf);
         cfg["sta_dhcp"] = iroot["wifi_dhcp"];
         cfg["sta_ip"] = iroot["wifi_ipa"];
         cfg["sta_gw"] = iroot["wifi_gw"];

         int m = 32-(iroot["wifi_mask"].as<int>())&0x1F;
         uint32_t ma = 0xFFFFFFFF<<m;
         uint8_t subnet[4];
         subnet[0] = (ma>>24)&0xFF; subnet[1] = (ma>>16)&0xFF; subnet[2] = (ma>>8)&0xFF; subnet[3] = ma&0xFF;
         String s; s = String(subnet[0])+'.'+String(subnet[1])+'.'+String(subnet[2])+'.'+String(subnet[3]);
         cfg["sta_subnet"] = s;
         DEBUG_MSG("NetMask: %s \n",s.c_str());
         DEBUG_MSG("NetMask: %s \n",cfg["sta_subnet"].as<const char*>());
         cfg["skip_logon"] = iroot["wifi_tnet"].as<String>();
  
         SaveConfig(false);
         DEBUG_MSG("write config\n");

         cmd = "defaults";
      }

      if(cmd=="defaults")
      {
         //ReadConfig();
         IPAddress l_sn; l_sn.fromString(cfg["sta_subnet"].as<const char*>());
         uint32_t ma = (((uint32_t)(l_sn[0]))<<24) + (((uint32_t)(l_sn[1]))<<16)
                       +(((uint32_t)(l_sn[2]))<<8) + l_sn[3];
         int m=0; while(ma!=0) { ma<<=1; m++; };

         root["action"] = "wifi";
         root["wifi_ssid"] = cfg["sta_ssid"];
         root["wifi_pwd"] = cfg["sta_pwd"];
         root["wifi_dhcp"] = cfg["sta_dhcp"];
         root["wifi_ipa"] = cfg["sta_ip"];
         root["wifi_gw"] = cfg["sta_gw"];
         root["wifi_mask"] = m;
         root["wifi_tnet"] = cfg["skip_logon"];
      }


   }
};

TestTask4 taskSettings;
