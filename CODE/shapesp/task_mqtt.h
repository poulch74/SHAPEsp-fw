
#define LIBRARY "PangolinMQTT "PANGO_VERSION

PangolinMQTT mqttClient;


//AsyncMqttClient mqttClient;

int mqtt_lock,mqtt_connected;

class TaskMqtt : public EspTask
{
public:
   TaskMqtt() : EspTask() {}

   void Initialize()
   {
      if(cfg.dev.en_mqtt) // enabled
      {
         mqttClient.onConnect(onMqttConnect);
         mqttClient.onDisconnect(onMqttDisconnect);
         mqttClient.onMessage(onMqttMessage);
         mqttClient.onError(onMqttError);

         mqttClient.setServer(cfg.mqtt.server, cfg.mqtt.port);
         if(strlen(cfg.mqtt.clientID) > 0)
         {
            mqttClient.setClientId(cfg.mqtt.clientID);
         }

         mqttClient.setKeepAlive(cfg.mqtt.keepAlive);
         mqttClient.setCleanSession(true);//false);

         if(strlen(cfg.mqtt.willTopic) > 0)
         {
            mqttClient.setWill(cfg.mqtt.willTopic, cfg.mqtt.qos, cfg.mqtt.retain, "0");
         }

         if ((strlen(cfg.mqtt.user) > 0) && (strlen(cfg.mqtt.pwd) > 0))
         {
            mqttClient.setCredentials(cfg.mqtt.user, cfg.mqtt.pwd);
         }

         DEBUG_MSG_P(PSTR("Init MQTT task.\n"));
         DEBUG_MSG_P(PSTR(" Host: %s\n"), cfg.mqtt.server);
         DEBUG_MSG_P(PSTR(" Port: %d\n"), cfg.mqtt.port);
         DEBUG_MSG_P(PSTR(" ClientID: %s\n"), cfg.mqtt.clientID);
         DEBUG_MSG_P(PSTR(" keepAlive: %d\n"), cfg.mqtt.keepAlive);
         DEBUG_MSG_P(PSTR(" User: %s\n"), cfg.mqtt.user);
         DEBUG_MSG_P(PSTR(" Password: %s\n"), cfg.mqtt.pwd);
         DEBUG_MSG_P(PSTR(" QoS: %d\n"), cfg.mqtt.qos);
         DEBUG_MSG_P(PSTR(" Retain: %d\n"), cfg.mqtt.retain);
         DEBUG_MSG_P(PSTR(" Will topic: %s\n"), cfg.mqtt.willTopic);
         DEBUG_MSG_P(PSTR(" In topic: %s\n"), cfg.mqtt.inTopic);
         DEBUG_MSG_P(PSTR(" Out topic: %s\n"), cfg.mqtt.outTopic);

         mqtt_lock = 1;
         mqtt_connected = 0;
         mqttClient.connect();
      }
      else
      {
         DEBUG_MSG_P(PSTR("MQTT task disabled!!!.\n"));
      }

   }

static void onMqttError(uint8_t e,uint32_t info)
{
  switch(e){
    case TCP_DISCONNECTED:
        // usually because your structure is wrong and you called a function before onMqttConnect
        DEBUG_MSG("ERROR: NOT CONNECTED info=%d\n",info);
        break;
    case MQTT_SERVER_UNAVAILABLE:
        // server has gone away - network problem? server crash?
        DEBUG_MSG("ERROR: MQTT_SERVER_UNAVAILABLE info=%d\n",info);
        break;
    case UNRECOVERABLE_CONNECT_FAIL:
        // there is something wrong with your connection parameters? IP:port incorrect? user credentials typo'd?
        DEBUG_MSG("ERROR: UNRECOVERABLE_CONNECT_FAIL info=%d\n",info);
        break;
    case TLS_BAD_FINGERPRINT:
        DEBUG_MSG("ERROR: TLS_BAD_FINGERPRINT info=%d\n",info);
        break;
    case SUBSCRIBE_FAIL:
        // you tried to subscribe to an invalid topic
        DEBUG_MSG("ERROR: SUBSCRIBE_FAIL info=%d\n",info);
        break;
    case INBOUND_QOS_ACK_FAIL:
        DEBUG_MSG("ERROR: OUTBOUND_QOS_ACK_FAIL id=%d\n",info);
        break;
    case OUTBOUND_QOS_ACK_FAIL:
        DEBUG_MSG("ERROR: OUTBOUND_QOS_ACK_FAIL id=%d\n",info);
        break;
    case INBOUND_PUB_TOO_BIG:
        // someone sent you a p[acket that this MCU does not have enough FLASH to handle
        DEBUG_MSG("ERROR: INBOUND_PUB_TOO_BIG size=%d Max=%d\n",e,mqttClient.getMaxPayloadSize());
        break;
    case OUTBOUND_PUB_TOO_BIG:
        // you tried to send a packet that this MCU does not have enough FLASH to handle
        DEBUG_MSG("ERROR: OUTBOUND_PUB_TOO_BIG size=%d Max=%d\n",e,mqttClient.getMaxPayloadSize());
        break;
    case BOGUS_PACKET: //  Your server sent a control packet type unknown to MQTT 3.1.1 
    //  99.99% unlikely to ever happen, but this message is better than a crash, non? 
        DEBUG_MSG("ERROR: BOGUS_PACKET TYPE=%02x\n",e,info);
        break;
    case X_INVALID_LENGTH: //  An x function rcvd a msg with an unexpected length: probale data corruption or malicious msg 
    //  99.99% unlikely to ever happen, but this message is better than a crash, non? 
        DEBUG_MSG("ERROR: X_INVALID_LENGTH TYPE=%02x\n",e,info);
        break;
    case OUTBOUND_PUB_HEAP_LOW: //  An x function rcvd a msg with an unexpected length: probale data corruption or malicious msg 
    //  99.99% unlikely to ever happen, but this message is better than a crash, non? 
        DEBUG_MSG("ERROR: OUTBOUND_PUB_HEAP_LOW=%02x\n",e,info);
        break;

    default:
      Serial.printf("UNKNOWN ERROR: %u extra info %d",e,info);
      break;
  }
}
// end error-handling


