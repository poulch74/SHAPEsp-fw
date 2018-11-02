
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
//      DEBUG_MSG("sensor cnt %d %d\n", sensors.size(), sens_count);
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
      Serial.println("Connected to MQTT.");
      Serial.print("Session present: ");
      Serial.println(sessionPresent);
      uint16_t packetIdSub = mqttClient.subscribe("domoticz/out", 2);
      Serial.print("Subscribing at QoS 2, packetId: ");
      Serial.println(packetIdSub);
   }

   static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
   {
      Serial.println("Publish received.");
      Serial.print("  topic: ");
      Serial.println(topic);
      Serial.print("  qos: ");
      Serial.println(properties.qos);
      Serial.print("  dup: ");
      Serial.println(properties.dup);
      Serial.print("  retain: ");
      Serial.println(properties.retain);
      Serial.print("  len: ");
      Serial.println(len);
      Serial.print("  index: ");
      Serial.println(index);
      Serial.print("  total: ");
      Serial.println(total);
   }

   void doTask(int evt)
   {
      if(mqttClient.connected())
      {
         std::vector<String> payload;
         __evtEVT_MQTT.doTasks(payload);
         for(int i=0;i<payload.size();i++)
         {
            uint16_t packetIdPub1 = mqttClient.publish("domoticz/in", 0, true, payload[i].c_str());
            Serial.print("Publishing at QoS 1, packetId: ");
         }
      }
      else
      {
         if(WiFi.status() == WL_CONNECTED ) mqttClient.connect();
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
            mqttset.s.idx_vcc = iroot["mqtt_vcc"];
            mqttset.s.idx_status = iroot["mqtt_status"];
            mqttset.s.idx_mode = iroot["mqtt_mode"];
            
            SaveMqttSettings(false);
            
            cmd = "defaults";
         }

         if(cmd == "defaults") // send reply
         {
            root["action"] = "mqtt";  
            root["mqtt_vcc"] = mqttset.s.idx_vcc;
            root["mqtt_status"] = mqttset.s.idx_status;
            root["mqtt_mode"] = mqttset.s.idx_mode;
         }
      }
   }

   
public:
   
};


TaskMqtt mqtt_task;
