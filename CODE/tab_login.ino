#include "tmrlib.h"
#include <MD5Builder.h>

MD5Builder _md5;

String md5(String str) { _md5.begin(); _md5.add(str); _md5.calculate(); return _md5.toString(); }

void handleLogin(AsyncWebServerRequest *request)
{
   String user; user = String(cfg.s.user);
   String pwd;  pwd = String(cfg.s.pwd);
   bool ok = true;

   String message = DbgArgMsg(request);

   if (request->hasHeader("Cookie")) { DbgPrint(("Found cookie: ")); DbgPrintln((request->header("Cookie"))); }

   if (request->hasArg("DISCONNECT"))
   {
      DbgPrintln(("Disconnection"));
      AsyncWebServerResponse *response = request->beginResponse(301); //Sends 404 File Not Found
      response->addHeader("Set-Cookie","ESPSESSIONID=0");
      response->addHeader("Location","/login");
      response->addHeader("Cache-Control","no-cache");
      request->send(response);      
      return;
   }

   if(request->hasArg("id"))
   {
      session_id = random(1000000);
      String xml;
      xml="<?xml version='1.0'?>";
      xml+="<xml>";
      xml+="<sessionid>" + String(session_id) + "</sessionid>";
      xml+="</xml>";

      request->send(200, "text/xml", xml);
      return;
   }
  
  
   //if (server.hasArg("USERNAME") && server.hasArg("PASSWORD"))
   if (request->hasArg("SHA"))
   {
     
      String calc = md5(user+pwd+String(session_id));
      DbgPrintln((calc));
      //if (server.arg("USERNAME") == user &&  server.arg("PASSWORD") == pwd )
      if(calc==request->arg("SHA"))
      {
         /*String content;
         content = "HTTP/1.1 301 OK\r\n";
         content += "Set-Cookie: ESPSESSIONID=" + String(session_id) + ";\r\n";
         content += "Location: /index1\r\nCache-Control: no-cache\r\n\r\n";
         server.sendContent(content);
         DbgPrintln((content));*/

         AsyncWebServerResponse *response = request->beginResponse(301); //Sends 404 File Not Found
         response->addHeader("Set-Cookie","ESPSESSIONID="+String(session_id));
         response->addHeader("Location","/index1");
         response->addHeader("Cache-Control","no-cache");
         request->send(response);      

         DbgPrintln(("Log in Successful"));
         return;
      }
      ok = false;
      DbgPrintln(("Log in Failed"));
   }
  
   String fn;
   if(ok) fn = "/login.htm"; else fn = "/logerr.htm";
   request->send(SPIFFS, fn);
   //size_t sz = sendFile(fn,"text/html");
   //DbgPrint(("Output size: ")); DbgPrintln((String(sz)));
   DbgPrintln((message));
}

void handleLogoff(AsyncWebServerRequest *request)
{
   DbgPrintln(("Disconnection"));
      AsyncWebServerResponse *response = request->beginResponse(301); //Sends 404 File Not Found
      response->addHeader("Set-Cookie","ESPSESSIONID=0");
      response->addHeader("Location","/login");
      response->addHeader("Cache-Control","no-cache");
      request->send(response);      

   //server.sendContent("HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n");
   return;      
}

