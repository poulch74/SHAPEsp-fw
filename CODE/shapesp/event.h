#define DEFINE_EVENT(id,num) const int id = num; EspEvent __evt##id(id,EVT_TASK);

#define EVENT_BEGIN_REGISTER_TASKS void EventRegisterTasks() {
#define EVENT_REGISTER_TASK(id,task) __evt##id.attach((EspTask *)&task);
#define EVENT_END_REGISTER_TASKS }

#define DEFINE_MSG(id,num) const int id = num; EspEvent __msg##id(id,EVT_MSG);

#define MSG_BEGIN_REGISTER_TASKS void MsgRegisterTasks() {
#define MSG_REGISTER_TASK(id,task) __msg##id.attach((EspTask *)&task);
#define MSG_END_REGISTER_TASKS }

#define MSG_BEGIN_SUBSCRIBE void MsgSubscribe() {
#define MSG_SUBSCRIBE(msg,id) msglist[msg] = (EspEvent *)&__msg##id;
#define MSG_END_SUBSCRIBE }

#define GetEvent(id) __evt##id
#define GetMsg(id)   __msg##id

#define EVT_TASK 1
#define EVT_MSG 2

class EspTask
{
public:
   EspTask(){}
   virtual void doTask(int evt) {}
   virtual void doWStask(int evt, JsonObject &iroot, JsonObject &root) {}
   virtual void doMqttTask(int evt, std::vector<String> &payload) {}
};

class EspEvent
{
   public:
      EspEvent(int id, int type) { _id = id; _type=type; }
      void attach(EspTask *task) { _tasks.push_back(task); }
      void doTasks() { for(uint32_t i = 0; i< _tasks.size();i++) _tasks[i]->doTask(_id); }
      void doTasks(std::vector<String> &payload) { for(uint32_t i = 0; i< _tasks.size();i++) { _tasks[i]->doMqttTask(_id, payload); } }
      void doTasks(JsonObject& iroot, JsonObject& root) { for(uint32_t i = 0; i< _tasks.size();i++) _tasks[i]->doWStask(_id, iroot, root); }

   public:
      int _id;
      int _type;
      std::vector<EspTask *> _tasks;
};