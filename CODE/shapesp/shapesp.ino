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

#include "TimeLib.h"

#include "brzo_i2c.h"

#include "AsyncMqttClient.h"

#define I2C_USE_BRZO 1

#define PDEBUG

#ifdef PDEBUG
    #define DEBUG_MSG(...) debugSend(__VA_ARGS__)
    #define DEBUG_MSG_P(...) debugSend_P(__VA_ARGS__)
    #define DEBUG_MSG1(str,...) debugSend_P(__VA_ARGS__);
#else
    #define DEBUG_MSG(...)
    #define DEBUG_MSG_P(...)
    #define DEBUG_MSG1(str,...)
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

//#include "eeprom24.h"
//EEPROM24Class eeprom(0x50,32,128);

#include "event.h"

std::queue<EspEvent *> sysqueue; // очередь сообщений
std::map<String, EspEvent *> msglist; // список подписок websocket

// events
DEFINE_EVENT(EVT_1SEC,1)
DEFINE_EVENT(EVT_60SEC,2)
DEFINE_EVENT(EVT_VCLOSE,3)
DEFINE_EVENT(EVT_VOPEN,4)
DEFINE_EVENT(EVT_VAUTO,5)

DEFINE_EVENT(EVT_VSTARTUP,6) // mqtt startup event

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
   EVENT_REGISTER_TASK(EVT_1SEC,task1,true) // периодические события
   EVENT_REGISTER_TASK(EVT_60SEC,task1,true) // обновление
   EVENT_REGISTER_TASK(EVT_1SEC,taskTimer,true)
   EVENT_REGISTER_TASK(EVT_1SEC,sens_task,cfg.dev.en_sensors)
   EVENT_REGISTER_TASK(EVT_1SEC,mqtt_task,cfg.dev.en_mqtt)

   EVENT_REGISTER_TASK(EVT_VCLOSE,taskTimer,true) // асинхронные события в очереди
   EVENT_REGISTER_TASK(EVT_VOPEN,taskTimer,true)
   EVENT_REGISTER_TASK(EVT_VAUTO,taskTimer,true)

   EVENT_REGISTER_TASK(EVT_VSTARTUP,taskTimer,true) //загрузка значений по умолчанию

   EVENT_REGISTER_TASK(EVT_MQTTPUB, mqtt_task,cfg.dev.en_mqtt) // публикация

   EVENT_REGISTER_TASK(EVT_MQTT,task1,true)
   EVENT_REGISTER_TASK(EVT_MQTT,taskTimer,true)
   EVENT_REGISTER_TASK(EVT_MQTT,sens_task,cfg.dev.en_sensors)
EVENT_END_REGISTER_TASKS

// обмен с web интерфейсом
MSG_BEGIN_REGISTER_TASKS
   MSG_REGISTER_TASK(MSG_STATUS,task1,true)
   MSG_REGISTER_TASK(MSG_STATUS,taskTimer,true)
   MSG_REGISTER_TASK(MSG_STATUS,sens_task,true)

   MSG_REGISTER_TASK(MSG_SET_TIME,taskTimer,true)

   MSG_REGISTER_TASK(MSG_SET_SETTINGS,taskSettings,true)

   MSG_REGISTER_TASK(MSG_SENSORS, sens_task,true)

   MSG_REGISTER_TASK(MSG_MQTT, mqtt_task,true)
MSG_END_REGISTER_TASKS

MSG_BEGIN_SUBSCRIBE
   MSG_SUBSCRIBE("status",MSG_STATUS)
   MSG_SUBSCRIBE("time",MSG_SET_TIME)
   MSG_SUBSCRIBE("wifi",MSG_SET_SETTINGS)
   MSG_SUBSCRIBE("senscnt",MSG_SENSORS)
   MSG_SUBSCRIBE("mqtt",MSG_MQTT)
MSG_END_SUBSCRIBE


const char* ssid = "DIR-300";
//const char* ssid = "CH1-Home";
const char* password = "chps74qwerty";

//const int led = 2; // led ow_pin

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
   Serial.begin(115200);
   Serial.flush();
   Serial.write('R');
   delay(5);
   char ch = Serial.read();
   Serial.end();

   if(ch=='R') // reset to defaults if jumper RX-TX set
   {
      WriteConfig(true,false);

      pinMode(2, OUTPUT);
      while(1)
      {
         digitalWrite(2, LOW);
         delay(500);
         digitalWrite(2, HIGH);
         delay(500);
      }
   }

   wifimode = 0; // station

   bool defaults = false;
   if(!ReadConfig())
   {
      WriteConfig(true,true);
      defaults = true;
   }

   String hostname(cfg.wifi.hostname);
   if(hostname=="")
   {
      String mac = WiFi.macAddress();
      hostname = "SHAPEsp_"+mac.substring(12,14)+mac.substring(15);
      snprintf(cfg.wifi.hostname,33,hostname.c_str());
   }

   setDebugPort(((cfg.dev.gpio2_mode == GPIO2_MODE_DEBUG) ? 1:0),115200);

   debugRecStart();

   DEBUG_MSG_P(PSTR("\n"));

   if(defaults) DEBUG_MSG_P(PSTR("FAILED read config!!! Writing defaults. \n"));//DEBUG_MSG_P(PSTR("FAILED read config!!! Writing defaults."));

   if(cfg.dev.gpio2_mode == GPIO2_MODE_1WIRE) { ow_pin = 2; DEBUG_MSG_P(PSTR("Set 1-Wire pin -> 2.\n"));}

   taskTimer.Initialize(cfg.dev.type); // первым делом инициализировали задачу таймера и закрыли кран/реле.

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

   DEBUG_MSG_P(PSTR("Startup at: %s \n"), strDateTime(startup).c_str());

   randomSeed(second());

   info();

   i2cScan();

   DEBUG_MSG_P(PSTR("eeprom Hostname: %s \n"), cfg.wifi.hostname);
   DEBUG_MSG_P(PSTR("Hostname: %s \n"), hostname.c_str());
   DEBUG_MSG_P(PSTR("User: %s \n"), cfg.wifi.user);
   DEBUG_MSG_P(PSTR("Pwd: %s \n"), cfg.wifi.pwd);

