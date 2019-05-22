#include <MD5Builder.h>

MD5Builder _md5;

String md5(String str) { _md5.begin(); _md5.add(str); _md5.calculate(); return _md5.toString(); }

String hash;
bool b_isauth=false;
bool isauth() { return b_isauth; }

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
   if(type == WS_EVT_CONNECT)
   {
      DEBUG_MSG_P(PSTR("ws[%s][%u] connect\n"), server->url(), client->id());
      client->ping();
      client->_tempObject = new WebSocketIncommingBuffer(&wsParseHandler, true); // буфер для принятого сообщения
   }
   else if(type == WS_EVT_DISCONNECT)
   {
      DEBUG_MSG_P(PSTR("ws[%s][%u] disconnect: %u\n"), server->url(), client->id());
      if(client->_tempObject) delete (WebSocketIncommingBuffer *)client->_tempObject;
   }
   else if(type == WS_EVT_ERROR)
   {
      DEBUG_MSG_P(PSTR("ws[%s][%u] error(%u): %s\n"), server->url(), client->id(), *((uint16_t*)arg), (char*)data);
   }
   else if(type == WS_EVT_PONG)
   {
      DEBUG_MSG_P(PSTR("ws[%s][%u] pong[%u]: %s\n"), server->url(), client->id(), len, (len)?(char*)data:"");
   }
   else if(type == WS_EVT_DATA)
   {
        WebSocketIncommingBuffer *buffer = (WebSocketIncommingBuffer *)client->_tempObject;
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        buffer->data_event(client, info, data, len);
   }
}

void wsParseHandler(AsyncWebSocketClient *client, uint8_t * payload, size_t length)
{
   // Get client ID
   uint32_t client_id = client->id();

   // Parse JSON input
   DynamicJsonBuffer inputBuffer;
   JsonObject& iroot = inputBuffer.parseObject((char *) payload);
   if (!iroot.success()) { DEBUG_MSG_P(PSTR("[WEBSOCKET] Error parsing data\n")); return; }

   DynamicJsonBuffer outBuffer;
   JsonObject& oroot = outBuffer.createObject();

   if(iroot["type"].as<String>() =="message")
   {
      if(!HandleStatus(iroot,oroot)) { client->close(1003); return; };

      size_t len = oroot.measureLength();
      AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
      if (buffer)
      {
         oroot.printTo((char *)buffer->get(), len + 1);
         if (client) client->text(buffer);
         else ws.textAll(buffer);
      }
   }
}

bool HandleStatus(JsonObject& iroot, JsonObject& root)
{
   if(iroot["text"].as<String>()=="sessionid")
   {
      root["action"] = "auth";
      hash = md5(String(cfg.wifi.user)+String(cfg.wifi.pwd)+iroot["data"].as<String>());
      DEBUG_MSG_P(PSTR("Hash server: %s \n"), hash.c_str());
      DEBUG_MSG_P(PSTR("Hash client: %s \n"), iroot["auth"].as<String>().c_str());
      if(iroot["auth"].as<String>() == hash) { b_isauth=true; root["status_auth"] = "ok"; DEBUG_MSG_P(PSTR("Auth OK\n"));}
      else { b_isauth=false; root["status_auth"] = "fail";  DEBUG_MSG_P(PSTR("Auth FAIL\n")); }
      return true;
   }

   // reject no auth requests
   if(iroot["auth"].as<String>() != hash) { DEBUG_MSG_P(PSTR("No auth!!!!\n")); return false; }


   String t = iroot["text"].as<String>();
   if(msglist.find(t) != msglist.end())  msglist.at(t)->doTasks(iroot,root);
   return true;
}
