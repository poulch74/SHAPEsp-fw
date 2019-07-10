#include <stdio.h>
#include <stdarg.h>

#ifndef DEBUG_UDP_SUPPORT
   #define DEBUG_UDP_SUPPORT       1               // Enable UDP debug log
#endif

#ifndef DEBUG_UDP_IP
   #define DEBUG_UDP_IP            IPAddress(192, 168, 137, 1)
#endif

#ifndef DEBUG_UDP_PORT
   #define DEBUG_UDP_PORT          514
#endif

//------------------------------------------------------------------------------
// UDP SYSLOG
//------------------------------------------------------------------------------

// Priority codes:
#define SYSLOG_EMERG       0 /* system is unusable */
#define SYSLOG_ALERT       1 /* action must be taken immediately */
#define SYSLOG_CRIT        2 /* critical conditions */
#define SYSLOG_ERR         3 /* error conditions */
#define SYSLOG_WARNING     4 /* warning conditions */
#define SYSLOG_NOTICE      5 /* normal but significant condition */
#define SYSLOG_INFO        6 /* informational */
#define SYSLOG_DEBUG       7 /* debug-level messages */

// Facility codes:
#define SYSLOG_KERN        (0<<3)  /* kernel messages */
#define SYSLOG_USER        (1<<3)  /* random user-level messages */
#define SYSLOG_MAIL        (2<<3)  /* mail system */
#define SYSLOG_DAEMON      (3<<3)  /* system daemons */
#define SYSLOG_AUTH        (4<<3)  /* security/authorization messages */
#define SYSLOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define SYSLOG_LPR         (6<<3)  /* line printer subsystem */
#define SYSLOG_NEWS        (7<<3)  /* network news subsystem */
#define SYSLOG_UUCP        (8<<3)  /* UUCP subsystem */
#define SYSLOG_CRON        (9<<3)  /* clock daemon */
#define SYSLOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define SYSLOG_FTP         (11<<3) /* ftp daemon */
#define SYSLOG_LOCAL0      (16<<3) /* reserved for local use */
#define SYSLOG_LOCAL1      (17<<3) /* reserved for local use */
#define SYSLOG_LOCAL2      (18<<3) /* reserved for local use */
#define SYSLOG_LOCAL3      (19<<3) /* reserved for local use */
#define SYSLOG_LOCAL4      (20<<3) /* reserved for local use */
#define SYSLOG_LOCAL5      (21<<3) /* reserved for local use */
#define SYSLOG_LOCAL6      (22<<3) /* reserved for local use */
#define SYSLOG_LOCAL7      (23<<3) /* reserved for local use */


// If DEBUG_UDP_PORT is set to 514 syslog format is assumed
// (https://tools.ietf.org/html/rfc3164)
// DEBUG_UDP_FAC_PRI is the facility+priority
#define DEBUG_UDP_FAC_PRI       (SYSLOG_LOCAL0 | SYSLOG_DEBUG)

#if DEBUG_UDP_SUPPORT
#include <WiFiUdp.h>
WiFiUDP _udp_debug;
#if DEBUG_UDP_PORT == 514
char _udp_syslog_header[40] = {0};
#endif
#endif

std::queue<String> dbgqueue; // очередь dbg
bool rec = false;
bool play = false;

HardwareSerial *debug_port = &Serial;

HardwareSerial *getDebugPort() { return debug_port; }

void setDebugPort(int port, int baud)
{
   if(port) debug_port = &Serial1; else debug_port = &Serial;
   debug_port->begin(baud);
}

void debugRecStart()
{
   #if DEBUG_UDP_SUPPORT
   #if DEBUG_UDP_PORT == 514
      snprintf_P(_udp_syslog_header, sizeof(_udp_syslog_header),
                 PSTR("<%u>%s : "), DEBUG_UDP_FAC_PRI, cfg.wifi.hostname);
   #endif
   #endif
   rec = true;
}

void debugRecStop()  { rec = false; }

void debugRecSend()
{
   play = true;
   while(!dbgqueue.empty())
   {
      _debugSend((char *)dbgqueue.front().c_str());
      dbgqueue.pop();
      delay(2);
   }
   play = false;
}


void _debugSend(char * message)
{
   #if DEBUG_ADD_TIMESTAMP
      static bool add_timestamp = true;
      char timestamp[10] = {0};
      if (add_timestamp) snprintf_P(timestamp, sizeof(timestamp), PSTR("[%06lu] "), millis() % 1000000);
      add_timestamp = (message[strlen(message)-1] == 10) || (message[strlen(message)-1] == 13);
   #endif

   #if DEBUG_UDP_SUPPORT
      if(rec)
      {
         dbgqueue.push(String(message));
      }
      else
      {
         _udp_debug.beginPacket(DEBUG_UDP_IP, DEBUG_UDP_PORT);
         #if DEBUG_UDP_PORT == 514
            _udp_debug.write(_udp_syslog_header);
         #endif
         _udp_debug.write(message);
         _udp_debug.endPacket();
         if(play) return;
         optimistic_yield(100);
      }
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

