#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266SSDP.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <ArduinoJson.h>

#include <limits.h>
#include <math.h>

#include <Hash.h>
#include <functional>
#include <queue>

#include <FS.h>
#include <ESP8266HTTPUpdateServer.h>

#include "TimeLib.h"

#include "brzo_i2c.h"
#include "BME280I2C_BRZO.h"

#define I2C_USE_BRZO 1

#define PDEBUG

#ifdef PDEBUG
   #define DbgPrintln(s) Serial.println s
   #define DbgPrint(s) Serial.print s
#else
   #define DbgPrint(s)
   #define DbgPrintln(s)
#endif

#include "relay.h"

void prototypes(void) {} // here we collect all func prototypes

const int VALVE = 1; // 1 valve 0 relay

const char* ssid = "DIR-300";
//const char* ssid = "CH1-Home";
const char* password = "chps74qwerty";

int session_id =0;

// events
#define EVT_1SEC 1
#define EVT_5SEC 2

std::queue<uint32_t> sysqueue; // очередь сообщений

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

static int sec5cnt = 0;
Ticker timer;
void alarm()
{ 
  // tflag = true;
   sysqueue.push(EVT_1SEC);
   sec5cnt++; if(sec5cnt == 5) { sysqueue.push(EVT_5SEC); sec5cnt = 0; }
}

Ticker timerSTBY; // timer 300ms pulse to start motor
void STBYoff() { digitalWrite(drvSTBY, LOW); }

bool skiptmr; // пропускать таймеры если открыл вручную, не авто режим
int mflag;
int curstate;

int wifimode;
String softAPname;

float volt;

BME280I2C_BRZO bme280;
bool bmepresent;
float temp,hum,pres;

IPAddress apIP(192, 168, 4, 1);

AsyncWebServer server(80);

Relay relay(14,12,13,R_VALVE,1);

ESP_CONFIG cfg;
ESP_TPRG prg;

void setup()
{
   wifimode = 0; // station
   bmepresent=0;
   temp=0;
   hum=0;
   pres=0;
   volt=0;

   Serial.begin(115200);

   relay.Initialize();

   i2c_setup(4,5,200,400);
   setSyncProvider(getTime_rtc);   // the function to get the time from the RTC

   time_t tm = now();
   DbgPrint(("NOW: ")); DbgPrintln((strDateTime(tm)));
/*
   sysevent *ev = new sysevent();
   ev->payload = "test";
   sysqueue.push(ev);

   ev = new sysevent();
   ev->payload = "test1";
   sysqueue.push(ev);
   

   genevent *e = sysqueue.front();
   sysqueue.pop();
   delete (sysevent *)e;
   DbgPrint((((sysevent *)e)->payload));
*/  
   randomSeed(second());

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

   mflag=0;
   skiptmr=false;
   curstate=0;

   timer.attach_ms(1000,alarm); // start sheduler&timout timer
}

void loop()
{
   uint32_t evt = 0; // include
   time_t t = now();

   if(!sysqueue.empty()) { evt = sysqueue.front();  sysqueue.pop(); }

   switch(evt)
   {
      case EVT_1SEC:
      {
         DbgPrint(("Queue size: ")); DbgPrintln((String(sysqueue.size())));
         DbgPrint(("DateTime: ")); DbgPrintln((strDateTime(t)));

         uint16_t adc = analogRead(A0);
         volt = adc*15.63/1024.0; //1000 15.98

         /////////////bme280
         if(bmepresent)
         {
            BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
            BME280::PresUnit presUnit(BME280::PresUnit_inHg);
            bme280.read(pres, temp, hum, tempUnit, presUnit);
         }      

         int  pin2 = digitalRead(led);
         DbgPrint(("Water Alarm:")); DbgPrintln((String(pin2)));

         if(mflag==1 || mflag==2) skiptmr = true;
         if(mflag==3) skiptmr = false;

         int vaction = TestSheduler(t,mflag,skiptmr);
         mflag = 0;

         if(vaction!=0)
         {  
           int state = 0;
           if(vaction>0) state = 1;
           curstate = relay.SetState(state);
         }
      } break;

      default: { delay(10); }// idle

   }   
}

