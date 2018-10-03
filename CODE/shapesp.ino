#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266SSDP.h>
#include <EEPROM.h>
#include <brzo_i2c.h>
#include <BME280I2C_BRZO.h>
#include <uRTCLib.h>
#include <Ticker.h>
#include <ArduinoJson.h>

#include <Hash.h>
#include <functional>

#include <FS.h>

#include <ESP8266HTTPUpdateServer.h>

#include <limits.h>
#include <math.h>

#include "tmrlib.h"

const int VALVE = 1; // 1 valve 0 relay

#define PDEBUG

#ifdef PDEBUG
   #define DbgPrintln(s) Serial.println s
   #define DbgPrint(s) Serial.print s
#else
   #define DbgPrint(s)
   #define DbgPrintln(s)
#endif

const char* dow[] = {"Mon","Tue","Wen","Thu","Fri","Sat","Sun"};

const char* ssid = "DIR-300";
//const char* ssid = "CH1-Home";
const char* password = "chps74qwerty";

int session_id =0;

#pragma pack(1)

typedef struct _ESP_TPRG_S
{
   uint8_t on_dowmask;
   uint8_t on_hour;
   uint8_t on_min;
   uint16_t on_ts;

   uint8_t off_dowmask;
   uint8_t off_hour;
   uint8_t off_min;
   uint16_t  off_ts;

   bool active;
} ESP_TPRG_S;

typedef struct _ESP_TPRG_A
{
   uint16_t crc;
   ESP_TPRG_S p[10]; 
} ESP_TPRG_A;

typedef union __ESP_TPRG_U
{
   ESP_TPRG_A ta;
   uint8_t    b[256];
} ESP_TPRG;

typedef struct _ESP_CONFIG_S
{
   uint16_t crc;
   char     user[21];
   char     pwd[21];
   uint8_t  wifi_mode;
   char     sta_ssid[33];
   char     sta_pwd[65];
   uint8_t  sta_dhcp;
   uint8_t  sta_ip[4];
   uint8_t  sta_gw[4];
   uint8_t  sta_subnet[4];
   char     ap_ssid[33];
   char     ap_pwd[65];
   uint8_t  ap_hidden;
   uint8_t  ap_chan;
   uint8_t  ap_ip[4];
   uint8_t  ap_gw[4];
   uint8_t  ap_subnet[4];
   uint8_t  skip_logon;
} ESP_CONFIG_S;

typedef union __ESP_CONFIG_U
{
   ESP_CONFIG_S s;
   uint8_t      b[512];
} ESP_CONFIG;

#pragma pack()

const int led = 2; // led pin

const int drvA1   = 14; // close pin 
const int drvA2   = 12; // open pin
const int drvSTBY = 13; // open pin

uRTCLib_brzo rtc;

bool tflag;
bool voffflag;
Ticker timer;
void alarm()
{ 
   tflag = true;
   if(voffflag)
   { 
      //DbgPrintln(("alloff"));
      //digitalWrite(led,   HIGH);
      digitalWrite(drvA1, LOW);
      digitalWrite(drvA2, LOW);
   }
}

Ticker timerSTBY; // timer 300ms pulse to start motor
void STBYoff() { digitalWrite(drvSTBY, LOW); }

bool vaction;
bool skiptmr; // пропускать таймеры если открыл вручную, не авто режим
int mflag;
int vstate;
int curstate;

int wifimode;
String softAPname;

char current_time[96]; // loop print time in this array

float volt;

BME280I2C_BRZO bme280;
bool bmepresent;
float temp,hum,pres;

IPAddress apIP(192, 168, 4, 1);

AsyncWebServer server(80);
//ESP8266HTTPUpdateServer httpUpdater;

ESP_CONFIG cfg;
ESP_TPRG prg;

