#define DEFINE_EVENT(id,num) const int id = num; EspEventPtr __evt##id(id,EVT_TASK);

#define EVENT_BEGIN_REGISTER_TASKS void EventRegisterTasks() {
#define EVENT_REGISTER_TASK(id,task,enable) if(enable) { __evt##id.attach((EspTask *)&task); }
#define EVENT_END_REGISTER_TASKS }

#define DEFINE_MSG(id,num) const int id = num; EspEventPtr __msg##id(id,EVT_MSG);

#define MSG_BEGIN_REGISTER_TASKS void MsgRegisterTasks() {
#define MSG_REGISTER_TASK(id,task,enable) if(enable) { __msg##id.attach((EspTask *)&task); }
#define MSG_END_REGISTER_TASKS }

#define MSG_BEGIN_SUBSCRIBE void MsgSubscribe() {
#define MSG_SUBSCRIBE(msg,id) msglist[msg] = (EspEventPtr *)&__msg##id;
#define MSG_END_SUBSCRIBE }

#define GetEvent(id) __evt##id
#define GetMsg(id)   __msg##id

#define EVT_TASK 1
#define EVT_MSG 2

class EspTask
{
public:
   EspTask(){}
   virtual void doTask(int evt, void *data) {}
   virtual void doWStask(int evt, JsonObject &iroot, JsonObject &root) {}
   virtual void doMqttTask(int evt, std::vector<String> &payload) {}
};

class EspEventPtr
{
   public:
      EspEventPtr(int id, int type) { _id = id; _type=type; }
      void attach(EspTask *task) { _tasks.push_back(task); }

      void doTasks(void *data) { for(auto i = 0; i< _tasks.size();i++) _tasks[i]->doTask(_id, data); }

      void doTasks(std::vector<String> &payload) { for(auto i = 0; i< _tasks.size();i++) { _tasks[i]->doMqttTask(_id, payload); } }

      void doTasks(JsonObject& iroot, JsonObject& root) { for(auto i = 0; i< _tasks.size();i++) _tasks[i]->doWStask(_id, iroot, root); }

   public:
      int _id;
      int _type;
      std::vector<EspTask *> _tasks;
};

struct EspEvent
{
   EspEventPtr *evt;
   void *data;
   EspEvent(EspEventPtr *e, void *d = nullptr): evt(e),data(d) {}
};
