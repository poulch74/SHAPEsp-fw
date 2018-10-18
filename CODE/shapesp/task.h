class EspTask
{
public:
   EspTask() { }
   virtual void doTask(int evt) {}
   //virtual void doSend(int evt, JsonObject &iroot, JsonObject &root) {}
   //virtual void doRecv(int evt, JsonObject &iroot, JsonObject &root) {}
   virtual void doWStask(int evt, JsonObject &iroot, JsonObject &root) {}
};

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
      DbgPrintln(("sendTask1"));
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
      root["status_voltage"] = vcc;
      root["status_heap"] = heap;
      root["status_temp"] = "0";
      root["status_hum"] = "0";
      root["status_pres"] = "0";
      root["status_wifirssi"] = rssi;
   }

   //void doRecv(int evt, JsonObject &iroot, JsonObject &root) {}

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
   void doTask(int evt) {DbgPrintln(("DoTask2"));}
   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      DbgPrintln(("sendTask2"));
      root["status_vmode"] = "Automatic";
      root["status_vstatus"] = "Close";
   }

   //void doRecv(int evt, JsonObject &iroot, JsonObject &root) {}
};

TestTask2 task2;


extern ESP_TPRG prg;

class TestTask3 : public EspTask
{
public:
   TestTask3() : EspTask() {}
   void doTask(int evt) {DbgPrintln(("DoTask3"));}
   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      DbgPrintln(("sendTask3"));
      if(iroot["cmd"].as<String>() == "defaults")
      {
         time_t t = now();
         tmElements_t tm;
         breakTime(t,tm);

         root["action"] = "time";  
         root["time_year"] = tmYearToCalendar(tm.Year);
         root["time_month"] = tm.Month;
         root["time_day"] = tm.Day;
         root["time_dow"] = (tm.Wday-1) ? (tm.Wday-1):7;
         root["time_hour"] = tm.Hour;
         root["time_min"] = tm.Minute;
         root["time_sec"] = tm.Second;

         for(int i=0;i<10;i++)
         {
            String n(i);
            root["time_sact"+n] = (int)prg.ta.p[i].active;
            root["time_sdmask"+n] = prg.ta.p[i].on_dowmask;
            root["time_shour"+n] = prg.ta.p[i].on_hour;
            root["time_smin"+n] = prg.ta.p[i].on_min;
            root["time_edmask"+n] = prg.ta.p[i].off_dowmask;
            root["time_ehour"+n] = prg.ta.p[i].off_hour;
            root["time_emin"+n] = prg.ta.p[i].off_min;
         }
      }
      if(iroot["cmd"].as<String>() == "settime")
      {
         DbgPrintln(("settime and response"));
         tmElements_t tm;
         tm.Year = CalendarYrToTm(iroot["time_year"].as<int>());
         tm.Month = iroot["time_month"].as<uint8_t>();
         tm.Day = iroot["time_day"].as<uint8_t>();
         tm.Wday = (iroot["time_dow"] == 7) ? 0:iroot["time_dow"].as<uint8_t>()+1;
         tm.Hour = iroot["time_hour"].as<uint8_t>();
         tm.Minute = iroot["time_min"].as<uint8_t>();
         tm.Second = iroot["time_sec"].as<uint8_t>();


         time_t t = makeTime(tm);
         setTime_rtc(t);
         setTime(t);

         root["action"] = "time";  
         root["time_year"] = tmYearToCalendar(tm.Year);
         root["time_month"] = tm.Month;
         root["time_day"] = tm.Day;
         root["time_dow"] = (tm.Wday-1) ? (tm.Wday-1):7;
         root["time_hour"] = tm.Hour;
         root["time_min"] = tm.Minute;
         root["time_sec"] = tm.Second;
      }

      if(iroot["cmd"].as<String>() == "settimer")
      {
         for(int i=0;i<10;i++)
         {
            String n(i);
            prg.ta.p[i].active = iroot["time_sact"+n];
            prg.ta.p[i].on_dowmask = iroot["time_sdmask"+n];
            prg.ta.p[i].on_hour = iroot["time_shour"+n];
            prg.ta.p[i].on_min = iroot["time_smin"+n];
            prg.ta.p[i].on_ts = prg.ta.p[i].on_hour*60+prg.ta.p[i].on_min;
            prg.ta.p[i].off_dowmask = iroot["time_edmask"+n];
            prg.ta.p[i].off_hour = iroot["time_ehour"+n];
            prg.ta.p[i].off_min = iroot["time_emin"+n];
            prg.ta.p[i].off_ts = prg.ta.p[i].off_hour*60+prg.ta.p[i].off_min;
         }

         root["action"] = "time";  
         for(int i=0;i<10;i++)
         {
            String n(i);
            root["time_sact"+n] = (int)prg.ta.p[i].active;
            root["time_sdmask"+n] = prg.ta.p[i].on_dowmask;
            root["time_shour"+n] = prg.ta.p[i].on_hour;
            root["time_smin"+n] = prg.ta.p[i].on_min;
            root["time_edmask"+n] = prg.ta.p[i].off_dowmask;
            root["time_ehour"+n] = prg.ta.p[i].off_hour;
            root["time_emin"+n] = prg.ta.p[i].off_min;
         }
      }               

   }
};

TestTask3 task3;