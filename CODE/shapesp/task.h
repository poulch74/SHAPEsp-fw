#define DECLARE_TASK(classname,name,evt) classname name(evt);
#define BEGIN_REGISTER_TASKS \
               std::vector<EspTask *> _tasks; \
               void RegisterTasks() {
#define REGISTER_TASK(name) _tasks.push_back((EspTask *)&name);
#define END_REGISTER_TASKS }



class EspTask
{
public:
   EspTask(int evt) { event = evt; }
   virtual void doTask(int evt) {}
   virtual void doSend(JsonObject &root) {}
   virtual void doRecv(JsonObject &root) {}

private:
   int event;
};

class TestTask1 : public EspTask
{
public:
   TestTask1(int evt) : EspTask(evt) {}
   void doTask(int evt) {DbgPrintln(("DoTask1"));}
   void doSend(JsonObject &root) {}
   void doRecv(JsonObject &root) {}
};

class TestTask2 : public EspTask
{
public:
   TestTask2(int evt) : EspTask(evt) {}
   void doTask(int evt) {DbgPrintln(("DoTask2"));}
   void doSend(JsonObject &root) {}
   void doRecv(JsonObject &root) {}
};
