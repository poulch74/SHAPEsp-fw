#include "relay.h"


const int drvA1   = 14; // close pin 
const int drvA2   = 12; // open pin
const int drvSTBY = 13; // open pin


class TimerTask : public EspTask
{
public:
   TimerTask() : EspTask() { }
   void Initialize()
   {
      skiptmr = false;
      relay = new Relay(drvA1,drvA2,drvSTBY,R_VALVE,1); // 1 use autostop
      //relay->Initialize();
   }

public:
   Relay *relay;
   bool skiptmr;

   void sendMqttDefaults()
   {
      std::vector<String> payload;

      if(cfg.mqtt.idx_relay)
      {
         payload.push_back(FmtMqttMessage(cfg.mqtt.idx_relay, relay->GetState(), "Status"));
      }

      if(cfg.mqtt.idx_mbtn)
      {
         payload.push_back(FmtMqttMessage(cfg.mqtt.idx_mbtn, (skiptmr ? 0:1), "Status"));
      }

      GetEvent(EVT_MQTTPUB).doTasks(payload); // force publish
   }

   void doTask(int evt)
   {
      int vstate = 0;
      bool updatemode = false;
      
      if(evt == EVT_VSTARTUP) { sendMqttDefaults(); return;}
      
      if(evt == EVT_VCLOSE) { skiptmr = true; vstate = -1; updatemode = true;}
      if(evt == EVT_VOPEN) { skiptmr = true; vstate = 1; updatemode = true;}
      if(evt == EVT_VAUTO) { skiptmr =  false; updatemode = true;}

      if(!skiptmr)
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

      std::vector<String> payload;

      if(vstate!=0)   // publish relay status to keep tracking
      { 
         int state = relay->SetState(((vstate>0) ? 1:0));
         if(cfg.mqtt.idx_relay) payload.push_back(FmtMqttMessage(cfg.mqtt.idx_relay, state, "Status"));
      }

      if(updatemode) // publish mode to keep tracking
      {
         if(cfg.mqtt.idx_mbtn) payload.push_back(FmtMqttMessage(cfg.mqtt.idx_mbtn, (skiptmr ? 0:1), "Status"));
      }

      GetEvent(EVT_MQTTPUB).doTasks(payload); // force publish
   }

   void doMqttTask(int evt, std::vector<String> &payload)
   {
      if(evt == EVT_MQTT)
      {
         if(cfg.mqtt.idx_mode) // non zero -> publish
         {
            String buf = FmtMqttMessage(cfg.mqtt.idx_mode,0, (skiptmr ? "Manual":"Auto"));
            payload.push_back(buf);
         }

         if(cfg.mqtt.idx_status)
         {
            String buf = FmtMqttMessage(cfg.mqtt.idx_status, relay->GetState(), (relay->GetState() ? "Open":"Close"));
            payload.push_back(buf);
         }
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
         if(cmd=="auto") { sysqueue.push(&GetEvent(EVT_VAUTO)); DEBUG_MSG("SCHEDULE AUTO\n");}
         if(cmd=="close") { sysqueue.push(&GetEvent(EVT_VCLOSE)); DEBUG_MSG("SCHEDULE CLOSE\n");}
         if(cmd=="open") { sysqueue.push(&GetEvent(EVT_VOPEN)); DEBUG_MSG("SCHEDULE OPEN\n");}

         if(cmd == "settime")
         {
            DEBUG_MSG("settime and response");
            tmElements_t tm;
            tm.Year = CalendarYrToTm(iroot["time_year"].as<int>());
            tm.Month = iroot["time_month"];//.as<uint8_t>();
            tm.Day = iroot["time_day"];//.as<uint8_t>();
            tm.Wday = (iroot["time_dow"] == 7) ? 0:iroot["time_dow"].as<uint8_t>()+1;
            tm.Hour = iroot["time_hour"];//.as<uint8_t>();
            tm.Minute = iroot["time_min"];//.as<uint8_t>();
            tm.Second = iroot["time_sec"];//.as<uint8_t>();

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
            if(cmd != "settimer")
            {
               WriteConfig(false, reset);
            }

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