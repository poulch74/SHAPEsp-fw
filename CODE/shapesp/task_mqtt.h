
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

      mqttClient.setServer(cfg.mqtt.server, cfg.mqtt.port);
      if(strlen(cfg.mqtt.clientID) > 0)
      {
         mqttClient.setClientId(cfg.mqtt.clientID);
      }

      mqttClient.setKeepAlive(cfg.mqtt.keepAlive);
      mqttClient.setCleanSession(false);

      if(strlen(cfg.mqtt.willTopic) > 0)
      {
         mqttClient.setWill(cfg.mqtt.willTopic, cfg.mqtt.qos, cfg.mqtt.retain, "0");
      }

      if ((strlen(cfg.mqtt.user) > 0) && (strlen(cfg.mqtt.pwd) > 0))
      {
         mqttClient.setCredentials(cfg.mqtt.user, cfg.mqtt.pwd);
      }

      DEBUG_MSG("Init MQTT task.\n");
      DEBUG_MSG(" Host: %s\n",cfg.mqtt.server);
      DEBUG_MSG(" Port: %d\n",cfg.mqtt.port);
      DEBUG_MSG(" ClientID: %s\n",cfg.mqtt.clientID);
      DEBUG_MSG(" keepAlive: %d\n",cfg.mqtt.keepAlive);
      DEBUG_MSG(" User: %s\n",cfg.mqtt.user);
      DEBUG_MSG(" Password: %s\n",cfg.mqtt.pwd);
      DEBUG_MSG(" QoS: %d\n",cfg.mqtt.qos);
      DEBUG_MSG(" Retain: %d\n",cfg.mqtt.retain);
      DEBUG_MSG(" Will topic: %s\n",cfg.mqtt.willTopic);
      DEBUG_MSG(" In topic: %s\n",cfg.mqtt.inTopic);
      DEBUG_MSG(" Out topic: %s\n",cfg.mqtt.outTopic);

      mqttClient.connect();
   }

   static void onMqttConnect(bool sessionPresent)
   {
      DEBUG_MSG("Connected to MQTT.\n");
      DEBUG_MSG("Session present: %d\n",sessionPresent);
      if(strlen(cfg.mqtt.outTopic)>0)
      {
         uint16_t packetIdSub = mqttClient.subscribe(cfg.mqtt.outTopic, cfg.mqtt.qos);
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
         if(idx == cfg.mqtt.idx_relay)
         {
            if(root["svalue1"].as<String>()=="Status")
            {
               DEBUG_MSG("[DOMOTICZ] Status IGNORED\n"); return;
            }
            DEBUG_MSG("[DOMOTICZ] Received value for Relay %u for IDX %u\n", value, idx);
            if(value==1)  { sysqueue.push(&GetEvent(EVT_VOPEN)); DEBUG_MSG("SCHEDULE OPEN MQTT\n"); }
            else {sysqueue.push(&GetEvent(EVT_VCLOSE)); DEBUG_MSG("SCHEDULE CLOSE MQTT\n");}
         }

         if(idx == cfg.mqtt.idx_mbtn)
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
      if(mqttClient.connected() && (strlen(cfg.mqtt.inTopic)>0))
      {
         for(int i=0;i<payload.size();i++)
         {
            uint16_t packetIdPub1 = mqttClient.publish(cfg.mqtt.inTopic, cfg.mqtt.qos, cfg.mqtt.retain, payload[i].c_str());
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
            snprintf(cfg.mqtt.server,64,"%s",iroot["mqtt_server"].as<String>().c_str());
            cfg.mqtt.port = iroot["mqtt_port"];
            snprintf(cfg.mqtt.user,20,"%s",iroot["mqtt_user"].as<String>().c_str());
            snprintf(cfg.mqtt.pwd,20,"%s",iroot["mqtt_pwd"].as<String>().c_str());
            snprintf(cfg.mqtt.inTopic,64,"%s",iroot["mqtt_intopic"].as<String>().c_str());
            snprintf(cfg.mqtt.outTopic,64,"%s",iroot["mqtt_outtopic"].as<String>().c_str());
            snprintf(cfg.mqtt.willTopic,64,"%s",iroot["mqtt_willtopic"].as<String>().c_str());
            snprintf(cfg.mqtt.clientID,32,"%s",iroot["mqtt_clientid"].as<String>().c_str());
            cfg.mqtt.qos = iroot["mqtt_qos"];
            cfg.mqtt.keepAlive = iroot["mqtt_keepalive"];
            cfg.mqtt.retain = iroot["mqtt_retain"];
            cfg.mqtt.idx_relay = iroot["mqtt_relay"];
            cfg.mqtt.idx_mbtn = iroot["mqtt_mbtn"];
            cfg.mqtt.idx_vcc = iroot["mqtt_vcc"];
            cfg.mqtt.idx_status = iroot["mqtt_status"];
            cfg.mqtt.idx_mode = iroot["mqtt_mode"];
            cfg.mqtt.idx_sens[0] = iroot["mqtt_sens0"];
            cfg.mqtt.idx_sens[1] = iroot["mqtt_sens1"];
            cfg.mqtt.idx_sens[2] = iroot["mqtt_sens2"];
            
            WriteConfig(false,false);
            
            cmd = "defaults";
         }

         if(cmd == "defaults") // send reply
         {
            root["action"] = "mqtt";
            root["mqtt_server"] = String(cfg.mqtt.server);
            root["mqtt_port"] = cfg.mqtt.port;
            root["mqtt_user"] = String(cfg.mqtt.user);
            root["mqtt_pwd"] = String(cfg.mqtt.pwd);
            root["mqtt_intopic"] = String(cfg.mqtt.inTopic);
            root["mqtt_outtopic"] = String(cfg.mqtt.outTopic);
            root["mqtt_willtopic"] = String(cfg.mqtt.willTopic);
            root["mqtt_clientid"] = String(cfg.mqtt.clientID);
            root["mqtt_qos"] = cfg.mqtt.qos;
            root["mqtt_keepalive"] = cfg.mqtt.keepAlive;
            root["mqtt_retain"] = cfg.mqtt.retain;
            root["mqtt_relay"] = cfg.mqtt.idx_relay;
            root["mqtt_mbtn"] = cfg.mqtt.idx_mbtn;
            root["mqtt_vcc"] = cfg.mqtt.idx_vcc;
            root["mqtt_status"] = cfg.mqtt.idx_status;
            root["mqtt_mode"] = cfg.mqtt.idx_mode;
            root["mqtt_sens0"] = cfg.mqtt.idx_sens[0];
            root["mqtt_sens1"] = cfg.mqtt.idx_sens[1];
            root["mqtt_sens2"] = cfg.mqtt.idx_sens[2];
         }
      }
   }

};


TaskMqtt mqtt_task;
