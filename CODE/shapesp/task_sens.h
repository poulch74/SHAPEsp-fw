std::vector<Sensor *> sensors;

OneWire bus(2);

class TaskSens : public EspTask
{
public:
   TaskSens() : EspTask() {}
   void Initialize()
   {
      BME280Sensor *bme = new BME280Sensor();
      sensors.push_back(bme);

/*
      uint8_t addr[8];
      if(!bus.search(addr))
      {
         Serial.println("No more addresses.");
         Serial.println();
         //bus.reset_search();
      }
  
      Serial.print("ROM =");
      for( int i = 0; i < 8; i++)
      {
         Serial.write(' ');
         Serial.print(addr[i], HEX);
      }

      if(OneWire::crc8(addr,7) != addr[7]) { Serial.println("CRC is not valid!"); }
      else
      {
         DS1820Sensor *dss = new DS1820Sensor(&bus,addr);
         sensors.push_back(dss);
      }
  */ 
      sens_count = 0;

      if(!sensors.empty())
      {
         for(int i=0;i<sensors.size();i++) 
         { 
            sensors[i]->init();
            sens_count+=sensors[i]->getTagCount();
         }
         DEBUG_MSG("sensor cnt %d %d\n", sensors.size(), sens_count);
      }
   }

   void doTask(int evt)
   {
      if(!sensors.empty())
      {
         int cnt = sensors.size();
         for(int i=0;i<cnt;i++) { sensors[i]->begin(); }
         for(int i=0;i<cnt;i++) { sensors[i]->run(); }
         for(int i=0;i<cnt;i++) { sensors[i]->end(); }
      }
   }

   void doMqttTask(int evt, std::vector<String> &payload)
   {
      if(evt == EVT_MQTT)
      {
         for(int i=0;i<sensors.size();i++)
         {
            String buf = sensors[i]->getMqttPayload(i,0);
            if(buf.length()!=0) payload.push_back(buf);
         }
      }
   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      if(iroot["text"].as<String>()=="senscnt")
      {
         root["action"] = "senscnt";
         root["sens_cnt"] = sens_count;
      }         

      if(!sensors.empty())
      {
         int k=0;
         for(int i=0;i<sensors.size();i++)
         {
            for(int j=0;j<sensors[i]->getTagCount();j++)
            {
               char tsensor[16]; snprintf(tsensor,16,"tsensor_%d",k);
               char vsensor[16]; snprintf(vsensor,16,"vsensor_%d",k);
               char nsensor[16]; snprintf(nsensor,16,"Sensor%d: ",i);
               String sname = String(nsensor) + sensors[i]->getName();
               root[String(tsensor)] = sname + "::" +sensors[i]->getTag(j);
               root[String(vsensor)] = sensors[i]->getValueAsStr(j);
               k++;
            }
         }
      }
   }
private:
   int sens_count;

};

TaskSens sens_task;
