class EspTask
{
public:
   EspTask() { }
   virtual void doTask(int evt) {}
   virtual void doSend(int evt, JsonObject &iroot, JsonObject &root) {}
   virtual void doRecv(int evt, JsonObject &iroot, JsonObject &root) {}
};

class TestTask1 : public EspTask
{
public:
   TestTask1() : EspTask() {}
   void doTask(int evt)
   {
      DbgPrintln(("DoTask1"));
      uint16_t adc = analogRead(A0);
      vcc = adc*15.63/1024.0; //1000 15.98
      heap = ESP.getFreeHeap();
      rssi = WiFi.RSSI();
   }

   void doSend(int evt, JsonObject &iroot, JsonObject &root)
   {
      DbgPrintln(("sendTask1"));
      root["status_wifimode"] = String((wifimode ? "SoftAP":"Station"));
      if(wifimode)
      {
            root["status_wifiip"] = WiFi.softAPIP().toString();
            root["status_wifissid"] = softAPname;
         }
         else
         {
            root["status_wifiip"] = WiFi.localIP().toString();
            root["status_wifissid"] = WiFi.SSID();
         }

      root["status_dt"] = strDateTime(now());
      root["status_voltage"] = vcc;
      root["status_heap"] = heap;
      root["status_temp"] = "0";
      root["status_hum"] = "0";
      root["status_pres"] = "0";
      root["status_wifirssi"] = rssi;
   }

   void doRecv(int evt, JsonObject &iroot, JsonObject &root) {}

private:
   double vcc;
   uint32_t heap;
   int32_t rssi;
};

TestTask1 task1;


class TestTask2 : public EspTask
{
public:
   TestTask2() : EspTask() {}
   void doTask(int evt) {DbgPrintln(("DoTask2"));}
   void doSend(int evt, JsonObject &iroot, JsonObject &root)
   {
      DbgPrintln(("sendTask2"));
      root["status_vmode"] = "Automatic";
      root["status_vstatus"] = "Close";
   }

   void doRecv(int evt, JsonObject &iroot, JsonObject &root) {}
};

TestTask2 task2;