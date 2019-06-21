#include <stdio.h>
#include <stdarg.h>

HardwareSerial *debug_port = &Serial;

HardwareSerial *getDebugPort() { return debug_port; }

void setDebugPort(int port, int baud)
{
   if(port) debug_port = &Serial1; else debug_port = &Serial;
   debug_port->begin(baud);
}
void _debugSend(char * message)
{
//   bool pause = false;

   #if DEBUG_ADD_TIMESTAMP
      static bool add_timestamp = true;
      char timestamp[10] = {0};
      if (add_timestamp) snprintf_P(timestamp, sizeof(timestamp), PSTR("[%06lu] "), millis() % 1000000);
      add_timestamp = (message[strlen(message)-1] == 10) || (message[strlen(message)-1] == 13);
   #endif

   #if DEBUG_ADD_TIMESTAMP
   debug_port->printf(timestamp);
   #endif
   debug_port->printf(message);
}

void debugSend(const char * format, ...)
{
   va_list args;
   va_start(args, format);
   char test[1];
   int len = ets_vsnprintf(test, 1, format, args) + 1;
   char * buffer = new char[len];
   ets_vsnprintf(buffer, len, format, args);
   va_end(args);
   _debugSend(buffer);
   delete[] buffer;
}

void debugSend_P(PGM_P format_P, ...)
{
   char format[strlen_P(format_P)+1];
   memcpy_P(format, format_P, sizeof(format));
   va_list args;
   va_start(args, format_P);
   char test[1];
   int len = ets_vsnprintf(test, 1, format, args) + 1;
   char * buffer = new char[len];
   ets_vsnprintf(buffer, len, format, args);
   va_end(args);
   _debugSend(buffer);
   delete[] buffer;
}

