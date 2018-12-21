//extern "C" {
//#include "user_interface.h"
//}

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266SSDP.h>
#include <EEPROM.h>
#include <Ticker.h>

#define ARDUINOJSON_ENABLE_PROGMEM 1
#include <ArduinoJson.h>

#include "WebSocketIncommingBuffer.h"

#include <limits.h>
#include <math.h>

#include <Hash.h>
#include <functional>
#include <queue>
#include <map>

//#include <FS.h>
//#include <ESP8266HTTPUpdateServer.h>

#include "TimeLib.h"

#include "brzo_i2c.h"

#include "AsyncMqttClient.h"

#define I2C_USE_BRZO 1

#define PDEBUG

#ifdef PDEBUG
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
#else
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
#endif

#include "config.h"

ESP_SET cfg; // configuration struct

void prototypes(void) {} // here we collect all func prototypes

int wifimode;
String softAPname;

int rssi = 50; // store global, so any can access this
int battery = 100; // 0-100 percentage 10.8 - 12.5 map to 10-90%
int minbat = 108;
int maxbat = 140;

#include "event.h"

std::queue<EspEvent *> sysqueue; // очередь сообщений
std::map<String, EspEvent *> msglist; // список подписок websocket

// events
DEFINE_EVENT(EVT_1SEC,1)
DEFINE_EVENT(EVT_60SEC,2)
DEFINE_EVENT(EVT_VCLOSE,3)
DEFINE_EVENT(EVT_VOPEN,4)
DEFINE_EVENT(EVT_VAUTO,5)

DEFINE_EVENT(EVT_VSTARTUP,6)

DEFINE_EVENT(EVT_MQTTPUB,7) // ipc посылка msg

DEFINE_EVENT(EVT_MQTT,8) // выделенное событие, его mqtt_task перебирает когда вызывается

DEFINE_MSG(MSG_STATUS,101)
DEFINE_MSG(MSG_SET_TIME,102)
DEFINE_MSG(MSG_SET_SETTINGS,103)
DEFINE_MSG(MSG_SENSORS,104)
DEFINE_MSG(MSG_MQTT,105)

// include tasks files
#include "task.h"
#include "task_timer.h"

#include "sensor.h"
#include "bme280sens.h"
#include "ds1820sens.h"
#include "task_sens.h"

#include "task_mqtt.h"

//static int sec60cnt = 0;

EVENT_BEGIN_REGISTER_TASKS
   EVENT_REGISTER_TASK(EVT_1SEC,task1) // периодические события
   EVENT_REGISTER_TASK(EVT_60SEC,task1) // обновление
   EVENT_REGISTER_TASK(EVT_1SEC,taskTimer)
   EVENT_REGISTER_TASK(EVT_1SEC,sens_task)
   EVENT_REGISTER_TASK(EVT_1SEC,mqtt_task)

   EVENT_REGISTER_TASK(EVT_VCLOSE,taskTimer) // асинхронные события в очереди
   EVENT_REGISTER_TASK(EVT_VOPEN,taskTimer)
   EVENT_REGISTER_TASK(EVT_VAUTO,taskTimer)

   EVENT_REGISTER_TASK(EVT_VSTARTUP,taskTimer) //загрузка значение по умолчанию

   EVENT_REGISTER_TASK(EVT_MQTTPUB, mqtt_task) // публикация

   EVENT_REGISTER_TASK(EVT_MQTT,task1)
   EVENT_REGISTER_TASK(EVT_MQTT,taskTimer)
   EVENT_REGISTER_TASK(EVT_MQTT,sens_task)
EVENT_END_REGISTER_TASKS

// обмен с web интерфейсом
MSG_BEGIN_REGISTER_TASKS
   MSG_REGISTER_TASK(MSG_STATUS,task1)
   MSG_REGISTER_TASK(MSG_STATUS,taskTimer)
   MSG_REGISTER_TASK(MSG_STATUS,sens_task)

   MSG_REGISTER_TASK(MSG_SET_TIME,taskTimer)

   MSG_REGISTER_TASK(MSG_SET_SETTINGS,taskSettings)

   MSG_REGISTER_TASK(MSG_SENSORS, sens_task)

   MSG_REGISTER_TASK(MSG_MQTT, mqtt_task)
MSG_END_REGISTER_TASKS

MSG_BEGIN_SUBSCRIBE
   MSG_SUBSCRIBE("status",MSG_STATUS)
   MSG_SUBSCRIBE("time",MSG_SET_TIME)
   MSG_SUBSCRIBE("wifi",MSG_SET_SETTINGS)
   MSG_SUBSCRIBE("senscnt",MSG_SENSORS)
   MSG_SUBSCRIBE("mqtt",MSG_MQTT)
MSG_END_SUBSCRIBE


const int VALVE = 1; // 1 valve 0 relay

const char* ssid = "DIR-300";
//const char* ssid = "CH1-Home";
const char* password = "chps74qwerty";

const int led = 2; // led pin

Ticker timer;
void alarm()
{
   sysqueue.push(&GetEvent(EVT_1SEC));
   //sec5cnt++; if(sec5cnt == 5) { sysqueue.push(EVT_5SEC); sec5cnt = 0; }
   static int sec60cnt = 0;
   sec60cnt++; if(sec60cnt == 60) { sysqueue.push(&GetEvent(EVT_60SEC)); sec60cnt = 0; }
}

