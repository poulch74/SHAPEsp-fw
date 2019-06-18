/*
 EEPROM24 class like EEPROM simulation for ESP8266
*/

#ifndef EEPROM24_hr
#define EEPROM24_h

#include <stddef.h>
#include <stdint.h>
#include <string.h>

class EEPROM24Class {
public:
   EEPROM24Class(uint8_t addr, int pgsz, int pgcnt)
   : _data(0),
     _sizepg(0),
     _pgsz(pgsz),
     _pgcnt(pgcnt),
     _startp(0),
     _dirty(false)
   {
      _fsize = _pgsz*_pgcnt;
   }


   void begin(size_t startp size_t sizepg)
   {
      if (sizepg <= 0) return;
      if (startp>(_pgcnt-1)) return;
      size_t maxsize = (_pgsz-startp);
      if (sizepg > maxsize) sizepg = maxsize;

     //In case begin() is called a 2nd+ time, don't reallocate if size is the same
      if(_data && sizepg!=_sizepg)
      {
         delete[] _data;
         _data = new uint8_t[sizepg*_pgsz];
      }
      else if(!_data) {
               _data = new uint8_t[sizepg*_pgsz];
            }

      _sizepg = sizepg;
      _startp = startp;

      uint8_t ddata[2];
      for(uint8_t j=_startp;j<_sizepg;j++)
      {
         uint16_t addr = _pgsz*j;
         ddata[0] = (uint8_t)((addr >> 8)&0xFF);
         ddata[1] = (uint8_t)(addr & 0xFF);
         i2c_write_buffer(0x50, ddata, 2);
         i2c_read_buffer(0x50,_data+addr,_pgsz);
         yield();
      }

     _dirty = false; //make sure dirty is cleared in case begin() is called 2nd+ time
}

   bool commit()
   {
      bool ret = false;
      if (!_sizepg) return false;
      if(!_dirty) return true;
      if(!_data)  return false;


      uint8_t ddata[34];
      for(uint8_t j=_startp;j<_sizepg;j++)
      {
         uint16_t addr = _pgsz*j;
         memcpy(ddata+2,_data+addr,32);
         ddata[0] = (uint8_t)((addr >> 8)&0xFF);
         ddata[1] = (uint8_t)(addr & 0xFF);
         uint8_t s = i2c_write_bufferACK(0x50, ddata, 34);
         yield();
      }
      return ret;
   }


   void end()
   {
      if (!_sizepg) return;

      commit();
      if(_data) { delete[] _data; }
      _data = 0;
      _sizepg = 0;
      _dirty = false;
   }


   uint8_t read(int const address)
   {
      if (address < 0 || (size_t)address >= _size) return 0;
      if(!_data) return 0;
      return _data[address];
   }

   void write(int const address, uint8_t const val)
   {
      if (address < 0 || (size_t)address >= _size) return;
      if(!_data) return;

      // Optimise _dirty. Only flagged if data written is different.
      uint8_t* pData = &_data[address];
      if (*pData != value)
      {
         *pData = value;
         _dirty = true;
      }
   }

/*

   uint8_t * getDataPtr() { _dirty = true; return &_data[0];}

   uint8_t const * getConstDataPtr() const { return &_data[0];}

   size_t length() {return _size;}

   uint8_t& operator[](int const address) {return getDataPtr()[address];}
   uint8_t const & operator[](int const address) const {return getConstDataPtr()[address];}
*/

protected:
   uint8_t _addr;
   int _startp; // start page
   int _cnt; // qnt to use
   int _pgsz; // page size
   int _pgcnt; // page count
   uint8_t* _data;
   bool _dirty;

   int _sizepg(0),
   int _fsize;
};

#endif
