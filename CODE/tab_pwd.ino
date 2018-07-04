#include "tmrlib.h"

// смена пароля
void handleSecurity()
{
   bool ok = true;
   DbgPrintln(("Enter handleSecurity"));
   if (!is_auth()) { UrlRedirect("/login"); return; }

   if (server.hasArg("USERNAME") && server.hasArg("PASSWORD1") && server.hasArg("PASSWORD2"))
   {
      if (server.arg("PASSWORD1") == server.arg("PASSWORD2") )
      {
         // сохраняем новые в eeprom 
         ReadConfig();
         snprintf(cfg.s.user,21,server.arg("USERNAME").c_str());
         snprintf(cfg.s.pwd,21,server.arg("PASSWORD1").c_str());
         WriteConfig(false);
         handleLogoff();
         return;      
      }
      ok = false;
      DbgPrintln(("pwd change failed"));
   }

   String fn;
   if(ok) fn = "/pwd.htm"; else fn = "/pwderr.htm";
   size_t sz = sendFile(fn,"text/html");   
   DbgPrint(("Output size: ")); DbgPrintln((String(sz)));
}