IPAddress apIP(192, 168, 4, 1);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void setup()
{
   wifimode = 0; // station

   DBGSERIAL.begin(115200);

   taskTimer.Initialize(); // первым делом инициализировали задачу таймера и закрыли кран реле.

   i2c_setup(4,5,200,400);

   if(i2cCheck(0x68)==0)
   {
      DEBUG_MSG_P(PSTR("DS3231 found at address 0x68. Setting SyncProvider... \n"));
      setSyncProvider(getTime_rtc);   // the function to get the time from the RTC
   }
   else
   {
      DEBUG_MSG_P(PSTR("RTC clock not found! Setting fake millis() SyncProvider... \n"));
      setSyncProvider(getTime_stub);
   }

   time_t startup = startUptime(); // start uptime calculation

   DEBUG_MSG_P(PSTR("Startup at: %s \n"),strDateTime(startup).c_str());

   randomSeed(second());

   //SPIFFS.begin();

   info();

   i2cScan();

   if(!ReadConfig())
   {
      DEBUG_MSG("Failed read config!!! Trying write defaults...");
      WriteConfig(true,false);
      DEBUG_MSG("Done. \n");
   }

   DEBUG_MSG("User: %s \n",cfg.wifi.user);
   DEBUG_MSG("Pwd: %s \n", cfg.wifi.pwd);

   //WiFi.setOutputPower(20);
   //system_phy_set_max_tpw(50);
   //WiFi.setAutoConnect(false);
   WiFi.mode(WIFI_OFF);
   delay(200);
   WiFi.mode(WIFI_STA);
   String mac = WiFi.macAddress();
   String hostname = "SHAPEsp_"+mac.substring(12,14)+mac.substring(15);

   WiFi.hostname(hostname);

   WiFi.begin(cfg.wifi.sta_ssid, cfg.wifi.sta_pwd);

   if(cfg.wifi.sta_dhcp == 0)
   {
      WiFi.config(IPAddress(cfg.wifi.sta_ip), IPAddress(cfg.wifi.sta_gw), IPAddress(cfg.wifi.sta_subnet));
   }


   uint16_t to = 50;
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
      Serial.print("Connected to "); Serial.println(cfg.wifi.sta_ssid);
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
   if(false) WriteConfig(true,false); // if pin pushed write def config
   DEBUG_MSG("write cfg end \n");

   // Init sensors
   sens_task.Initialize();
   if(wifimode==0) mqtt_task.Initialize();

   //if(cfg.s.skip_logon)  server.on("/"     , handleIndex1);
   //else server.on("/"     , handleLogin);

   server.rewrite("/","/index.html");
   server.on("/index.html", handleIndex);
   server.onNotFound(handleNotFound);
   server.on("/description.xml", HTTP_GET, handleSSDP);
   ws.onEvent(onWsEvent);
   server.addHandler(&ws);

   server.on("/reboot",HTTP_POST,
   [](AsyncWebServerRequest *request)
   {
      String str;
      if(isauth())
      {
         str = "<META http-equiv=\"refresh\" content=\"15;URL=/\">Rebooting... Wait about 15 sec.\n";
         deferredReset(200);
      }
      else
      {
         str = "<META http-equiv=\"refresh\" content=\"15;URL=/\">No Auth!!! Rejected! \n ";
      }
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", str);
      request->send(response);
   });


   server.on("/update", HTTP_POST,
   [](AsyncWebServerRequest *request)
   {
      String str;
      if(isauth())
      {
         // the request handler is triggered after the upload has finished...
         // create the response, add header, and send response
         str = "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update ";
         str += String((Update.hasError())?"FAIL! ":"Success! ") + "Rebooting... Wait about 15 sec.\n";
         AsyncWebServerResponse *response = request->beginResponse(200, "text/html", str);
         deferredReset(200);
      }
      {
         str = "<META http-equiv=\"refresh\" content=\"15;URL=/\">No Auth!!! Rejected! \n ";
      }
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", str);
      request->send(response);
   },
   [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool last)
   {
      //Upload handler chunks in data
      if(isauth())
      {
      if(!index)
      { // if index == 0 then this is the first frame of data
         Serial.printf("UploadStart: %s\n", filename.c_str());

         Serial.setDebugOutput(true);

         // calculate sketch space required for the update
         uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
         if(!Update.begin(maxSketchSpace))
         {//start with max available size
            Update.printError(Serial);
         }
         Update.runAsync(true); // tell the updaterClass to run in async mode
      }

      //Write chunked data to the free sketch space
      if(Update.write(data, len) != len) { Update.printError(Serial); }

      if(last)
      { // if the final flag is set then this is the last frame of data
         if(Update.end(true))
         { //true to set the size to the current progress
            Serial.printf("Update Success: %u B\nRebooting...\n", index+len);
         }
         else
         {
            Update.printError(Serial);
         }
        Serial.setDebugOutput(false);
      }
      }
   });

   ssdpSetup();

   // Start the server
   server.begin();
   delay(10);

   DEBUG_MSG("Server started \n");

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
   if(!sysqueue.empty())
   {
      sysqueue.front()->doTasks();
      sysqueue.pop();
   }
   yield();
}