/*////////////////////////////////////////////
   DEBUG_MSG_P(PSTR("Test EEPROM \n"));

   eeprom.begin(0,2);
   uint8_t d[32];
   for(uint16_t ij=0;ij<32;ij++)
   {
      d[ij] = ij+10;
   }

   int s = millis();
   for(uint16_t j=0;j<64;j++)
   {
       eeprom.write(j, (uint8_t)j+1);
   }
   eeprom.end();

   int s = millis();

   eeprom.begin(0,2);
   for(uint16_t j=0;j<64;j++)
   {
      DEBUG_MSG_P(PSTR("EEPROM[%d] %d\n"), j, eeprom.read(j));
      //eeprom.read(j*32, d);
   }
   DEBUG_MSG_P(PSTR("write millis %d\n"), millis()-s);
   eeprom.end();
*/

   //WiFi.setOutputPower(20);
   //system_phy_set_max_tpw(50);
   //WiFi.setAutoConnect(false);

   WiFi.mode(WIFI_OFF);
   delay(200);
   WiFi.mode(WIFI_STA);

   WiFi.hostname(hostname);

   WiFi.begin(cfg.wifi.sta_ssid, cfg.wifi.sta_pwd);

   if(cfg.wifi.sta_dhcp == 0)
   {
      WiFi.config(IPAddress(cfg.wifi.sta_ip), IPAddress(cfg.wifi.sta_gw), IPAddress(cfg.wifi.sta_subnet));
   }

   uint16_t to = 60;
   while(WiFi.status() != WL_CONNECTED )
   {
      DEBUG_MSG_P(PSTR(".")); delay(500); to--;
      if(to==0) break;
   };
   DEBUG_MSG_P(PSTR("%d \n"), to);

   if(to)
   {
      WiFi.setSleepMode((WiFiSleepType_t)0);
      WiFi.setAutoReconnect(true);
      wifimode = 0; // station
      //Serial.print("Connected to "); Serial.println(cfg.wifi.sta_ssid);
      //Serial.print("IP address: "); Serial.println(WiFi.localIP());
      DEBUG_MSG_P(PSTR("Connected to %s IP address %s \n"), cfg.wifi.sta_ssid, WiFi.localIP().toString().c_str());
   }
   else
   {
      wifimode = 1; // softap
      WiFi.mode(WIFI_AP);
      String mac = WiFi.softAPmacAddress();
      softAPname = "SHAPEsp_"+mac.substring(12,14)+mac.substring(15);
      WiFi.softAP(softAPname.c_str());
      WiFi.disconnect(false);
      delay(500); // delay for ao startup, config work right after
      //WiFi.softAPConfig(apIP,apIP,IPAddress(255,255,255,0));
      //Serial.print("AP is "); Serial.println(softAPname);
      //Serial.print("AP IP address: "); Serial.println(WiFi.softAPIP());
      DEBUG_MSG_P(PSTR("AP is %s AP IP address %s \n"), softAPname.c_str(), WiFi.softAPIP().toString().c_str());
   }

   debugRecStop();

   debugRecSend();

   //DEBUG_MSG("write cfg \n");
   //if(false) WriteConfig(true,false); // if pin pushed write def config
   //DEBUG_MSG("write cfg end \n");

   // Init sensors
   sens_task.Initialize();
   if(wifimode==0) mqtt_task.Initialize();

   //if(cfg.s.skip_logon)  server.on("/"     , handleIndex1);
   //else server.on("/"     , handleLogin);

   server.rewrite("/","/index.html");
   server.on("/index.html", handleIndex);
   server.onNotFound(handleNotFound);
   server.on("/description.xml", HTTP_GET, handleSSDP);
   server.on("/reboot",HTTP_POST,handleReboot);
   server.on("/update", HTTP_POST,handleReset,handleUpload); // first upload then reset
   ws.onEvent(onWsEvent);
   server.addHandler(&ws);

   ssdpSetup();

   // Start the server
   server.begin();
   delay(10);

   DEBUG_MSG_P(PSTR("Server started. \n"));

   // test deepsleep
   //    DbgPrintln(("Deepsleep for 10s"));
   //    ESP.deepSleep(10e6);

   EventRegisterTasks();
   MsgRegisterTasks();
   MsgSubscribe();

   timer.attach_ms(1000,alarm); // start sheduler&timeout timer
}

//uint32_t sum = 0;

void loop()
{
   if(!sysqueue.empty())
   {
      /*
      if(sysqueue.front() == &GetEvent(EVT_1SEC))
      {
         DEBUG_MSG_P(PSTR("evt 1sec. %u\n") , sum);
         sum = 0;
      }*/
      //uint32_t start = millis();
      sysqueue.front()->doTasks();
      sysqueue.pop();
      //sum +=(millis()-start+1);
   }
   delay(1);
}
