class Sensor
{
public:
   Sensor() {};
   int init(){ return 0; };
   virtual int begin(){ return 0; };
   virtual int run(){ return 0; };
   virtual int end(){ return 0; };
   virtual String getName(){ return String(""); };
   virtual String getTag(int idx){ return String(""); };
   virtual String getValueAsStr(int idx){ return String(""); };
   virtual double getValueAsDbl(int idx){ return 0.0; };
   virtual int    getValueAsInt(int idx){ return 0; };
   virtual String getMqttPayload(int sens, int v) { return String(""); }; // variants of mqtt payload


   int    getTagCount(){ return tcnt;};
   bool   ok() { return f_ok; }
   bool   ready() { return f_ready; }
protected:
   int tcnt;
   bool f_ok;
   bool f_ready;
};
