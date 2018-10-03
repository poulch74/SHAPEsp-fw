#include "tmrlib.h"

String DbgArgMsg(AsyncWebServerRequest *request)
{
   String message;
   message += "URI: " + request->url();
   message += "\nMethod: " + String((request->method() == HTTP_GET)?"GET":"POST");
   message += "\nArguments: " + String(request->args()) + "\n";
   for (uint8_t i=0; i<request->args(); i++)
   {
      message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
   }
   return message;  
}

uint16_t crc16(const uint8_t *msg, int msg_len)
{
   uint16_t crc = 0;
   const uint16_t crc_poly = 0x1021;         /* порождающий многочлен */
   while (msg_len-- > 0)
   {
      crc ^= (uint16_t)*msg++ << 8;
      for (uint8_t j = 8; j > 0; j--) { if (crc&0x8000) crc = crc<<1^crc_poly; else crc<<= 1; }
   }
   return crc;
}

void handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}


void handleFavicon(AsyncWebServerRequest *request)
{
   request->send(SPIFFS, "/favicon.ico","image/ico");
}


bool is_auth(AsyncWebServerRequest *request)
{
    /*
   if(cfg.s.skip_logon)
   {
      do
      {
         IPAddress remip = server.client().remoteIP();
         DbgPrint(("CONNECT FROM : ")); DbgPrintln((remip.toString()));

         IPAddress locip = server.client().localIP();
         DbgPrint(("My IP : ")); DbgPrintln((locip.toString()));

         IPAddress gw(cfg.s.sta_gw[0],cfg.s.sta_gw[1],cfg.s.sta_gw[2],cfg.s.sta_gw[3]);
         IPAddress mask(cfg.s.sta_subnet[0],cfg.s.sta_subnet[1],cfg.s.sta_subnet[2],cfg.s.sta_subnet[3]);
//         if((remip[0]==gw[0])&&(remip[1]==gw[1])&&(remip[2]==gw[2])&&(remip[3]==gw[3]))
//         {
//            break;
//         }
//         else
         {
            for(int i=0;i<4;i++) {locip[i]&=mask[i]; remip[i]&=mask[i];}
            if((remip[0]==locip[0])&&(remip[1]==locip[1])&&(remip[2]==locip[2])&&(remip[3]==locip[3]))
            return true; // skip logon
         }
      } while(0);         
      DbgPrint(("Remote client!!!"));
   }
*/

   DbgPrintln(("Enter is_authentified"));
   if (request->hasHeader("Cookie")){   
      DbgPrint(("Found cookie: "));
      String cookie = request->header("Cookie");
      DbgPrintln((cookie));
      if (cookie.indexOf("ESPSESSIONID=") != -1) {
         String si = cookie.substring(cookie.indexOf('=')+1);
         long lsi = si.toInt();
         if(lsi==session_id)
         {
            DbgPrintln(("Authentification Successful"));
            return true;
         }
      }
   }
   DbgPrintln(("Authentification Failed"));
   return false; 
}

bool ReadConfig()
{
  memset(&cfg,0,sizeof(ESP_CONFIG));
  EEPROM.begin(4096);
  for(uint16_t i=0; i<sizeof(ESP_CONFIG); i++) cfg.b[i] = EEPROM.read(i);//rtc.eeprom_read(i);
  EEPROM.end();    
  uint16_t c_crc = crc16(&(cfg.b[2]),sizeof(ESP_CONFIG)-2);
  if(cfg.s.crc!=c_crc) return false;
  return true;
}

void WriteConfig(bool def)
{
   if(def)
   {
      memset(&cfg,0,sizeof(ESP_CONFIG));

      snprintf(cfg.s.user,21,"root");
      snprintf(cfg.s.pwd,21,"esp8266");

      cfg.s.wifi_mode = 0; // 0 ap 1 sta+ap 2 sta
      snprintf(cfg.s.sta_ssid,33,"HomeAP");
      snprintf(cfg.s.sta_pwd,65,"HomeAPpwd");
      cfg.s.sta_dhcp = 1;
      cfg.s.skip_logon = 0;
      cfg.s.sta_ip[0] = 192; cfg.s.sta_ip[1] = 168; cfg.s.sta_ip[2] = 0; cfg.s.sta_ip[3] = 10;
      cfg.s.sta_gw[0] = 192; cfg.s.sta_gw[1] = 168; cfg.s.sta_gw[2] = 0; cfg.s.sta_gw[3] = 1;
      cfg.s.sta_subnet[0] = 255; cfg.s.sta_subnet[1] = 255; cfg.s.sta_subnet[2] = 255; cfg.s.sta_subnet[3] = 0;
      sprintf(cfg.s.ap_ssid,"");
      sprintf(cfg.s.ap_pwd,"");
      cfg.s.ap_hidden = 0; // 1 - hidden
      cfg.s.ap_chan = 6;
      cfg.s.ap_ip[0] = 192; cfg.s.ap_ip[1] = 168; cfg.s.ap_ip[2] = 4; cfg.s.ap_ip[3] = 1;
      cfg.s.ap_gw[0] = 192; cfg.s.ap_gw[1] = 168; cfg.s.ap_gw[2] = 4; cfg.s.ap_gw[3] = 1;
      cfg.s.ap_subnet[0] = 255; cfg.s.ap_subnet[1] = 255; cfg.s.ap_subnet[2] = 255; cfg.s.ap_subnet[3] = 0;
   }    
   cfg.s.crc = crc16(&(cfg.b[2]),sizeof(ESP_CONFIG)-2);
   EEPROM.begin(4096);  
   for(uint16_t i=0; i<sizeof(ESP_CONFIG); i++) { EEPROM.write(i,cfg.b[i]);/*rtc.eeprom_write(i,cfg.b[i]);*/ }
   EEPROM.end();
}

bool ReadTmrPrg()
{
   memset(&prg,0,sizeof(ESP_TPRG));
   EEPROM.begin(4096);
   for(uint16_t i=0; i<sizeof(ESP_TPRG); i++) prg.b[i] = EEPROM.read(512+i);//rtc.eeprom_read(512+i);
   EEPROM.end();    
   uint16_t c_crc = crc16(&(prg.b[2]),sizeof(ESP_TPRG)-2);
   if(prg.ta.crc!=c_crc) return false;
   return true;
}

void SaveTmrPrg(bool def)
{
   if(def) { memset(&prg,0,sizeof(ESP_TPRG)); }    
   prg.ta.crc = crc16(&(prg.b[2]),sizeof(ESP_TPRG)-2);
   EEPROM.begin(4096);    
   for(uint16_t i=0; i<sizeof(ESP_TPRG); i++) { EEPROM.write(512+i,prg.b[i]);/*rtc.eeprom_write(512+i,prg.b[i]);*/ }
   EEPROM.end();
}
