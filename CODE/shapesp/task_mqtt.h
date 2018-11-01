
#define MQTT_HOST IPAddress(192, 168, 137, 1)
#define MQTT_PORT 1883


class TaskMqtt : public EspTask
{
public:
   TaskMqtt() : EspTask() {}

   void Initialize()
   {
//      DEBUG_MSG("sensor cnt %d %d\n", sensors.size(), sens_count);
     // mqttClient.onConnect(onMqttConnect);
      //mqttClient.onDisconnect(onMqttDisconnect);
      //mqttClient.onSubscribe(onMqttSubscribe);
      //mqttClient.onUnsubscribe(onMqttUnsubscribe);
      //mqttClient.onMessage(onMqttMessage);
      //mqttClient.onPublish(onMqttPublish);

      mqttClient.setServer(MQTT_HOST, MQTT_PORT);
      mqttClient.connect();
   }


   void doTask(int evt)
   {
      if(mqttClient.connected())
      {
         uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, true, "test 2");
         Serial.print("Publishing at QoS 1, packetId: ");
         Serial.println(packetIdPub1);
         uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, true, "test 3");
         Serial.print("Publishing at QoS 2, packetId: ");
         Serial.println(packetIdPub2);
      }
      else {
         Serial.print("Not connected \n");
         mqttClient.connect();
      }

   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
   }

private:
   AsyncMqttClient mqttClient;
};



TaskMqtt mqtt_task;
