#include "relay.h"

extern ESP_TPRG prg;

const int drvA1   = 14; // close pin 
const int drvA2   = 12; // open pin
const int drvSTBY = 13; // open pin

//Relay relay(drvA1,drvA1,drvSTBY,R_VALVE,1); // 1 use autostop


class TimerTask : public EspTask
{
public:
   TimerTask() : EspTask() { }
   void Initialize()
   {
      valvemode = true; // manual auto;      
      skiptmr = false;

      relay = new Relay(drvA1,drvA1,drvSTBY,R_VALVE,1); // 1 use autostop
      relay->Initialize();
   }

public:
   Relay *relay;
   bool valvemode; // manual auto;
   bool skiptmr;

   void doTask(int evt)
   {
      int vstate = 0;

      DbgPrintln(("DoTask3"));
      
      if(evt == EVT_VCLOSE) { skiptmr = true; vstate = -1; }
      if(evt == EVT_VOPEN) { skiptmr = true; vstate =  1; }
      if(evt == EVT_VAUTO)  { skiptmr =  false; }

      if(!skiptmr)
      {
         time_t ct = now();
      
         uint16_t tcur = (uint16_t)((ct-previousMidnight(ct))/60);
         uint8_t shift = (dayOfWeek(ct)-1) ? (dayOfWeek(ct)-2):6;
         uint8_t cdow = 1 << shift; // 0 based day of week 0 monday
         for(int i=0;i<10;i++)
            if((prg.ta.p[i].active)&&(tcur==prg.ta.p[i].on_ts)&&(cdow&prg.ta.p[i].on_dowmask)) { vstate = 1; break;}
         for(int i=0;i<10;i++)  
            if((prg.ta.p[i].active)&&(tcur==prg.ta.p[i].off_ts)&&(cdow&prg.ta.p[i].off_dowmask)) { vstate = -1; break;}
      }

      if(vstate!=0)
      {  
         int state = 0;
         if(vstate>0) state = 1;
         relay->SetState(state);
      }
   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      DbgPrintln(("sendTask3"));

      String cmd = iroot["cmd"];
      String event = iroot["text"];

      if(event == "status") // событие статуса
      {
         root["status_vmode"] = "Automatic--";
         root["status_vstatus"] = "Close---";
         return;
      }

      if(event== "time")
      {
         if(cmd == "settime")
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

            cmd = "defaults";
         }

         if((cmd == "settimer") ||
            (cmd == "savetimer") ||
            (cmd == "resettimer"))
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

            bool reset = false;
            if(cmd == "resettimer") reset = true;
            if(cmd != "settimer") SaveTmrPrg(reset);

            cmd = "defaults";
         }

         if(cmd == "defaults") // send reply
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
      }
   }
};

TimerTask taskTimer;