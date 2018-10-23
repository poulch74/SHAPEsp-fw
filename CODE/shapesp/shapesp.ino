#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266SSDP.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include "WebSocketIncommingBuffer.h"

#include <limits.h>
#include <math.h>

#include <Hash.h>
#include <functional>
#include <queue>
#include <map>

#include <FS.h>
#include <ESP8266HTTPUpdateServer.h>

#include "TimeLib.h"

#include "brzo_i2c.h"
#include "BME280I2C_BRZO.h"

#define I2C_USE_BRZO 1

#define PDEBUG

#ifdef PDEBUG
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#else
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif

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


void prototypes(void) {} // here we collect all func prototypes

int wifimode;
String softAPname;

//#include "relay.h"

#include "event.h"

std::queue<EspEvent *> sysqueue; // очередь сообщений
std::map<String, EspEvent *> msglist; // список подписок websocket

// events

//#define EVT_5SEC 2

DEFINE_EVENT(EVT_1SEC,1)
DEFINE_EVENT(EVT_VCLOSE,3)
DEFINE_EVENT(EVT_VOPEN,4)
DEFINE_EVENT(EVT_VAUTO,5)

DEFINE_MSG(MSG_STATUS,101)
DEFINE_MSG(MSG_SET_TIME,102)
DEFINE_MSG(MSG_SET_SETTINGS,103)

// include tasks files
#include "task.h"
#include "task_timer.h"


EVENT_BEGIN_REGISTER_TASKS
   EVENT_REGISTER_TASK(EVT_1SEC,task1) // периодические события
   EVENT_REGISTER_TASK(EVT_1SEC,taskTimer)

   EVENT_REGISTER_TASK(EVT_VCLOSE,taskTimer) // асинхронные события в очереди 
   EVENT_REGISTER_TASK(EVT_VOPEN,taskTimer)
   EVENT_REGISTER_TASK(EVT_VAUTO,taskTimer)
EVENT_END_REGISTER_TASKS

MSG_BEGIN_REGISTER_TASKS
   MSG_REGISTER_TASK(MSG_STATUS,task1)
   MSG_REGISTER_TASK(MSG_STATUS,taskTimer)

   MSG_REGISTER_TASK(MSG_SET_TIME,taskTimer)

   MSG_REGISTER_TASK(MSG_SET_SETTINGS,taskSettings)
MSG_END_REGISTER_TASKS

MSG_BEGIN_SUBSCRIBE
   MSG_SUBSCRIBE("status",MSG_STATUS)
   MSG_SUBSCRIBE("time",MSG_SET_TIME)
   MSG_SUBSCRIBE("wifi",MSG_SET_SETTINGS)
MSG_END_SUBSCRIBE


const int VALVE = 1; // 1 valve 0 relay

const char* ssid = "DIR-300";
//const char* ssid = "CH1-Home";
const char* password = "chps74qwerty";


const int led = 2; // led pin

Ticker timer;
void alarm()
{ 
   sysqueue.push(&__evtEVT_1SEC);
   //sec5cnt++; if(sec5cnt == 5) { sysqueue.push(EVT_5SEC); sec5cnt = 0; }
}


float volt;

BME280I2C_BRZO bme280;
bool bmepresent;
float temp,hum,pres;

IPAddress apIP(192, 168, 4, 1);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

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

   taskTimer.Initialize(); // первым делом инициализировали задачу таймера и закрыли кран реле.

   i2c_setup(4,5,200,400);
   setSyncProvider(getTime_rtc);   // the function to get the time from the RTC

   time_t tm = now();
   DEBUG_MSG_P(PSTR("NOW: %s \n"),strDateTime(tm).c_str());

   randomSeed(second());

   SPIFFS.begin();

   info();

   i2cScan();

   bmepresent = bme280.begin();

   if(!ReadConfig())
   { 
      DEBUG_MSG("Failed read config!!! Trying write default...");
      WriteConfig(true);
      DEBUG_MSG("Done. \n");
   }
  
   DEBUG_MSG("User: %s \n",cfg.s.user);
   DEBUG_MSG("Pwd: %s \n", cfg.s.pwd);

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
  
   DEBUG_MSG("write cfg \n");
   if(false) WriteConfig(true); // if pin pushed write def config
   DEBUG_MSG("write cfg end \n");

   //if(cfg.s.skip_logon)  server.on("/"     , handleIndex1);
   //else server.on("/"     , handleLogin);

   server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
   server.on("/favicon.ico", handleFavicon);
   server.onNotFound(handleNotFound);

   ws.onEvent(onWsEvent);
   server.addHandler(&ws);


   // ssdp
   ///server.on("/description.xml", HTTP_GET, [](AsyncWebServerRequest *request)){
   ///   SSDP.schema(server.client());
   ///});


   // webupdate
   //httpUpdater.setup(&server);
     
   // Start the server
   server.begin();
   delay(10);

   DEBUG_MSG("Server started \n");
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
      DEBUG_MSG("Failed read tmr config!!! Trying write default...");
      SaveTmrPrg(true);
      DEBUG_MSG("Done.\n");
   }

   // test deepsleep
   //    DbgPrintln(("Deepsleep for 10s"));
   //    ESP.deepSleep(10e6);

   EventRegisterTasks();
   MsgRegisterTasks();
   MsgSubscribe();
   timer.attach_ms(1000,alarm); // start sheduler&timeout timer
}


void loop()
{
   time_t t = now();
   if(!sysqueue.empty())
   { 
      sysqueue.front()->doTasks();
      sysqueue.pop();
   }
   yield();
}

/*
         /////////////bme280
         if(bmepresent)
         {
            BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
            BME280::PresUnit presUnit(BME280::PresUnit_inHg);
            bme280.read(pres, temp, hum, tempUnit, presUnit);
         }      

         int  pin2 = digitalRead(led);
         DbgPrint(("Water Alarm:")); DbgPrintln((String(pin2)));

   }   
*/   