class Sensor
{
public:
   Sensor() {};
   virtual int init(){};
   virtual int begin(){};
   virtual int run(){};
   virtual int end(){};
   virtual String getName(){};
   virtual String getTag(int idx){};
   virtual String getValueAsStr(int idx){};
   virtual double getValueAsDbl(int idx){};
   virtual int    getValueAsInt(int idx){};
   virtual String getMqttPayload(int sens, int v) {}; // variants of mqtt payload 


   int    getTagCount(){ return tcnt;};
   bool   ok() {return f_ok;}
   bool   ready() {return f_ready;}
protected:
   int tcnt;
   bool f_ok;
   bool f_ready;
};
