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
         if(f_ok)
         {
            BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
            BME280::PresUnit presUnit(BME280::PresUnit_inHg);
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
            switch(idx)
            {
               case 0: return String(pres,1);
               case 1: return String(temp,1);
               case 2: return String(hum,1);
            }
         }
         return String("none");
      };

      double getValueAsDbl(int idx){ return 0.0;};
      int    getValueAsInt(int idx){ return 0;};


   protected:
      BME280I2C_BRZO bme280;
      float temp,hum,pres;
      
};