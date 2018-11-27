#include "OneWire.h"
#include "DS2482.h"

class DS1820Sensor: public Sensor
{
public:
   uint8_t adr[8];
   int step;
   bool type_s;
   OneWire *owds;
   DS2482  *dsds;
   String name;
   float temp;

   bool reset()
   {
      if(dsds!=nullptr) return dsds->reset();
      if(owds!=nullptr) return owds->reset();
   }

   void select(uint8_t addr[8])
   {
      if(dsds!=nullptr) { dsds->select(addr); return; }
      if(owds!=nullptr) { owds->select(addr); return; }
   }

   void write(uint8_t b, uint8_t power = 0)
   {
      if(dsds!=nullptr) { dsds->write(b, power); return; }
      if(owds!=nullptr) { owds->write(b, power); return; }
   }

   uint8_t read()
   {
      if(dsds!=nullptr) return dsds->read();
      if(owds!=nullptr) return owds->read();
   }

   DS1820Sensor(DS2482 *bus, uint8_t addr[8]) { dsds = bus; owds = nullptr; init(addr); }
   DS1820Sensor(OneWire *bus, uint8_t addr[8]) { owds = bus; dsds = nullptr; init(addr); }

   int init(uint8_t addr[8])
   { 
      memcpy(adr,addr,8);
      f_ready=false;
      tcnt = 1;
      step = 2;
      f_ok = true;
      type_s = false;
      name = "Unknown";
      switch(adr[0])
      {
         case 0x10: type_s = true;  name = "DS18S20"; break;
         case 0x28: type_s = false; name = "DS18B20"; break;
         case 0x22: type_s = false; name = "DS1822";  break;
         default: f_ok= false;
      } 
   }

   int begin()
   {
      if(f_ok)
      {
         if(step==2)
         {
            reset();
            select(adr);
            write(0x44);
         }
         return 0;
      }         
      return 1;
   }

   int end() {return 0;}

   int run()
   {
      if(f_ok)
      {
         if(step==2) {step--; return 0;}
         //DEBUG_MSG("sensor run\n");
         if((reset()==1))
         {
            //DEBUG_MSG("sensor run OK\n");
            select(adr);
            write(0xBE);
            uint8_t data[9];
            for (int i = 0; i < 9; i++) { data[i] = read(); }

            //DEBUG_MSG("get value OK\n");
            int16_t raw = (data[1] << 8) | data[0]; 
            if (type_s)
            {
               raw = raw << 3; // 9 bit resolution default
               if (data[7] == 0x10)
               {
                  raw = (raw & 0xFFF0) + 12 - data[6]; // "count remain" gives full 12 bit resolution
               }
            } 
            else 
            {
               byte cfg = (data[4] & 0x60); // at lower res, the low bits are undefined, so let's zero them
               if (cfg == 0x00) raw = raw & ~7;      // 9 bit resolution, 93.75 ms
               else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
               else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
               //// default is 12 bit resolution, 750 ms conversion time
            }

            temp = (float)raw/16.0;

            f_ready = true;
            step=2;
            return 0;
         }
      }
      f_ready = false;
      return 1;
   }

   String getName() { return name; }

   String getTag(int idx)
   {
      return String("Temperature");
   };

   String getValueAsStr(int idx)
   {
      if(f_ready)
      {
         return String(temp,1);
      }
      return String("none");
   };

   String getMqttPayload(int snum, int v) // only for domoticz now, add v case for over
   {
      if(f_ready && mqttset.s.idx_sens[snum]) // ready and valid mapping
      {
         char str[65];
         snprintf(str,64,"%s",String(temp,1).c_str());
         return FmtMqttMessage(mqttset.s.idx_sens[snum],0, str);
      }
      return String("");
   }

};
