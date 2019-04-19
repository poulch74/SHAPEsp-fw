
AsyncMqttClient mqttClient;

class TaskMqtt : public EspTask
{
public:
   TaskMqtt() : EspTask() {}

   void Initialize()
   {
      if(cfg.dev.en_mqtt) // enabled
      {
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

         DEBUG_MSG1("Init MQTT task.\n", dstring30);
         DEBUG_MSG1(" Host: %s\n", dstring31,cfg.mqtt.server);
         DEBUG_MSG1(" Port: %d\n", dstring32,cfg.mqtt.port);
         DEBUG_MSG1(" ClientID: %s\n", dstring33,cfg.mqtt.clientID);
         DEBUG_MSG1(" keepAlive: %d\n", dstring34,cfg.mqtt.keepAlive);
         DEBUG_MSG1(" User: %s\n", dstring35,cfg.mqtt.user);
         DEBUG_MSG1(" Password: %s\n", dstring36,cfg.mqtt.pwd);
         DEBUG_MSG1(" QoS: %d\n", dstring37,cfg.mqtt.qos);
         DEBUG_MSG1(" Retain: %d\n", dstring38,cfg.mqtt.retain);
         DEBUG_MSG1(" Will topic: %s\n", dstring39,cfg.mqtt.willTopic);
         DEBUG_MSG1(" In topic: %s\n", dstring40,cfg.mqtt.inTopic);
         DEBUG_MSG1(" Out topic: %s\n", dstring41, cfg.mqtt.outTopic);

         mqttClient.connect();
      }
      else
      {
         DEBUG_MSG_P(PSTR("MQTT task disabled!!!.\n"));
      }

   }

   static void onMqttConnect(bool sessionPresent)
   {
      DEBUG_MSG1("Connected to MQTT.\n", dstring42);
      DEBUG_MSG1("Session present: %d\n", dstring43,sessionPresent);
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
      if (!root.success()) { DEBUG_MSG1("[DOMOTICZ] Error parsing data\n", dstring44); }
      else
      {
         int idx = root["idx"];
         int value = root["nvalue"];
         if(idx == cfg.mqtt.idx_relay)
         {
            if(root["svalue1"]=="Status")
            {
               DEBUG_MSG1("[DOMOTICZ] Status IGNORED\n", dstring45); return;
            }
            DEBUG_MSG1("[DOMOTICZ] Received value for Relay %u for IDX %u\n", dstring46, value, idx);
            if(value==1)  { sysqueue.push(&GetEvent(EVT_VOPEN)); DEBUG_MSG1("SCHEDULE OPEN MQTT\n", dstring47); }
            else {sysqueue.push(&GetEvent(EVT_VCLOSE)); DEBUG_MSG1("SCHEDULE CLOSE MQTT\n", dstring48);}
         }

         if(idx == cfg.mqtt.idx_mbtn)
         {
            if(root["svalue1"]=="Status")
            {
               DEBUG_MSG1("[DOMOTICZ] Status IGNORED\n", dstring49); return;
            }
            DEBUG_MSG1("[DOMOTICZ] Received value for Button %u for IDX %u\n", dstring50, value, idx);
            if(value==1)  { sysqueue.push(&GetEvent(EVT_VAUTO)); DEBUG_MSG1("SCHEDULE AUTO MQTT\n", dstring51); }
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
            uint16_t packetIdPub1 = mqttClient.publish(cfg.mqtt.inTopic, cfg.mqtt.qos, cfg.mqtt.retain,
                                                         payload[i].c_str());
         }
      }
   }

   // work always - configuration part of task
   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
      String cmd = iroot["cmd"];
      String event = iroot["text"];

      if(event== "mqtt")
      {
         if(cmd == "setmqtt")
         {
            DEBUG_MSG1("setmqtt and response", dstring52);
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

            for(int i=0; i<MAX_SENSORS_CNT; i++)
            {
               String mqttstr = "mqtt_sens" + String(i);
               cfg.mqtt.idx_sens[i] = iroot[mqttstr];
            }
            WriteConfig(false,false);
         }

         // defaults
         ReadConfig();
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

         for(int i=0; i<MAX_SENSORS_CNT; i++)
         {
            String mqttstr = "mqtt_sens" + String(i);
            root[mqttstr] = cfg.mqtt.idx_sens[i];
         }
      }
   }

};

TaskMqtt mqtt_task;
