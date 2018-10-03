#include "tmrlib.h"

// смена пароля

void handleSecurity(AsyncWebServerRequest *request)
{
   bool ok = true;
   DbgPrintln(("Enter handleSecurity"));

   if(!is_auth(request)) { request->redirect("/login"); return; }

   if (request->hasArg("USERNAME") && request->hasArg("PASSWORD1") && request->hasArg("PASSWORD2"))
   {
      if (request->arg("PASSWORD1") == request->arg("PASSWORD2") )
      {
         // сохраняем новые в eeprom 
         ReadConfig();
         snprintf(cfg.s.user,21,request->arg("USERNAME").c_str());
         snprintf(cfg.s.pwd,21,request->arg("PASSWORD1").c_str());
         WriteConfig(false);
         handleLogoff(request);
         return;      
      }
      ok = false;
      DbgPrintln(("pwd change failed"));
   }

   String fn;
   if(ok) fn = "/pwd.htm"; else fn = "/pwderr.htm";
   request->send(SPIFFS, fn,"text/html");
   //size_t sz = sendFile(fn,"text/html");   
   //DbgPrint(("Output size: ")); DbgPrintln((String(sz)));
}
