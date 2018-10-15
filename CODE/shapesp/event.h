#define DECLARE_EVENT(id) EspEvent __evt##id(id,EVT_TASK,"");

#define EVENT_BEGIN_REGISTER_TASKS void EventRegisterTasks() {
#define EVENT_REGISTER_TASK(id,task) __evt##id.attach((EspTask *)&task);
#define EVENT_END_REGISTER_TASKS }


#define DECLARE_MSG(id,type,msg) EspEvent __msg##id(id,type,msg);

#define MSG_BEGIN_REGISTER_TASKS void MsgRegisterTasks() {
#define MSG_REGISTER_TASK(id,task) __msg##id.attach((EspTask *)&task);
#define MSG_END_REGISTER_TASKS }

#define MSG_BEGIN_STORE_VECTOR void MsgStoreVector() {
#define MSG_STORE(id) msglist.push_back((EspEvent *)&__msg##id);
#define MSG_END_STORE_VECTOR }



#define EVT_TASK 1
#define EVT_SEND 2
#define EVT_RECV 3

class EspEvent
{
   public:
      EspEvent(int id, int type, const char *msg) { _id = id; _type=type; _msg=String(msg);}
      void attach(EspTask *task) { _tasks.push_back(task); }
      bool doTasks()
      {
         for(int i = 0; i< _tasks.size();i++) _tasks[i]->doTask(_id);
         return true;
      }
      bool doTasks(JsonObject& iroot, JsonObject& root)
      {
         if(_type == EVT_TASK)
         {
            for(int i = 0; i< _tasks.size();i++) _tasks[i]->doTask(_id);
         }
         else
         {
            if(iroot["text"].as<String>() == _msg)
            {
               switch(_type)
               {
                  case EVT_SEND: { for(int i = 0; i< _tasks.size();i++) _tasks[i]->doSend(_id, iroot, root); } break;
                  case EVT_RECV: { for(int i = 0; i< _tasks.size();i++) _tasks[i]->doRecv(_id, iroot, root); }
               }
            }
            else return false;
         }
         return true;
      }

      String getMsg() { return _msg; }
      
   public:
      int _id;
      int _type;
      String _msg;
      std::vector<EspTask *> _tasks;
};