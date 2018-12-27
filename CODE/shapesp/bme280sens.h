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
            //DEBUG_MSG("get value OK\n");
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
         if(f_ready) // ready and valid mapping
         {
            char str[65];
            snprintf(str,64,"%s;%s;0;%s;0",String(temp,1).c_str(), String(hum,1).c_str(), String(pres,1).c_str());
            return FmtMqttMessage(snum,0, str);
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