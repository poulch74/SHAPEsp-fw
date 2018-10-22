class TestTask1 : public EspTask
{
public:
   TestTask1() : EspTask() {}
   void doTask(int evt)
   {
      //DbgPrintln(("DoTask1"));
      uint16_t adc = analogRead(A0);
      vcc = adc*15.63/1024.0; //1000 15.98
      heap = ESP.getFreeHeap();
      rssi = WiFi.RSSI();
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

      root["status_dt"] = strDateTime(now());
      root["status_voltage"] = String(vcc,3);
      root["status_heap"] = heap;
      root["status_temp"] = "0";
      root["status_hum"] = "0";
      root["status_pres"] = "0";
      root["status_wifirssi"] = rssi;
   }

private:
   double vcc;
   uint32_t heap;
   int32_t rssi;
};

TestTask1 task1;


class TestTask2 : public EspTask
{
public:
   TestTask2() : EspTask() {}
   void doTask(int evt)
   {
   //   DbgPrintln(("DoTask2"));
   }
   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      //DbgPrintln(("sendTask2"));
      //root["status_vmode"] = "Automatic";
      //root["status_vstatus"] = "Close";
   }
};

TestTask2 task2;




extern ESP_CONFIG cfg;

class TestTask4 : public EspTask
{
public:
   TestTask4() : EspTask() {}
   void doTask(int evt) {DbgPrintln(("DoTask4"));}
   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      String cmd = iroot["cmd"];

      DbgPrintln(("sendTask4"));

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
         DbgPrintln(("NetMask: "));
         DbgPrintln((s));

         cfg.s.skip_logon = iroot["wifi_tnet"];
  
         WriteConfig(false);
         DbgPrintln(("write config"));

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
