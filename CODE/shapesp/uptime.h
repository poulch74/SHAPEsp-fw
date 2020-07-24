// uptime service
static time_t startup = 0;  // startup timestamp
static time_t uptime = 0;   // uptime store


//uptime service
time_t startUptime()
{
   startup = now();
   uptime = 0;
   return startup;
}

void setTimeUptime(time_t t)
{
  if(startup)
  {
    time_t st = now();
    if(st>t)
    { 
      time_t diff = st-t;
      startup -= diff;
    }
    else
    {
      time_t diff = t-st;
      startup += diff;
    }
    setTime(t);
  }  
}


void adjustTimeUptime(long adjustment) {
  startup += adjustment;
  adjustTime(adjustment);
}

time_t getUptime()
{
   uptime = now() - startup;
   return uptime;
}

