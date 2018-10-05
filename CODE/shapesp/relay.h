#define R_VALVE 0
#define R_RELAY 1

class Relay;

void alarm1(Relay *r);

class Relay
{
public:
   Relay(int p1, int p2, int p3, int mode, int stop)
   {
      rmode = mode; // valve relay
      autostop = stop;
      curstate = 0;
      pinA1 = p1; pinA2 = p2; pinSTBY = p3;
   }      

   void Initialize() // блокирующая инициализация
   {
      pinMode(pinA1, OUTPUT); digitalWrite(pinA1, LOW);
      pinMode(pinA2, OUTPUT); digitalWrite(pinA2, LOW);
      if(autostop) { pinMode(pinSTBY, OUTPUT); digitalWrite(pinSTBY, LOW); }

       // must set valve off
      if(rmode==R_VALVE)
      {
         digitalWrite(pinA2, HIGH);
         if(autostop) { digitalWrite(pinSTBY, HIGH); delay(300); digitalWrite(pinSTBY, LOW); }
         delay(5000);
         digitalWrite(pinA2, LOW);
      }      
      
   }

   int SetState(int state)
   {
      if(state!=curstate)
      {
         int a1,a2=a1=LOW;

         if(rmode==R_VALVE)
         {
            timer.once_ms(5100, alarm1, this);
            if(state==1) { a1=HIGH; a2=LOW;  DbgPrintln(("on valve start")); }
            if(state==0) { a1=LOW;  a2=HIGH; DbgPrintln(("off valve start"));}
         }
         else
         {
            if(state==1) { a1=HIGH; a2=LOW;  DbgPrintln(("on relay"));  }
            if(state==0) { a1=LOW;  a2=LOW;  DbgPrintln(("off relay")); }
         }               
        
         digitalWrite(pinA1, a1);
         digitalWrite(pinA2, a2);
         if(autostop) { digitalWrite(pinSTBY, HIGH); delay(300); digitalWrite(pinSTBY, LOW);}//timerSTBY.once_ms(300,STBYoff); }// timer off

         curstate = state;
      }
      return curstate;
   }

   int GetState() { return curstate;}
   void SetMode(int mode) {rmode = mode;}

public:   
   int rmode; // valve relay
   int autostop;
   int curstate;
   int pinA1;
   int pinA2;
   int pinSTBY;
   Ticker timer;

//friend void alarm1(Relay *r);
};

void alarm1(Relay *r)
{ 
   if(r->rmode==R_VALVE)
   { 
      digitalWrite(r->pinA1, LOW);
      digitalWrite(r->pinA2, LOW);
   }
   DbgPrintln(("OFF relay"));
}
