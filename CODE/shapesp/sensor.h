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
   virtual String getMqttPayload(int v) {}; // variants of mqtt payload 


   int    getTagCount(){ return tcnt;};
   bool   ok() {return f_ok;}
   bool   ready() {return f_ready;}
protected:
   int tcnt;
   bool f_ok;
   bool f_ready;
};

#include "BME280I2C_BRZO.h"

class BME280Sensor: public Sensor
{
   public:
      BME280Sensor() { init(); }
      int init() { f_ready=false; tcnt = 3; f_ok = bme280.begin() ? true:false;}
      int begin() { return 0;}
      int end() {return 0;};
      int run()
      {
         DEBUG_MSG("sensor run\n");
         if(f_ok)
         {
            DEBUG_MSG("sensor run OK\n");
            BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
            BME280::PresUnit presUnit(BME280::PresUnit_hPa);
            bme280.read(pres, temp, hum, tempUnit, presUnit);
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

      String getMqttPayload(int v) // only for domoticz now, add v case for over
      {
         if(f_ready)
         {
            char buf[128];
            mqttset.s.idx_sens[0] = 4;
            String str = String(temp,1) +";"+ String(hum,1) +";0;"+ String(pres,1)+";0";
            snprintf(buf, sizeof(buf), 
                     "{\"command\":\"udevice\",\"idx\":%u,\"nvalue\":%s,\"svalue\":\"%s\"}", 
                     mqttset.s.idx_sens[0], "0", str.c_str()
                    );
            Serial.println(buf);
            return String(buf);
         }
         return String("");
      }

      double getValueAsDbl(int idx){ return 0.0;};
      int    getValueAsInt(int idx){ return 0;};


   protected:
      BME280I2C_BRZO bme280;
      float temp,hum,pres;
      
};