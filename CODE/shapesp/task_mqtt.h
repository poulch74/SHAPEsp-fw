extern ESP_MQTT mqttset;

AsyncMqttClient mqttClient;

class TaskMqtt : public EspTask
{
public:
   TaskMqtt() : EspTask() {}

   void Initialize()
   {
      //JsonObject& cfg = config.root();
      mqttClient.onConnect(onMqttConnect);
      mqttClient.onMessage(onMqttMessage);

      mqttClient.setServer(getSetting("mqtt_server","localhost").c_str(), getSetting("mqtt_port",1883).toInt());
      if(strlen(getSetting("mqtt_clientID",WiFi.hostname()).c_str()) > 0)
      {
         mqttClient.setClientId(getSetting("mqtt_clientID").c_str());
      }
      mqttClient.setKeepAlive(getSetting("mqtt_keepAlive",15));
      mqttClient.setCleanSession(false);

      if(strlen(getSetting("mqtt_willTopic","domoticz/out").c_str()) > 0)
      {
         mqttClient.setWill(getSetting("mqttset_willTopic"), getSetting("mqtt_qos",0), getSetting("mqtt_retain",0), "0");
      }

      if ((strlen(getSetting("mqtt_user").c_str()) > 0) && (strlen(getSetting("mqtt_pwd").c_str()) > 0))
      {
         mqttClient.setCredentials(getSetting("mqtt_user").c_str(), getSetting("mqtt_pwd").c_str());
      }

      in_topic = getSetting("mqtt_inTopic","domoticz/in");
      out_topic = getSetting("mqtt_outTopic","domoticz/out");
      idvcc = getSetting("mqtt_vcc",0);
      idrel = getSetting("mqtt_relay",0);
      idmbtn = getSetting("mqtt_mbtn",0);
      for(int i=0;i<10;i++) id[i] = getSetting("mqtt_sens",i,0);

      DEBUG_MSG("Init MQTT task.\n");
      DEBUG_MSG(" Host: %s\n",getSetting("mqtt_server"));
      DEBUG_MSG(" Port: %d\n",getSetting("mqtt_port"));
      DEBUG_MSG(" ClientID: %s\n",getSetting("mqtt_clientID"));
      DEBUG_MSG(" keepAlive: %d\n",getSetting("mqtt_keepAlive"));
      DEBUG_MSG(" User: %s\n",getSetting("mqtt_user"));
      DEBUG_MSG(" Password: %s\n",getSetting("mqtt_pwd"));
      DEBUG_MSG(" QoS: %d\n",getSetting("mqtt_qos"));
      DEBUG_MSG(" Retain: %d\n",getSetting("mqtt_retain"));
      DEBUG_MSG(" Will topic: %s\n",getSetting("mqtt_willTopic"));
      DEBUG_MSG(" In topic: %s\n",getSetting("mqtt_inTopic"));
      DEBUG_MSG(" Out topic: %s\n",getSetting("mqtt_outTopic"));

      mqttClient.connect();
   }

   static void onMqttConnect(bool sessionPresent)
   {
      DEBUG_MSG("Connected to MQTT.\n");
      DEBUG_MSG("Session present: %d\n",sessionPresent);
      if(strlen(out_topic.c_str())>0)
      {
         uint16_t packetIdSub = mqttClient.subscribe(out_topic.c_str(), getSetting("mqtt_qos",0));
      }
      // initial state
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
         if(idx == idrel)
         {
            if(root["svalue1"].as<String>()=="Status")
            {
               DEBUG_MSG("[DOMOTICZ] Status IGNORED\n"); return;
            }
            DEBUG_MSG("[DOMOTICZ] Received value for Relay %u for IDX %u\n", value, idx);
            if(value==1)  { sysqueue.push(&GetEvent(EVT_VOPEN)); DEBUG_MSG("SCHEDULE OPEN MQTT\n"); }
            else {sysqueue.push(&GetEvent(EVT_VCLOSE)); DEBUG_MSG("SCHEDULE CLOSE MQTT\n");}
         }

         if(idx == idmbtn)
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
      }
      else
      {
         if(WiFi.status() == WL_CONNECTED ) mqttClient.connect();
      }
   }

   // task то send mqtt msg from other tasks GetEvent(EVT_MQTTPUB).doTasks(payload);
   void doMqttTask(int evt, std::vector<String> &payload )
   {
      if(mqttClient.connected() && (strlen(inTopic)>0))
      {
         for(int i=0;i<payload.size();i++)
         {
            uint16_t packetIdPub1 = mqttClient.publish(mqttset.s.inTopic, mqttset.s.qos, mqttset.s.retain, payload[i].c_str());
         }
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
            snprintf(mqttset.s.server,64,"%s",iroot["mqtt_server"].as<String>().c_str());
            mqttset.s.port = iroot["mqtt_port"];
            snprintf(mqttset.s.user,20,"%s",iroot["mqtt_user"].as<String>().c_str());
            snprintf(mqttset.s.pwd,20,"%s",iroot["mqtt_pwd"].as<String>().c_str());
            snprintf(mqttset.s.inTopic,64,"%s",iroot["mqtt_intopic"].as<String>().c_str());
            snprintf(mqttset.s.outTopic,64,"%s",iroot["mqtt_outtopic"].as<String>().c_str());
            snprintf(mqttset.s.willTopic,64,"%s",iroot["mqtt_willtopic"].as<String>().c_str());
            snprintf(mqttset.s.clientID,32,"%s",iroot["mqtt_clientid"].as<String>().c_str());
            mqttset.s.qos = iroot["mqtt_qos"];
            mqttset.s.keepAlive = iroot["mqtt_keepalive"];
            mqttset.s.retain = iroot["mqtt_retain"];
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
            root["mqtt_server"] = String(mqttset.s.server);
            root["mqtt_port"] = mqttset.s.port;
            root["mqtt_user"] = String(mqttset.s.user);
            root["mqtt_pwd"] = String(mqttset.s.pwd);
            root["mqtt_intopic"] = String(mqttset.s.inTopic);
            root["mqtt_outtopic"] = String(mqttset.s.outTopic);
            root["mqtt_willtopic"] = String(mqttset.s.willTopic);
            root["mqtt_clientid"] = String(mqttset.s.clientID);
            root["mqtt_qos"] = mqttset.s.qos;
            root["mqtt_keepalive"] = mqttset.s.keepAlive;
            root["mqtt_retain"] = mqttset.s.retain;
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

   
private:
   String in_topic;
   String out_topic;
   int idvcc;
   int idrel;
   int mbtn;
   int ids[10];
};


TaskMqtt mqtt_task;