void setup()
{
   pinMode(drvA1, OUTPUT);   digitalWrite(drvA1, LOW);
   pinMode(drvA2, OUTPUT);   digitalWrite(drvA2, LOW);
   pinMode(drvSTBY, OUTPUT); digitalWrite(drvSTBY, LOW);
   pinMode(led, INPUT);//     digitalWrite(led, HIGH);

   brzo_i2c_setup(4,5,200);   

   wifimode = 0; // station
   bmepresent=0;
   temp=0;
   hum=0;
   pres=0;
   volt=0;

   Serial.begin(115200);

   rtc.refresh();
   randomSeed(rtc.second());

   SPIFFS.begin();

   bmepresent = bme280.begin();

   if(!ReadConfig())
   { 
      DbgPrintln(("Failed read config!!! Trying write default..."));
      WriteConfig(true);
      DbgPrintln(("Done."));
   }
  
   DbgPrint(("User: ")); DbgPrintln((cfg.s.user));
   DbgPrint(("Pwd: ")); DbgPrintln((cfg.s.pwd));

   WiFi.mode(WIFI_STA);
   WiFi.hostname("test_host");

   WiFi.begin(cfg.s.sta_ssid, cfg.s.sta_pwd);

   if(cfg.s.sta_dhcp==0)
   {
      IPAddress l_ip(cfg.s.sta_ip[0],cfg.s.sta_ip[1],cfg.s.sta_ip[2],cfg.s.sta_ip[3]);
      IPAddress l_gw(cfg.s.sta_gw[0],cfg.s.sta_gw[1],cfg.s.sta_gw[2],cfg.s.sta_gw[3]);
      IPAddress l_sn(cfg.s.sta_subnet[0],cfg.s.sta_subnet[1],cfg.s.sta_subnet[2],cfg.s.sta_subnet[3]);
      WiFi.config(l_ip, l_gw, l_sn);
   }
  
   uint8_t to = 50;
   while(WiFi.status() != WL_CONNECTED )
   {
      Serial.print("."); delay(500); to--;
      if(to==0) break;
   };

   Serial.println(to);
   Serial.println("");

   if(to)
   {
      WiFi.setSleepMode((WiFiSleepType_t)0);
      WiFi.setAutoReconnect(true);
      wifimode = 0; // station
      Serial.print("Connected to "); Serial.println(cfg.s.sta_ssid);
      Serial.print("IP address: "); Serial.println(WiFi.localIP());
   }
   else
   { 
      wifimode = 1; // softap
      WiFi.mode(WIFI_AP);
      String mac = WiFi.softAPmacAddress();
      softAPname = "SHAPEsp_"+mac.substring(12,14)+mac.substring(15);
      WiFi.softAP(softAPname.c_str());
      WiFi.disconnect(false);

      Serial.print("AP is "); Serial.println(softAPname);
      Serial.print("AP IP address: "); Serial.println(WiFi.softAPIP());
   }
  
   Serial.println("write cfg ");
   if(false) WriteConfig(true); // if pin pushed write def config
   Serial.println("write cfg end");

   //if(cfg.s.skip_logon)  server.on("/"     , handleIndex1);
   //else server.on("/"     , handleLogin);
    
   server.on("/"     , handleLogin);

   server.on("/login", handleLogin);
   server.on("/index1", handleIndex1); // status
   server.on("/index2", HTTP_GET, handleIndex2); //timer settings
   server.on("/index3", handleWiFiSettings); //wifi settings
   server.on("/index4", handleSecurity); // pwd change
   server.on("/index5", handleLogoff);

   server.on("/favicon.ico", handleFavicon);
   server.onNotFound(handleNotFound);

   // ssdp
   ///server.on("/description.xml", HTTP_GET, [](AsyncWebServerRequest *request)){
   ///   SSDP.schema(server.client());
   ///});


   // webupdate
   //httpUpdater.setup(&server);
     
   // Start the server
   server.begin();
   DbgPrintln(("Server started"));
/*
   DbgPrintln(("Starting SSDP...\n"));

   SSDP.setDeviceType("upnp:rootdevice");
   SSDP.setSchemaURL("description.xml");
   SSDP.setHTTPPort(80);
   SSDP.setName("SHAPEsp");
   SSDP.setSerialNumber("SHAPEsp");
   SSDP.setURL("/");
   SSDP.setModelName("SHAPEsp-tmr");
   SSDP.setModelNumber("01");
   SSDP.setModelURL("http://www.google.com");
   SSDP.setManufacturer("SHAPEsp");
   SSDP.setManufacturerURL("http://www.google.com");
   SSDP.begin();
*/
   if(!ReadTmrPrg())
   { 
      DbgPrintln(("Failed read tmr config!!! Trying write default..."));
      SaveTmrPrg(true);
      DbgPrintln(("Done."));
   }

   // test deepsleep
   //    DbgPrintln(("Deepsleep for 10s"));
   //    ESP.deepSleep(10e6);

   tflag=false;
   mflag=0;
   skiptmr=false;
   voffflag=false;
   vaction=false;
   vstate=0;
   curstate=0;

   // must set valve off
   if(VALVE)
   {
      voffflag=true;
      //digitalWrite(led, LOW);
      digitalWrite(drvA1, LOW);
      digitalWrite(drvA2, HIGH);
      digitalWrite(drvSTBY, HIGH); timerSTBY.once_ms(300,STBYoff); // timer off
   }      
   timer.once_ms(5000,alarm); // start sheduler&timout timer
}

