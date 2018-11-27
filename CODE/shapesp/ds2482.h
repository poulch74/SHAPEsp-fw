#ifndef __DS2482_H__
#define __DS2482_H__

// you can exclude onewire_search by defining that to 0
#ifndef ONEWIRE_SEARCH
#define ONEWIRE_SEARCH 1
#endif

// You can exclude CRC checks altogether by defining this to 0
#ifndef ONEWIRE_CRC
#define ONEWIRE_CRC 1
#endif


#define DS2482_CONFIG_APU (1<<0)
#define DS2482_CONFIG_PPM (1<<1)
#define DS2482_CONFIG_SPU (1<<2)
#define DS2484_CONFIG_WS  (1<<3)

#define DS2482_STATUS_BUSY (1<<0)
#define DS2482_STATUS_PPD  (1<<1)
#define DS2482_STATUS_SD   (1<<2)
#define DS2482_STATUS_LL   (1<<3)
#define DS2482_STATUS_RST  (1<<4)
#define DS2482_STATUS_SBR  (1<<5)
#define DS2482_STATUS_TSB  (1<<6)
#define DS2482_STATUS_DIR  (1<<7)

class DS2482
{
public:
   //Address is 0-3
   DS2482(uint8_t address);
   
   bool configure(uint8_t config);
   void resetMaster();
  
   bool reset(); // return true if presence pulse is detected
   
   void write(uint8_t b, uint8_t power = 0);
   uint8_t read();
   
   // Issue a 1-Wire rom select command, you do the reset first.
   void select( uint8_t rom[8]);
   // Issue skip rom
   void skip();
   
   uint8_t hasTimeout() { return mTimeout; }
#if ONEWIRE_SEARCH
    // Clear the search state so that if will start from the beginning again.
    void reset_search();

    // Look for the next device. Returns 1 if a new address has been
    // returned. A zero might mean that the bus is shorted, there are
    // no devices, or you have already retrieved all of them.  It
    // might be a good idea to check the CRC to make sure you didn't
    // get garbage.  The order is deterministic. You will always get
    // the same devices in the same order.
    uint8_t search(uint8_t *newAddr);
#endif
#if ONEWIRE_CRC
    // Compute a Dallas Semiconductor 8 bit CRC, these are used in the
    // ROM and scratchpad registers.
    static uint8_t crc8( uint8_t *addr, uint8_t len);
#endif

private:
   
   uint8_t mAddress;
   uint8_t mTimeout;
   
   uint8_t statusWait(); //blocks until
   
#if ONEWIRE_SEARCH
   uint8_t searchAddress[8];
   int8_t searchLastDisrepancy;
   uint8_t searchExhausted;
#endif
   
};

#endif
