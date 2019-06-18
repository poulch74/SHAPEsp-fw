//#define EE24C_ADDR 0x50

uint8_t eeprom_read(uint16_t address)
{
/*
   unsigned int rdata = 0xFF;

   Wire.beginTransmission(0x50);
   Wire.write((int)(address >> 8)); // MSB
   Wire.write((int)(address & 0xFF)); // LSB
   Wire.endTransmission();
   Wire.requestFrom(0x50, 1);
   if (Wire.available()) { rdata = Wire.read(); }
   return rdata;
*/

   uint8_t ddata[2];
   ddata[0] = (uint8_t)((address >> 8)&0xFF);
   ddata[1] = (uint8_t)(address & 0xFF);
   i2c_write_buffer(0x50, ddata, 2);
   i2c_read_buffer(0x50,ddata,1);
   return ddata[0];

}

void eeprom_read_full(uint8_t *data)
{
/*
   unsigned int rdata = 0xFF;

   Wire.beginTransmission(0x50);
   Wire.write((int)(address >> 8)); // MSB
   Wire.write((int)(address & 0xFF)); // LSB
   Wire.endTransmission();
   Wire.requestFrom(0x50, 1);
   if (Wire.available()) { rdata = Wire.read(); }
   return rdata;
*/
   uint8_t ddata[2];
   for(uint8_t j=0;j<128;j++)
   {
      uint16_t addr = 32*j;
      ddata[0] = (uint8_t)((addr >> 8)&0xFF);
      ddata[1] = (uint8_t)(addr & 0xFF);
      i2c_write_buffer(0x50, ddata, 2);
      i2c_read_buffer(0x50,data+j*32,32);
      yield();
   }

/*

   ddata[0] = 0;
   ddata[1] = 0;
   i2c_write_buffer(0x50, ddata, 2);
   i2c_read_buffer(0x50,data,4096);
*/
}


uint8_t eeprom_write(uint16_t address, uint8_t data)
{
/*
   Wire.beginTransmission(0x50);
   Wire.write((int)(address >> 8)); // MSB
   Wire.write((int)(address & 0xFF)); // LSB
   Wire.write(data);
   Wire.endTransmission();
   delay(10);
*/
   uint8_t ddata[3];
   ddata[0] = 0;(uint8_t)((address >> 8)&0xFF);
   ddata[1] = (uint8_t)(address & 0xFF);
   ddata[2] = data;
   uint8_t s = i2c_write_bufferACK(0x50, ddata, 3);
   return s;
}

uint8_t eeprom_write_32(uint16_t address, uint8_t *data)
{
/*
   Wire.beginTransmission(0x50);
   Wire.write((int)(address >> 8)); // MSB
   Wire.write((int)(address & 0xFF)); // LSB
   Wire.write(data);
   Wire.endTransmission();
   delay(10);
*/
   uint8_t ddata[34];
   memcpy(ddata+2,data,32);
   ddata[0] = (uint8_t)((address >> 8)&0xFF);
   ddata[1] = (uint8_t)(address & 0xFF);
   uint8_t s = i2c_write_bufferACK(0x50, ddata, 34);
   yield();
   return s;
}