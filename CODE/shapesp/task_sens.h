std::vector<Sensor *> sensors;

OneWire bus(2);

class TaskSens : public EspTask
{
public:
   TaskSens() : EspTask() {}
   void Initialize()
   {

      if(i2cCheck(0x76)==0)
      {
         DEBUG_MSG("BME280 found at address 0x76. Adding... \n");
         BME280Sensor *bme = new BME280Sensor(0x76);
         sensors.push_back(bme);
      }

      if(i2cCheck(0x77)==0)
      {
         DEBUG_MSG("next BME280 found at address 0x77. Adding... \n");
         BME280Sensor *bme = new BME280Sensor(0x77);
         sensors.push_back(bme);
      }

      uint8_t addr[8];      
      while(bus.search(addr))
      {
         DEBUG_MSG("found 1-Wire ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \n",
                      addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7]);

         if(OneWire::crc8(addr,7) != addr[7]) { DEBUG_MSG("CRC is not valid! \n"); }
         else
         {
            DEBUG_MSG("Add into sensors list! \n");
            DS1820Sensor *dss = new DS1820Sensor(&bus,addr);
            sensors.push_back(dss);
         }
      }
      DEBUG_MSG("No more addresses. \n");
      bus.reset_search();

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
