#include <Wiegand.h>

class Task_WG : public EspTask
{
public:
   Task_WG() : EspTask() {  wg.begin(12,13); }

   void doTask(int evt, void *data)
   {
      if(wg.available())
      {
		   DEBUG_MSG_P(PSTR("Wiegand HEX = %x \n"),wg.getCode());
         DEBUG_MSG_P(PSTR("Wiegand DEC = %d \n"),wg.getCode());
         DEBUG_MSG_P(PSTR("Wiegand Type = %d \n"),wg.getWiegandType());

         if(wg.getCode() == 14208388 )
         {
            sysqueue.push(EspEvent(&GetEvent(EVT_VOPEN),new String("open")));
            DEBUG_MSG_P(PSTR("WG SCHEDULE OPEN\n"));
         }
      }
   }

   void doMqttTask(int evt, std::vector<String> &payload)
   {
   }

   void doWStask(int evt, JsonObject &iroot, JsonObject &root)
   {
   }

private:
   WIEGAND wg;
};

Task_WG task_wg;
