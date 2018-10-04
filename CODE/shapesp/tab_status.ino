//#include "tmrlib.h"

void handleIndex1(AsyncWebServerRequest *request)
{
   DbgPrintln(("Enter handle status"));
   String message = DbgArgMsg(request);

   do
   { 
      if(!is_auth(request)) { request->redirect("/login"); break; }

      if(request->hasArg("status"))
      {
         String xml;
         xml="<?xml version='1.0'?>";
         xml+="<xml>";
         xml+="<datetime>" + strDateTime(now()) + "</datetime>";
         xml+="<temp>" + String(temp) + "</temp>";
         xml+="<hum>" + String(hum) + "</hum>";
         xml+="<pres>" + String(pres*25.4) + "</pres>";
         xml+="<voltage>" + String(volt) + "</voltage>";
         xml+="<vstatus>" + String((curstate ? "Open":"Close")) + "</vstatus>";
         xml+="<vmode>" + String((skiptmr ? "Manual":"Automatic")) + "</vmode>";
         xml+="<wifimode>" + String((wifimode ? "SoftAP":"Station")) + "</wifimode>";
         if(wifimode)
         {
            xml+="<ip1>" + WiFi.softAPIP().toString()+ "</ip1>";
            xml+="<ssid>" + softAPname + "</ssid>";
         }
         else
         {
            xml+="<ip1>" + WiFi.localIP().toString()+ "</ip1>";
            xml+="<ssid>" + WiFi.SSID() + "</ssid>";
         }
         xml+="<rssi>" + String(WiFi.RSSI()) + "</rssi>";
         xml+="</xml>";
         request->send(200, "text/xml", xml);
         break;
      }

      if(request->hasArg("valve"))
      {
         DbgPrintln(("On/Off"));
         if(request->hasArg("action"))
         {
            int action = request->arg("action").toInt();
            if(action==1) mflag = 1; //on
            if(action==0) mflag = 2; //off
            if(action==2) mflag = 3; // auto
         }      
         request->redirect("/index1");
         break;
      }

      request->send(SPIFFS, "/status.htm","text/html");
   } while(0);

   DbgPrintln((message));  
}
 