   static void onMqttConnect(bool sessionPresent)
   {
      DEBUG_MSG_P(PSTR("Connected to MQTT.\n"));
      DEBUG_MSG_P(PSTR("Session present: %d\n"), sessionPresent);
      if(strlen(cfg.mqtt.outTopic)>0)
      {
         mqttClient.subscribe(cfg.mqtt.outTopic, cfg.mqtt.qos);
      }
      // initial state
      GetEvent(EVT_VSTARTUP).doTasks(nullptr);
      mqtt_connected = 1;
   }

   static void onMqttDisconnect(uint8_t reason)
   {
      DEBUG_MSG_P(PSTR("Disconnected from MQTT.\n"));
      mqtt_lock = 0;
      mqtt_connected = 0;
   }


   //static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
   static void onMqttMessage(const char* topic, const uint8_t* payload, size_t len,uint8_t qos,bool retain,bool dup)
   {
      
      DEBUG_MSG("Publish received. \n");
      DEBUG_MSG("  topic: %s\n",topic);
      DEBUG_MSG("  qos: %d \n",qos);
      DEBUG_MSG("  dup: %d\n",dup);
      DEBUG_MSG("  retain: %d\n",retain);
      DEBUG_MSG("  len: %d\n", len);
      char *dbuf = new char[len+1];
      snprintf(dbuf, len+1,"%s",payload);
      DEBUG_MSG("  payload: %s\n",dbuf);
      delete[] dbuf;

      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject((char *) payload);
      if (!root.success()) { DEBUG_MSG_P(PSTR("[DOMOTICZ] Error parsing data\n")); }
      else
      {
         int idx = root["idx"];
         int value = root["nvalue"];
         if(idx == cfg.mqtt.idx_relay)
         {
            if(root["svalue1"]=="Status")
            {
               DEBUG_MSG_P(PSTR("[DOMOTICZ] Status IGNORED\n")); return;
            }
            DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value for Relay %u for IDX %u\n"), value, idx);
            if(value==1)
            {
               sysqueue.push(EspEvent(&GetEvent(EVT_VOPEN)));
               DEBUG_MSG_P(PSTR("SCHEDULE OPEN MQTT\n"));
            }
            else
            {
               sysqueue.push(EspEvent(&GetEvent(EVT_VCLOSE)));
               DEBUG_MSG_P(PSTR("SCHEDULE CLOSE MQTT\n"));
            }
         }

         if(idx == cfg.mqtt.idx_mbtn)
         {
            if(root["svalue1"]=="Status")
            {
               DEBUG_MSG_P(PSTR("[DOMOTICZ] Status IGNORED\n")); return;
            }
            DEBUG_MSG_P(PSTR("[DOMOTICZ] Received value for Button %u for IDX %u\n"), value, idx);
            if(value==1)
            {
               sysqueue.push(EspEvent(&GetEvent(EVT_VAUTO)));
               DEBUG_MSG_P(PSTR("SCHEDULE AUTO MQTT\n"));
            }
         }

         // тут if и send noolite типа GetEvent(EVT_NOOSEND).doTasks();

      }
   }

   void doTask(int evt, void *data)
   {
      if(mqtt_lock && mqtt_connected)
      {
         std::vector<String> payload;
         GetEvent(EVT_MQTT).doTasks(payload);
         doMqttTask(evt, payload);
      }
      else
      {
         if(WiFi.isConnected())
         { 
            if(!mqtt_lock)
            {
               DEBUG_MSG_P(PSTR("try reconnect mqtt\n"));
               mqtt_lock=1;
               mqttClient.connect();
            }
         }
         //if(WiFi.status() == WL_CONNECTED ) mqttClient.connect();
      }
   }

   // task то send mqtt msg from other tasks GetEvent(EVT_MQTTPUB).doTasks(payload);
   void doMqttTask(int evt, std::vector<String> &payload )
   {
      if(/*mqttClient.connected() && */(strlen(cfg.mqtt.inTopic)>0))
      {
         for(int i=0;i<payload.size();i++)
         {
            mqttClient.publish(cfg.mqtt.inTopic, payload[i].c_str(), payload[i].length(), cfg.mqtt.qos, cfg.mqtt.retain);
            DynamicJsonBuffer jsonBuffer;
            JsonObject& root = jsonBuffer.parseObject(payload[i]);
            if (!root.success()) { DEBUG_MSG_P(PSTR("[JSON_helper] Error parsing data\n")); }
            else
            {
               int idx = root["idx"];
               int nvalue = root["nvalue"];
               String svalue= root["svalue"];
               char topic[80]; snprintf(topic,80,"%s/idx%i",cfg.mqtt.inTopic,idx);
               mqttClient.publish(topic, svalue.c_str(), svalue.length(), cfg.mqtt.qos, cfg.mqtt.retain);
            }

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
            DEBUG_MSG_P(PSTR("setmqtt and response"));
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
