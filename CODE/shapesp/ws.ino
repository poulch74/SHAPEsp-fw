
void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
   if(type == WS_EVT_CONNECT)
   {
      Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
      client->ping();
      // send start data
      client->_tempObject = new WebSocketIncommingBuffer(&wsParseHandler, true); // буфер для принятого сообщения
   }
   else if(type == WS_EVT_DISCONNECT)
   {
      Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
      if(client->_tempObject) delete (WebSocketIncommingBuffer *)client->_tempObject;
   }

   else if(type == WS_EVT_ERROR)
   {
      Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
   }
   else if(type == WS_EVT_PONG)
   {
      Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
   }
   else if(type == WS_EVT_DATA)
   {
        WebSocketIncommingBuffer *buffer = (WebSocketIncommingBuffer *)client->_tempObject;
        AwsFrameInfo * info = (AwsFrameInfo*)arg;
        buffer->data_event(client, info, data, len);
   }
}
