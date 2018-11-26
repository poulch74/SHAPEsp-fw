class Sensor
{
public:
   Sensor() {};
   virtual int init(){};
   virtual int begin(){};
   virtual int run(){};
   virtual int end(){};
   virtual String getName(){};
   virtual String getTag(int idx){};
   virtual String getValueAsStr(int idx){};
   virtual double getValueAsDbl(int idx){};
   virtual int    getValueAsInt(int idx){};
   virtual String getMqttPayload(int sens, int v) {}; // variants of mqtt payload 


   int    getTagCount(){ return tcnt;};
   bool   ok() {return f_ok;}
   bool   ready() {return f_ready;}
protected:
   int tcnt;
   bool f_ok;
   bool f_ready;
};


#include "OneWire.h"

class DS1820Sensor: public Sensor
{
public:
   uint8_t adr[8];
   int step;
   bool type_s;
   OneWire *ds;
   String name;
   float temp;

   DS1820Sensor(OneWire *bus, uint8_t addr[8]) { ds = bus; memcpy(adr,addr,8); init(); }
   int init()
   { 
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
            ds->reset();
            ds->select(adr);
            ds->write(0x44);
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
         if((ds->reset()==1))
         {
            //DEBUG_MSG("sensor run OK\n");
            ds->select(adr);
            ds->write(0xBE);
            uint8_t data[9];
            for (int i = 0; i < 9; i++) { data[i] = ds->read(); }

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


#include "BME280I2C_BRZO.h"

class BME280Sensor: public Sensor
{
   public:
      BME280Sensor(uint8_t addr) { adr = addr; init(); }
      int init()
      { 
         BME280I2C_BRZO::Settings settings(
                  BME280::OSR_X1,
                  BME280::OSR_X1,
                  BME280::OSR_X1,
                  BME280::Mode_Forced,
                  BME280::StandbyTime_1000ms,
                  BME280::Filter_Off,
                  BME280::SpiEnable_False,
                  adr,
                  400
         );

         bme280 = new BME280I2C_BRZO(settings);
         f_ready=false;
         tcnt = 3;
         f_ok = bme280->begin() ? true:false;
      }
      int begin() { return 0;}
      int end() {return 0;}
      int run()
      {
         //DEBUG_MSG("sensor run\n");
         if(f_ok)
         {
            //DEBUG_MSG("sensor run OK\n");
            BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
            BME280::PresUnit presUnit(BME280::PresUnit_hPa);
            bme280->read(pres, temp, hum, tempUnit, presUnit);
            f_ready = true;
            return 0;
         }
         f_ready = false;
         return 1;
      }
      String getName() { return String("BME280"); }
      String getTag(int idx)
      {
         switch(idx)
         {
            case 0: return String("Pressure");
            case 1: return String("Temperature");
            case 2: return String("Humidity");
         }
      };
      String getValueAsStr(int idx)
      {
         if(f_ready)
         {
            DEBUG_MSG("get value OK\n");
            switch(idx)
            {
               case 0: return String(pres,1);
               case 1: return String(temp,1);
               case 2: return String(hum,1);
            }
         }
         return String("none");
      };

      String getMqttPayload(int snum, int v) // only for domoticz now, add v case for over
      {
         if(f_ready && mqttset.s.idx_sens[snum]) // ready and valid mapping
         {
            char str[65];
            snprintf(str,64,"%s;%s;0;%s;0",String(temp,1).c_str(), String(hum,1).c_str(), String(pres,1).c_str());
            return FmtMqttMessage(mqttset.s.idx_sens[snum],0, str);
         }
         return String("");
      }

      double getValueAsDbl(int idx){ return 0.0;};
      int    getValueAsInt(int idx){ return 0;};


   protected:
      BME280I2C_BRZO *bme280;
      float temp,hum,pres;
      uint8_t adr;
      
};