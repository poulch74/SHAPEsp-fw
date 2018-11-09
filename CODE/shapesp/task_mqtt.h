
#define MQTT_HOST IPAddress(192, 168, 137, 1)
#define MQTT_PORT 1883

extern ESP_MQTT mqttset;

AsyncMqttClient mqttClient;

class TaskMqtt : public EspTask
{
public:
   TaskMqtt() : EspTask() {}

   void Initialize()
   {
      DEBUG_MSG("Init MQTT task.\n");
      mqttClient.onConnect(onMqttConnect);
      //mqttClient.onDisconnect(onMqttDisconnect);
      //mqttClient.onSubscribe(onMqttSubscribe);
      //mqttClient.onUnsubscribe(onMqttUnsubscribe);
      mqttClient.onMessage(onMqttMessage);
      //mqttClient.onPublish(onMqttPublish);

      mqttClient.setServer(MQTT_HOST, MQTT_PORT);
      mqttClient.connect();
   }

   static void onMqttConnect(bool sessionPresent)
   {
      DEBUG_MSG("Connected to MQTT.\n");
      DEBUG_MSG("Session present: %d\n",sessionPresent);
      uint16_t packetIdSub = mqttClient.subscribe("domoticz/out", 2);

      // послать тут состояния выключателей
      GetEvent(EVT_VSTARTUP).doTasks();
   }

   static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
   {
      /*
      DEBUG_MSG("Publish received. \n");
      DEBUG_MSG("  topic: %s\n",topic);
      DEBUG_MSG("  qos: %d \n",properties.qos);
      DEBUG_MSG("  dup: %d\n",properties.dup);
      DEBUG_MSG("  retain: %d\n",properties.retain);
      DEBUG_MSG("  len: %d\n", len);
      DEBUG_MSG("  index: %d\n",index);
      DEBUG_MSG("  total: %d\n",total);
*/
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject((char *) payload);
      if (!root.success()) { DEBUG_MSG("[DOMOTICZ] Error parsing data\n");}
      else
      {
         int idx = root["idx"];
         int value = root["nvalue"];
         if(idx == mqttset.s.idx_relay)
         {
            if(root["svalue1"].as<String>()=="Status")
            {
               DEBUG_MSG("[DOMOTICZ] Status IGNORED\n"); return;
            }
            DEBUG_MSG("[DOMOTICZ] Received value for Relay %u for IDX %u\n", value, idx);
            if(value==1)  { sysqueue.push(&GetEvent(EVT_VOPEN)); DEBUG_MSG("SCHEDULE OPEN MQTT\n"); }
            else {sysqueue.push(&GetEvent(EVT_VCLOSE)); DEBUG_MSG("SCHEDULE CLOSE MQTT\n");}
         }

         if(idx == mqttset.s.idx_mbtn)
         {
            if(root["svalue1"].as<String>()=="Status")
            {
               DEBUG_MSG("[DOMOTICZ] Status IGNORED\n"); return;
            }
            DEBUG_MSG("[DOMOTICZ] Received value for Button %u for IDX %u\n", value, idx);
            if(value==1)  { sysqueue.push(&GetEvent(EVT_VAUTO)); DEBUG_MSG("SCHEDULE AUTO MQTT\n"); }
         }

      }
   }

   void doTask(int evt)
   {
      if(mqttClient.connected())
      {
         std::vector<String> payload;
         GetEvent(EVT_MQTT).doTasks(payload);
         doMqttTask(evt, payload);
         /*
         for(int i=0;i<payload.size();i++)
         {
            uint16_t packetIdPub1 = mqttClient.publish("domoticz/in", 0, true, payload[i].c_str());
         }*/
      }
      else
      {
         if(WiFi.status() == WL_CONNECTED ) mqttClient.connect();
      }
   }

   // task то send mqtt msg from other tasks GetEvent(EVT_MQTTPUB).doTasks(payload);
   void doMqttTask(int evt, std::vector<String> &payload )
   {
      for(int i=0;i<payload.size();i++)
      {
         uint16_t packetIdPub1 = mqttClient.publish("domoticz/in", 0, true, payload[i].c_str());
      }
   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      String cmd = iroot["cmd"];
      String event = iroot["text"];

      if(event== "mqtt")
      {
         if(cmd == "setmqtt")
         {
            DEBUG_MSG("setmqtt and response");
            mqttset.s.idx_relay = iroot["mqtt_relay"];
            mqttset.s.idx_mbtn = iroot["mqtt_mbtn"];
            mqttset.s.idx_vcc = iroot["mqtt_vcc"];
            mqttset.s.idx_status = iroot["mqtt_status"];
            mqttset.s.idx_mode = iroot["mqtt_mode"];
            mqttset.s.idx_sens[0] = iroot["mqtt_sens0"];
            mqttset.s.idx_sens[1] = iroot["mqtt_sens1"];
            mqttset.s.idx_sens[2] = iroot["mqtt_sens2"];
            
            SaveMqttSettings(false);
            
            cmd = "defaults";
         }

         if(cmd == "defaults") // send reply
         {
            root["action"] = "mqtt";
            root["mqtt_relay"] = mqttset.s.idx_relay;
            root["mqtt_mbtn"] = mqttset.s.idx_mbtn;
            root["mqtt_vcc"] = mqttset.s.idx_vcc;
            root["mqtt_status"] = mqttset.s.idx_status;
            root["mqtt_mode"] = mqttset.s.idx_mode;
            root["mqtt_sens0"] = mqttset.s.idx_sens[0];
            root["mqtt_sens1"] = mqttset.s.idx_sens[1];
            root["mqtt_sens2"] = mqttset.s.idx_sens[2];
         }
      }
   }

   
public:
   
};


TaskMqtt mqtt_task;
