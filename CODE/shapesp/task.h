extern ESP_MQTT mqttset;

class TestTask1 : public EspTask
{
public:
   TestTask1() : EspTask() {}
   void doTask(int evt)
   {
      if(evt == EVT_60SEC)
      {
         time_t t = getUptime();
         int day = (t/86400);
         int hr =  (t/3600)%24;
         int min = (t%3600)/60;
         snprintf(uptime,32,"%dd:%02dh:%02dm\n",day,hr,min);
         DEBUG_MSG("Uptime: %s",uptime);
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


extern ESP_CONFIG cfg;

class TestTask4 : public EspTask
{
public:
   TestTask4() : EspTask() {}
   void doTask(int evt) {DEBUG_MSG("DoTask4\n");}
   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      String cmd = iroot["cmd"];

      DEBUG_MSG("sendTask4\n");

      if(cmd=="setwifi")
      {
         snprintf(cfg.s.sta_ssid, 33 ,iroot["wifi_ssid"].as<String>().c_str());
         snprintf(cfg.s.sta_pwd,65, iroot["wifi_pwd"].as<String>().c_str());
         cfg.s.sta_dhcp = iroot["wifi_dhcp"];
         IPAddress ip;
         ip.fromString(iroot["wifi_ipa"].as<String>()); for(int i=0;i<4; i++) cfg.s.sta_ip[i] = ip[i];
         ip.fromString(iroot["wifi_gw"].as<String>()); for(int i=0;i<4; i++) cfg.s.sta_gw[i] = ip[i];

         int m = 32-(iroot["wifi_mask"].as<int>())&0x1F;
         uint32_t ma = 0xFFFFFFFF<<m;
         cfg.s.sta_subnet[0] = (ma>>24)&0xFF;
         cfg.s.sta_subnet[1] = (ma>>16)&0xFF;
         cfg.s.sta_subnet[2] = (ma>>8)&0xFF;
         cfg.s.sta_subnet[3] = ma&0xFF;
         String s; s = String(cfg.s.sta_subnet[0])+'.'+String(cfg.s.sta_subnet[1])+'.'+String(cfg.s.sta_subnet[2])+'.'+String(cfg.s.sta_subnet[3]);
         DEBUG_MSG("NetMask: %s \n",s.c_str());


         cfg.s.skip_logon = iroot["wifi_tnet"];
  
         WriteConfig(false);
         DEBUG_MSG("write config\n");

         cmd = "defaults";
      }

      if(cmd=="defaults")
      {
         ReadConfig();
         uint32_t ma = (((uint32_t)cfg.s.sta_subnet[0])<<24) + (((uint32_t)cfg.s.sta_subnet[1])<<16)
                       +(((uint32_t)cfg.s.sta_subnet[2])<<8) + cfg.s.sta_subnet[3];
         int m=0; while(ma!=0) { ma<<=1; m++; };

         root["action"] = "wifi";
         root["wifi_ssid"] = String(cfg.s.sta_ssid);
         root["wifi_pwd"] = String(cfg.s.sta_pwd);
         root["wifi_dhcp"] = cfg.s.sta_dhcp;
         root["wifi_ipa"] = IPAddress(cfg.s.sta_ip[0],cfg.s.sta_ip[1],cfg.s.sta_ip[2],cfg.s.sta_ip[3]).toString();
         root["wifi_gw"] = IPAddress(cfg.s.sta_gw[0],cfg.s.sta_gw[1],cfg.s.sta_gw[2],cfg.s.sta_gw[3]).toString();
         root["wifi_mask"] = m;
         root["wifi_tnet"] = cfg.s.skip_logon;
      }


   }
};

TestTask4 taskSettings;
