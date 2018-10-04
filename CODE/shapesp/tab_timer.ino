void handleIndex2(AsyncWebServerRequest *request)
{
   DbgPrintln(("Enter handle timerset"));
   String message = DbgArgMsg(request);
   do
   {
      if(!is_auth(request)) { request->redirect("/login"); break; }

      if(request->hasArg("timeset"))
      {
         do
         {
            StaticJsonBuffer<JSON_OBJECT_SIZE(8) + 90> jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(request->arg("par"));
            if (root.success())
            {
               Serial.println("JSON parsing!");
               if(root["act"].as<int>()) // 8 в 1 устанавливать а если 0  только читать
               {
                  time_t t;
                  tmElements_t tm;

                  tm.Second = root["sec"];
                  tm.Minute = root["minu"];
                  tm.Hour   = root["hour"];

                  tm.Wday = root["dow"].as<int>()+1;
                  if(tm.Wday>7) tm.Wday = 1;


                  tm.Day   =  root["day"];
                  tm.Month = root["month"];
                  tm.Year  = y2kYearToTm(root["year"].as<int>()-2000);

                  t = makeTime(tm);
                  setTime_rtc(t);
                  setTime(t);
               }
            }        
         } while(0);

         time_t t = now();
         tmElements_t tm;
         breakTime(t, tm);

         StaticJsonBuffer<JSON_OBJECT_SIZE(8) + 90> respBuffer;
         JsonObject& resp = respBuffer.createObject();
         resp["rand"] = request->arg("r");
         resp["year"] = tmYearToCalendar(tm.Year);
         resp["month"] = tm.Month;
         resp["day"] = tm.Day;
         resp["dow"] = (tm.Wday-1) ? (tm.Wday-1):7;
         resp["hour"] = tm.Hour;
         resp["minu"] = tm.Minute;
         resp["sec"] = tm.Second;
         String json;
         resp.printTo(json);
         DbgPrintln((json));
         request->send(200, "text/plain", json);
         break;
      }

      if(request->hasArg("act"))
      {
         int act = request->arg("act").toInt(); // 0 читать 1писать 2 соранить 3 сброситьи сохранить
 
         if(act==3) { SaveTmrPrg(true); } // reset и сохраним в eeprom 
         if(act==2) { SaveTmrPrg(false); } // сохраним в eeprom
         if(act==1)
         {
            StaticJsonBuffer<JSON_ARRAY_SIZE(10) + 10*JSON_OBJECT_SIZE(7) + 590> jsonBuffer;
            JsonArray& root = jsonBuffer.parseArray(request->arg("par"));
            if (root.success())
            {
               DbgPrintln(("JSON parsing!"));
               for(int i=0;i<10;i++)
               {
                  prg.ta.p[i].active = root[i]["sact"];
                  prg.ta.p[i].on_dowmask = root[i]["sdmask"];
                  prg.ta.p[i].on_hour = root[i]["shour"];
                  prg.ta.p[i].on_min = root[i]["smin"];
                  prg.ta.p[i].on_ts = prg.ta.p[i].on_hour*60+prg.ta.p[i].on_min;
                  prg.ta.p[i].off_dowmask = root[i]["edmask"];
                  prg.ta.p[i].off_hour = root[i]["ehour"];
                  prg.ta.p[i].off_min = root[i]["emin"];
                  prg.ta.p[i].off_ts = prg.ta.p[i].off_hour*60+prg.ta.p[i].off_min;
               }
            }
         }

         StaticJsonBuffer<JSON_ARRAY_SIZE(10) + 10*JSON_OBJECT_SIZE(7) + 590> respBuffer;
         JsonArray& resp = respBuffer.createArray();
         for(int i=0;i<10;i++)
         {
            JsonObject& nested = resp.createNestedObject();
            nested["sact"] = (int)prg.ta.p[i].active;
            nested["sdmask"] = prg.ta.p[i].on_dowmask;
            nested["shour"] = prg.ta.p[i].on_hour;
            nested["smin"] = prg.ta.p[i].on_min;
            nested["edmask"] = prg.ta.p[i].off_dowmask;
            nested["ehour"] = prg.ta.p[i].off_hour;
            nested["emin"] = prg.ta.p[i].off_min;
         }        

         String json;
         resp.printTo(json);
         DbgPrintln((json));
         request->send(200, "text/plain", json);    
         break;
      }     

      request->send(SPIFFS, "/timeset.htm","text/html");

   } while(0);

   DbgPrintln((message));
}
 