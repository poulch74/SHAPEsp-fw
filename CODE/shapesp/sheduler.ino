int TestSheduler(time_t ct, int mflag, bool skiptmr)
{
   int vstate=0;

   if(mflag==1) return ( 1); // открыть вручную
   if(mflag==2) return (-1); // закрыть вручную
   if(!skiptmr)
   {
      uint16_t tcur = (uint16_t)((ct-previousMidnight(ct))/60);
      
      uint8_t shift = (dayOfWeek(ct)-1) ? (dayOfWeek(ct)-2):6;
      uint8_t cdow = 1 << shift; // 0 based day of week 0 monday

      for(int i=0;i<10;i++)
         if((prg.ta.p[i].active)&&(tcur==prg.ta.p[i].on_ts)&&(cdow&prg.ta.p[i].on_dowmask)) { vstate = 1; break;}

      for(int i=0;i<10;i++)  
         if((prg.ta.p[i].active)&&(tcur==prg.ta.p[i].off_ts)&&(cdow&prg.ta.p[i].off_dowmask)) { vstate = -1; break;}
   }
   return vstate;      
}