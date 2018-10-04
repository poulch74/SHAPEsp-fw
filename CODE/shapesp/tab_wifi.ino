//#include "tmrlib.h"

void handleWiFiSettings(AsyncWebServerRequest *request)
{
   DbgPrintln(("Enter handle wifi set"));
   String message = DbgArgMsg(request);

   if(!is_auth(request)) { request->redirect("/login"); return; }
  
   if(request->hasArg("wifiset"))
   {
      int act = request->arg("wifiset").toInt();
      if(act==1)
      {
         if(request->hasArg("ssid")) snprintf(cfg.s.sta_ssid, 33 ,request->arg("ssid").c_str());
         if(request->hasArg("pwd")) snprintf(cfg.s.sta_pwd,65, request->arg("pwd").c_str());
         if(request->hasArg("dhcp")) { cfg.s.sta_dhcp = request->arg("dhcp").toInt();}
         IPAddress ip;
         if(request->hasArg("vip")) { ip.fromString(request->arg("vip")); for(int i=0;i<4; i++) cfg.s.sta_ip[i] = ip[i]; }
         if(request->hasArg("vgw")) { ip.fromString(request->arg("vgw")); for(int i=0;i<4; i++) cfg.s.sta_gw[i] = ip[i]; }
         if(request->hasArg("vmask"))
         { 
            int m = 32-(request->arg("vmask").toInt())&0x1F;
            uint32_t ma = 0xFFFFFFFF<<m;
            cfg.s.sta_subnet[0] = (ma>>24)&0xFF;
            cfg.s.sta_subnet[1] = (ma>>16)&0xFF;
            cfg.s.sta_subnet[2] = (ma>>8)&0xFF;
            cfg.s.sta_subnet[3] = ma&0xFF;
            String s; s = String(cfg.s.sta_subnet[0])+'.'+String(cfg.s.sta_subnet[1])+'.'+String(cfg.s.sta_subnet[2])+'.'+String(cfg.s.sta_subnet[3]);
            DbgPrintln(("NetMask: "));
            DbgPrintln((s));
         }

         if(request->hasArg("tnet")) { cfg.s.skip_logon = request->arg("tnet").toInt();}
  
         WriteConfig(false);
         DbgPrintln(("write config"));
      }


      uint32_t ma = (((uint32_t)cfg.s.sta_subnet[0])<<24) + (((uint32_t)cfg.s.sta_subnet[1])<<16)
                       +(((uint32_t)cfg.s.sta_subnet[2])<<8) + cfg.s.sta_subnet[3];
      int m=0; while(ma!=0) { ma<<=1; m++; };
      
      String content;  
      content += "wifiset";
      content += "," + String(cfg.s.sta_ssid);
      content += "," + String(cfg.s.sta_pwd);
      content += "," + String(cfg.s.sta_dhcp);
      content += "," + IPAddress(cfg.s.sta_ip[0],cfg.s.sta_ip[1],cfg.s.sta_ip[2],cfg.s.sta_ip[3]).toString();
      content += "," + IPAddress(cfg.s.sta_gw[0],cfg.s.sta_gw[1],cfg.s.sta_gw[2],cfg.s.sta_gw[3]).toString();
      content += "," + String(m);//IPAddress(cfg.s.sta_subnet[0],cfg.s.sta_subnet[1],cfg.s.sta_subnet[2],cfg.s.sta_subnet[3]).toString();
      content += "," + String(cfg.s.skip_logon);
      request->send(200, "text/plain", content);    
   }
  
   request->send(SPIFFS, "/wifi.htm","text/html");
   //size_t sz = sendFile("/wifi.htm","text/html");   
   //DbgPrint(("Output size: ")); DbgPrintln((String(sz)));
   DbgPrintln((message));  
}
  