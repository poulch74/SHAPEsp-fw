std::vector<Sensor *> sensors;

//OneWire owbus(2);
int8_t ow_pin = -1;
OneWire *owbus;
DS2482 *dsbus;

class TaskSens : public EspTask
{
public:
   TaskSens() : EspTask() {}
   void Initialize()
   {

      if(cfg.dev.en_sensors)
      {

         if(cfg.dev.scan_i2c) // i2c sensors
         {

            if(i2cCheck(0x76)==0)
            {
               DEBUG_MSG1("BME280 found at address 0x76. Adding... \n", dstring55);
               BME280Sensor *bme = new BME280Sensor(0x76);
               sensors.push_back(bme);
            }

            if(i2cCheck(0x77)==0)
            {
               DEBUG_MSG1("BME280 found at address 0x77. Adding... \n", dstring56);
               BME280Sensor *bme = new BME280Sensor(0x77);
               sensors.push_back(bme);
            }

         }

         if(cfg.dev.scan_ds1w) // ds1w sensors on i2c-1w bridge
         {
            if(i2cCheck(0x18)==0)
            {
               DEBUG_MSG1("I2C-1W bridge found at address 0x18. Adding... \n", dstring57);
               dsbus = new DS2482(0);

               uint8_t addr[8];
               while(dsbus->search(addr))
               {
                  DEBUG_MSG1("found 1-Wire ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \n", dstring58,
                           addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7]);
                  if(DS2482::crc8(addr,7) != addr[7]) { DEBUG_MSG1("CRC is not valid! \n", dstring59); }
                  else
                  {
                     DEBUG_MSG1("Add into sensors list! \n", dstring60);
                     DS1820Sensor *dss = new DS1820Sensor(dsbus,addr);
                     sensors.push_back(dss);
                  }
               }
               DEBUG_MSG1("No more addresses on I2C-1W bridge. \n", dstring61);
               dsbus->reset_search();
            }
         }

         if(ow_pin!=(-1)) // soft ds1w sensors on i/o pin
         {
            owbus = new OneWire((uint8_t)ow_pin);
            uint8_t addr[8];
            while(owbus->search(addr))
            {
               DEBUG_MSG1("found 1-Wire ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X \n", dstring58,
                           addr[0],addr[1],addr[2],addr[3],addr[4],addr[5],addr[6],addr[7]);

               if(OneWire::crc8(addr,7) != addr[7]) { DEBUG_MSG1("CRC is not valid! \n", dstring59); }
               else
               {
                  DEBUG_MSG1("Add into sensors list! \n", dstring60);
                  DS1820Sensor *dss = new DS1820Sensor(owbus,addr);
                  sensors.push_back(dss);
               }
            }
            DEBUG_MSG1("No more addresses. \n", dstring61);
            owbus->reset_search();
         }
      }

      sens_count = 0;

      if(!sensors.empty())
      {
         for(int i=0;i<sensors.size();i++)
         {
            sensors[i]->init();
            sens_count+=sensors[i]->getTagCount();
         }
         DEBUG_MSG1("Sensor cnt %d %d\n", dstring62, sensors.size(), sens_count);
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
         int max = sensors.size()>MAX_SENSORS_CNT ? MAX_SENSORS_CNT : sensors.size();
         for(int i=0;i<max;i++)
         {
            if(cfg.mqtt.idx_sens[i])
            {
               String buf = sensors[i]->getMqttPayload(cfg.mqtt.idx_sens[i],0);
               if(buf.length()!=0) payload.push_back(buf);
            }
         }
      }
   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      if(iroot["text"]=="senscnt")
      {
         root["action"] = "senscnt";
         root["sens_cnt"] = sens_count;
         return;
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
               char nsensor[16]; snprintf(nsensor,16,"%d:&nbsp",i);
               String sname = String(nsensor) + sensors[i]->getName();
               root[tsensor] = sname + "::" +sensors[i]->getTag(j);
               root[vsensor] = sensors[i]->getValueAsStr(j);
               k++;
            }
         }
      }
   }

private:
   int sens_count;

};

TaskSens sens_task;
