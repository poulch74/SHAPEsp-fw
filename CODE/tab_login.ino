#include "tmrlib.h"
#include <MD5Builder.h>

MD5Builder _md5;

String md5(String str) { _md5.begin(); _md5.add(str); _md5.calculate(); return _md5.toString(); }

void handleLogin()
{
   String user; user = String(cfg.s.user);
   String pwd;  pwd = String(cfg.s.pwd);
   bool ok = true;

   String message = DbgArgMsg();

   if (server.hasHeader("Cookie")) { DbgPrint(("Found cookie: ")); DbgPrintln((server.header("Cookie"))); }

   if (server.hasArg("DISCONNECT"))
   {
      DbgPrintln(("Disconnection"));
      server.sendContent("HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0;\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n");
      return;
   }

   if(server.hasArg("id"))
   {
      session_id = random(1000000);
      String xml;
      xml="<?xml version='1.0'?>";
      xml+="<xml>";
      xml+="<sessionid>" + String(session_id) + "</sessionid>";
      xml+="</xml>";
      server.send(200, "text/xml", xml);
      return;
   }
  
  
   //if (server.hasArg("USERNAME") && server.hasArg("PASSWORD"))
   if (server.hasArg("SHA"))
   {
     
      String calc = md5(user+pwd+String(session_id));
      DbgPrintln((calc));
      //if (server.arg("USERNAME") == user &&  server.arg("PASSWORD") == pwd )
      if(calc==server.arg("SHA"))
      {
         String content;
         content = "HTTP/1.1 301 OK\r\n";
         content += "Set-Cookie: ESPSESSIONID=" + String(session_id) + ";\r\n";
         content += "Location: /index1\r\nCache-Control: no-cache\r\n\r\n";
         server.sendContent(content);
         DbgPrintln((content));
         DbgPrintln(("Log in Successful"));
         return;
      }
      ok = false;
      DbgPrintln(("Log in Failed"));
   }
  
   String fn;
   if(ok) fn = "/login.htm"; else fn = "/logerr.htm";
   size_t sz = sendFile(fn,"text/html");
   DbgPrint(("Output size: ")); DbgPrintln((String(sz)));
   DbgPrintln((message));
}

void handleLogoff()
{
   DbgPrintln(("Disconnection"));
   server.sendContent("HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n");
   return;      
}

