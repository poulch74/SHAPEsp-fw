// noolite msg bytes

#define b_start  0
#define b_mode   1
#define b_ctr    2
#define b_togl   3
#define b_ch     4
#define b_cmd    5
#define b_fmt    6
#define b_d0     7
#define b_d1     8
#define b_d2     9
#define b_d3     10
#define b_id0    11
#define b_id1    12
#define b_id2    13
#define b_id3    14
#define b_crc    15
#define b_stop   16

class Noolite
{
public:
   uint8_t r_msg[17];
   uint8_t s_msg[17];
   HardwareSerial *port;
public:
   Noolite()
   {
      port = &Serial;
      port->begin(9600);
      port->flush();
   }
   void SendMsgInit() { memset(s_msg,0,sizeof(s_msg)); s_msg[b_start] = 171; s_msg[b_stop] = 172;}
   void Send()
   {
      _crc();
      port->write(s_msg,sizeof(s_msg));
      DEBUG_MSG_P(PSTR("Send: "));
      for(auto i=0;i<17;i++) DEBUG_MSG_P(PSTR("%02x"), s_msg[i]);
      DEBUG_MSG_P(PSTR("\n"));
   }
   void Recv()
   {
      port->readBytes(r_msg,17);
      DEBUG_MSG_P(PSTR("Recv: "));
      for(auto i=0;i<17;i++) DEBUG_MSG_P(PSTR("%02x"), r_msg[i]);
      DEBUG_MSG_P(PSTR("\n"));
   }
   void _crc() { s_msg[b_crc] = 0; for(auto i=0;i<15;i++) s_msg[b_crc] += s_msg[i]; }
};



class TaskNoolite : public EspTask
{
public:
   Noolite *noo;
public:
   TaskNoolite() : EspTask() { }
   Initialize()
   {
      noo = new Noolite();
      noo.SendMsgInit();
      noo.s_msg[b_mode] = 4; // service mode
      noo.Send();
      uint32_t start = millis();
      while((millis()-start)<50) { if(Serial.available()>16) { noo.Recv(); } }
   }

   void doTask(int evt)
   {
      if(evt==EVT_NOOSEND) { noo.Send(); }

      //if(!sendqueue.empty()) Serial.write(sendqueue.front());
      uint32_t start = millis();
      while((millis()-start)<50)
      {
         if(Serial.available()>16)
         {
            noo.Recv();
            //if(rx) { push. event.}  //это принятое в очередь
            //if(tx) { break; } // это был ответ
         }
      }
      //sendqueue.pop();

      //!!!!!!!!!!!!!!sysqueue.push(&GetEvent(EVT_NOO));
   }

   void doMqttTask(int evt, std::vector<String> &payload)
   {
   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      String   cmd = iroot["cmd"];
      String event = iroot["text"];
/*
      if(event == "status") // событие статуса
      {
         return;
      }

      if(event== "noolite")
      {
         if(cmd=="send")
         {

            sysqueue.push(&GetEvent(EVT_NOOSEND));
            DEBUG_MSG_P(PSTR("SCHEDULE NOOLITE_SEND\n"));
         }
         //if(cmd=="close") { sysqueue.push(&GetEvent(EVT_VCLOSE)); DEBUG_MSG_P(PSTR("SCHEDULE CLOSE\n"));}
         //if(cmd=="open") { sysqueue.push(&GetEvent(EVT_VOPEN)); DEBUG_MSG_P(PSTR("SCHEDULE OPEN\n"));}

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


         if(cmd == "defaults") // send reply
         {
         }
      }
      */
   }

};

TaskNoolite taskNoolite;
