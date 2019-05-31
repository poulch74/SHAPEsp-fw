#include "relay.h"


const int drvA1   = 15;//14; // close pin
const int drvA2   = 14;//12; // open pin
const int drvSTBY = 13; // open pin


class TimerTask : public EspTask
{
public:
   TimerTask() : EspTask() { }
   void Initialize(uint8_t typ)
   {
      skiptmr = false;
      relay = new Relay(drvA1,drvA2,drvSTBY,typ,1); // 1 use autostop R_VALVE
      relay->Initialize();
   }

public:
   Relay *relay;
   bool skiptmr;

public:

   void sendMqttState(int state, int mode)
   {
      std::vector<String> payload;

      if(cfg.mqtt.idx_relay && (state!=0))
      {
         payload.push_back(FmtMqttMessage(cfg.mqtt.idx_relay, relay->GetState(), "Status"));
      }

      if(cfg.mqtt.idx_mbtn && mode)
      {
         payload.push_back(FmtMqttMessage(cfg.mqtt.idx_mbtn, (skiptmr ? 0:1), "Status"));
      }

      GetEvent(EVT_MQTTPUB).doTasks(payload); // force publish
   }

   void doTask(int evt)
   {
      int vstate = 0;
      bool updatemode = false;

      do
      {
         if(evt == EVT_VSTARTUP) { vstate=1; updatemode=true; break;}

         if(evt == EVT_VCLOSE) { skiptmr = true;  updatemode = true; vstate = -1; }
         if(evt == EVT_VOPEN)  { skiptmr = true;  updatemode = true; vstate = 1; }
         if(evt == EVT_VAUTO)  { skiptmr = false; updatemode = true;}

         if((cfg.dev.en_timer) && (!skiptmr)) // if timer enabled - check
         {
            time_t ct = now();

            uint16_t tcur = (uint16_t)((ct-previousMidnight(ct))/60);
            uint8_t shift = (dayOfWeek(ct)-1) ? (dayOfWeek(ct)-2):6;
            uint8_t cdow = 1 << shift; // 0 based day of week 0 monday
            for(int i=0;i<10;i++)
               if((cfg.tmr[i].active)&&(tcur==cfg.tmr[i].on_ts)&&(cdow&cfg.tmr[i].on_dowmask)) { vstate = 1; break;}
            for(int i=0;i<10;i++)
               if((cfg.tmr[i].active)&&(tcur==cfg.tmr[i].off_ts)&&(cdow&cfg.tmr[i].off_dowmask)) { vstate = -1; break;}
         }

         if(vstate!=0) { relay->SetState(((vstate>0) ? 1:0)); }

      } while(0);

      // publish relay status to keep tracking
      sendMqttState(vstate,updatemode);
   }

   void doMqttTask(int evt, std::vector<String> &payload)
   {
      if(evt == EVT_MQTT)
      {
         if(cfg.mqtt.idx_mode) // non zero -> publish
            payload.push_back(FmtMqttMessage(cfg.mqtt.idx_mode,0, (skiptmr ? "Manual":"Auto")));

         if(cfg.mqtt.idx_status)
            payload.push_back(FmtMqttMessage(cfg.mqtt.idx_status, relay->GetState(), (relay->GetState() ? "Open":"Close")));
      }
   }


   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      String cmd = iroot["cmd"];
      String event = iroot["text"];

      if(event == "status") // событие статуса
      {
         root["status_vmode"] = skiptmr ? "Manual":"Auto";
         root["status_vstatus"] = relay->GetState() ? "Open":"Close";
         return;
      }

      if(event== "time")
      {
         if(cmd=="auto") { sysqueue.push(&GetEvent(EVT_VAUTO)); DEBUG_MSG_P(PSTR("SCHEDULE AUTO\n"));}
         if(cmd=="close") { sysqueue.push(&GetEvent(EVT_VCLOSE)); DEBUG_MSG_P(PSTR("SCHEDULE CLOSE\n"));}
         if(cmd=="open") { sysqueue.push(&GetEvent(EVT_VOPEN)); DEBUG_MSG_P(PSTR("SCHEDULE OPEN\n"));}

         if(cmd == "settime")
         {
            DEBUG_MSG_P(PSTR("settime and response"));
            tmElements_t tm;
            tm.Year = CalendarYrToTm(iroot["time_year"].as<int>());
            tm.Month = iroot["time_month"];
            tm.Day = iroot["time_day"];
            tm.Wday = (iroot["time_dow"] == 7) ? 0:iroot["time_dow"].as<uint8_t>()+1;
            tm.Hour = iroot["time_hour"];
            tm.Minute = iroot["time_min"];
            tm.Second = iroot["time_sec"];

            time_t t = makeTime(tm);
            setTime_rtc(t);
            setTimeUptime(t); // set time and correct uptime

            cmd = "defaults";
         }

         if((cmd == "settimer") ||
            (cmd == "savetimer") ||
            (cmd == "resettimer"))
         {
            for(int i=0;i<10;i++)
            {
               String n(i);
               cfg.tmr[i].active = iroot["time_sact"+n];

               cfg.tmr[i].on_dowmask = iroot["time_sdmask"+n];
               cfg.tmr[i].on_hour = iroot["time_shour"+n];
               cfg.tmr[i].on_min = iroot["time_smin"+n];
               cfg.tmr[i].on_ts = cfg.tmr[i].on_hour*60+cfg.tmr[i].on_min;

               cfg.tmr[i].off_dowmask = iroot["time_edmask"+n];
               cfg.tmr[i].off_hour = iroot["time_ehour"+n];
               cfg.tmr[i].off_min = iroot["time_emin"+n];
               cfg.tmr[i].off_ts = cfg.tmr[i].off_hour*60+cfg.tmr[i].off_min;
            }

            ///////////////// set !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            bool reset = false;
            if(cmd == "resettimer") reset = true;
            if(cmd != "settimer") { WriteConfig(false, reset); }

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
               root["time_sact"+n] = (int)cfg.tmr[i].active;
               root["time_sdmask"+n] = cfg.tmr[i].on_dowmask;
               root["time_shour"+n] = cfg.tmr[i].on_hour;
               root["time_smin"+n] = cfg.tmr[i].on_min;
               root["time_edmask"+n] = cfg.tmr[i].off_dowmask;
               root["time_ehour"+n] = cfg.tmr[i].off_hour;
               root["time_emin"+n] = cfg.tmr[i].off_min;
            }
         }
      }
   }
};

TimerTask taskTimer;