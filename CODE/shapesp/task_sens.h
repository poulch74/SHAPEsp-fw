std::vector<Sensor *> sensors;

class TaskSens : public EspTask
{
public:
   TaskSens() : EspTask() {}
   void Initialize()
   {
      BME280Sensor *bme = new BME280Sensor();
      sensors.push_back(bme);

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
               char nsensor[16]; snprintf(nsensor,16,"Sensor%d: ",k);
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
