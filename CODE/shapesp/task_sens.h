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
         int cnt = sensors.size();
         for(int i=0;i<cnt;i++) 
         { 
            sensors[i]->init();
            sens_count+=sensors[i]->getTagCount();
         }
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
         DEBUG_MSG("sensor reply \n");
         int k=0;
         for(int i=0;i<sensors.size();i++)
         {
            int cnt = sensors[i]->getTagCount();
            for(int j=0;j<cnt;j++)
            {
               String jname = String("sensor_")+String(k++);
               String sname = sensors[i]->getName();
               String vname = sensors[i]->getTag(j);
               String value = sensors[i]->getValueAsStr(j);
               root[jname] = sname+ "::" + vname + " ->"+ value;
            }
         }
      }
   }
private:
   int sens_count;

};

TaskSens sens_task;
