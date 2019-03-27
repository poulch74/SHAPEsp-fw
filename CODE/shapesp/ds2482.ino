#include "inttypes.h"
#include "DS2482.h"
#define PTR_STATUS 0xF0
#define PTR_READ   0xE1
#define PTR_CONFIG 0xC3


DS2482::DS2482(uint8_t addr)
{
   mAddress = 0x18 | addr;
}

uint8_t DS2482::statusWait()
{
   uint8_t status;
   int loopCount = 1000;
   do
   {
      i2c_read_buffer(mAddress,&status,1);
      loopCount--;
      delayMicroseconds(5);
   } while((loopCount>0) && (status & DS2482_STATUS_BUSY));
   if(loopCount==0) { mTimeout = 1; yield(); }
   return status;
}

//----------interface
void DS2482::resetMaster()
{
   mTimeout = 0;
   uint8_t buf = 0xF0;
   i2c_write_buffer(mAddress,&buf,1);
}

bool DS2482::configure(uint8_t config)
{
   uint8_t buf[2] = {0xE1,PTR_STATUS};
   i2c_write_buffer(mAddress, buf,2);
   statusWait();
   buf[0] = 0xD2; buf[1] = (config | (~config)<<4);
   i2c_write_buffer(mAddress,buf,2);
   i2c_read_buffer(mAddress,buf,1);
   return buf[0] == config;
}


bool DS2482::reset()
{
   uint8_t buf[2] = {0xE1,PTR_STATUS};
   i2c_write_buffer(mAddress, buf,2);
   statusWait();
   buf[0] = 0xB4;
   i2c_write_buffer(mAddress,buf,1);
   uint8_t status = statusWait();
   return status&DS2482_STATUS_PPD ? true : false;
}

void DS2482::write(uint8_t b, uint8_t power)
{
   uint8_t buf[2] = {0xE1,PTR_STATUS};
   i2c_write_buffer(mAddress, buf,2);
   statusWait();
   buf[0] = 0xA5; buf[1] = b;
   i2c_write_buffer(mAddress,buf,2);
}

uint8_t DS2482::read()
{
   uint8_t buf[2] = {0xE1,PTR_STATUS};
   i2c_write_buffer(mAddress, buf,2);
   statusWait();
   buf[0] = 0x96;
   i2c_write_buffer(mAddress,buf,1);
   statusWait();
   buf[0] = 0xE1; buf[1] = PTR_READ;
   i2c_write_buffer(mAddress, buf,2);
   i2c_read_buffer(mAddress,buf,1);
   return buf[0];
}

void DS2482::skip() { write(0xcc); }

void DS2482::select(uint8_t rom[8])
{
   write(0x55); for(int i=0;i<8;i++) write(rom[i]);
}


#if ONEWIRE_SEARCH
void DS2482::reset_search()
{
   searchExhausted = 0;
   // Initialize to negative value
   searchLastDisrepancy = -1;
   for(uint8_t i = 0; i<8; i++) searchAddress[i] = 0;
}

uint8_t DS2482::search(uint8_t *newAddr)
{
   uint8_t i;
   uint8_t direction;
   uint8_t last_zero=0;

   if(searchExhausted) return 0;

   if(!reset()) return 0;

   write(0xF0);

   for(i=0;i<64;i++)
   {
      int romByte = i/8;
      int romBit = 1<<(i&7);

      if(i < searchLastDisrepancy) direction = searchAddress[romByte] & romBit;
      else direction = i == searchLastDisrepancy;

      statusWait();
      uint8_t buf[2] = {0x78,(direction ? 0x80 : 0)};
      i2c_write_buffer(mAddress,buf,2);
      uint8_t status = statusWait();

      uint8_t id = status & DS2482_STATUS_SBR;
      uint8_t comp_id = status & DS2482_STATUS_TSB;
      direction = status & DS2482_STATUS_DIR;

      if (id && comp_id) return 0;
      else
      {
         if (!id && !comp_id && !direction) last_zero = i;
      }

      if(direction) searchAddress[romByte] |= romBit;
      else searchAddress[romByte] &= (uint8_t)~romBit;
   }

   searchLastDisrepancy = last_zero;
   if(last_zero == 0) searchExhausted = 1;
   for (i=0;i<8;i++) newAddr[i] = searchAddress[i];
   return 1;
}
#endif

#if ONEWIRE_CRC
// The 1-Wire CRC scheme is described in Maxim Application Note 27:
// "Understanding and Using Cyclic Redundancy Checks with Maxim iButton Products"
//

uint8_t DS2482::crc8( uint8_t *addr, uint8_t len)
{
   uint8_t crc=0;

   for (uint8_t i=0; i<len;i++)
   {
      uint8_t inbyte = addr[i];
      for (uint8_t j=0;j<8;j++)
      {
         uint8_t mix = (crc ^ inbyte) & 0x01;
         crc >>= 1;
         if (mix)
            crc ^= 0x8C;

         inbyte >>= 1;
      }
   }
   return crc;
}

#endif
