#define DECLARE_EVENT(name,evt) EspEvent name(evt);

#define EVENT_BEGIN_REGISTER_TASKS void EventRegisterTasks() {
#define EVENT_REGISTER_TASK(name,task) name.attach((EspTask *)&task);
#define EVENT_END_REGISTER_TASKS }

class EspEvent
{
   public:
      EspEvent(int eid) { id = eid;}
      void attach(EspTask *task) { _tasks.push_back(task); }
      void doTasks()
      { 
         for(int i = 0; i< _tasks.size();i++)
         {
            _tasks[i]->doTask(id);
            delay(20);
         }
      }
   public:
      int id;
      std::vector<EspTask *> _tasks;
};