void loop()
{
  // server.handleClient();

   if(tflag)
   {
      tflag=false;
      voffflag=false;

      if(mflag==1) { mflag=0; skiptmr = true;  vaction=true; vstate=1; } // открыть вручную
      if(mflag==2) { mflag=0; skiptmr = true; vaction=true; vstate=0; } // закрыть вручную
      if(mflag==3) { mflag=0; skiptmr = false; } // автомат

      uint16_t adc = analogRead(A0);
      volt = adc*15.63/1024.0; //1000 15.98

      /////////////bme280
      if(bmepresent)
      {
         BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
         BME280::PresUnit presUnit(BME280::PresUnit_inHg);
         bme280.read(pres, temp, hum, tempUnit, presUnit);
      }      

      rtc.refresh();

      snprintf(current_time,96,"%02d/%02d/%02d %s %02d:%02d:%02d",
                 rtc.year(),rtc.month(),rtc.day(),dow[rtc.dayOfWeek()-1],rtc.hour(),rtc.minute(),rtc.second());

      //debug
      DbgPrint(("DateTime:")); DbgPrintln((current_time));

      int  pin2 = digitalRead(led);

      DbgPrint(("Water Alarm:")); DbgPrintln((String(pin2)));

      uint16_t tcur = rtc.hour()*60+rtc.minute();
      uint8_t cdow = 1 << (rtc.dayOfWeek()-1);

      if(!skiptmr)
      {
         for(int i=0;i<10;i++)
            if((prg.ta.p[i].active)&&(tcur==prg.ta.p[i].on_ts)&&(cdow&prg.ta.p[i].on_dowmask)) {vaction = true; vstate = 1; break;}

         for(int i=0;i<10;i++)  
            if((prg.ta.p[i].active)&&(tcur==prg.ta.p[i].off_ts)&&(cdow&prg.ta.p[i].off_dowmask)) { vaction = true; vstate = 0; break;}
      }
  
      if(vaction)
      {
         vaction = false;

         if(vstate!=curstate)
         {
            int a1,a2=a1=LOW;

            if(VALVE)
            {
               voffflag=true;
               //digitalWrite(led, LOW);
               if(vstate==1) { a1=HIGH; a2=LOW;  DbgPrintln(("on valve start")); }
               if(vstate==0) { a1=LOW;  a2=HIGH; DbgPrintln(("off valve start"));}
            }
            else
            {
               //voffflag=false;
               if(vstate==1) { a1=HIGH; a2=LOW;  DbgPrintln(("on relay"));  }
               if(vstate==0) { a1=LOW;  a2=LOW;  DbgPrintln(("off relay")); }
            }               
        
            digitalWrite(drvA1, a1);
            digitalWrite(drvA2, a2);
            digitalWrite(drvSTBY, HIGH); timerSTBY.once_ms(300,STBYoff); // timer off

            curstate = vstate;
         }
      }
      timer.once_ms(5000,alarm); // timer rearm
   }
}

