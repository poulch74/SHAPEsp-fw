#if I2C_USE_BRZO
   #include "brzo_i2c.h"
   unsigned long _i2c_scl_frequency = 400;
#else
   #include "Wire.h"
#endif

#if I2C_USE_BRZO

void i2c_wakeup(uint8_t address)
{
    brzo_i2c_start_transaction(address, _i2c_scl_frequency);
    brzo_i2c_end_transaction();
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t value) {
    uint8_t buffer[1] = {value};
    brzo_i2c_start_transaction(address, _i2c_scl_frequency);
    brzo_i2c_write(buffer, 1, false);
    return brzo_i2c_end_transaction();
}

uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    brzo_i2c_start_transaction(address, _i2c_scl_frequency);
    brzo_i2c_write(buffer, len, false);
    return brzo_i2c_end_transaction();
}

uint8_t i2c_read_uint8(uint8_t address, uint8_t reg) {
    uint8_t buffer[1] = {reg};
    brzo_i2c_start_transaction(address, _i2c_scl_frequency);
    brzo_i2c_write(buffer, 1, false);
    brzo_i2c_read(buffer, 1, false);
    brzo_i2c_end_transaction();
    return buffer[0];
};

uint16_t i2c_read_uint16(uint8_t address, uint8_t reg) {
    uint8_t buffer[2] = {reg, 0};
    brzo_i2c_start_transaction(address, _i2c_scl_frequency);
    brzo_i2c_write(buffer, 1, false);
    brzo_i2c_read(buffer, 2, false);
    brzo_i2c_end_transaction();
    return (buffer[0] * 256) | buffer[1];
};

void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    brzo_i2c_start_transaction(address, _i2c_scl_frequency);
    brzo_i2c_read(buffer, len, false);
    brzo_i2c_end_transaction();
}

#else // not I2C_USE_BRZO

void i2c_wakeup(uint8_t address) {
    Wire.beginTransmission((uint8_t) address);
    Wire.endTransmission();
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t value) {
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) value);
    return Wire.endTransmission();
}

uint8_t i2c_write_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    Wire.beginTransmission((uint8_t) address);
    Wire.write(buffer, len);
    return Wire.endTransmission();
}

uint8_t i2c_read_uint8(uint8_t address) {
    uint8_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    Wire.endTransmission();
    return value;
};

uint8_t i2c_read_uint8(uint8_t address, uint8_t reg) {
    uint8_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 1);
    value = Wire.read();
    Wire.endTransmission();
    return value;
};

uint16_t i2c_read_uint16(uint8_t address) {
    uint16_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    Wire.endTransmission();
    return value;
};

uint16_t i2c_read_uint16(uint8_t address, uint8_t reg) {
    uint16_t value;
    Wire.beginTransmission((uint8_t) address);
    Wire.write((uint8_t) reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t) address, (uint8_t) 2);
    value = (Wire.read() * 256) | Wire.read();
    Wire.endTransmission();
    return value;
};

void i2c_read_buffer(uint8_t address, uint8_t * buffer, size_t len) {
    Wire.beginTransmission((uint8_t) address);
    Wire.requestFrom(address, (uint8_t) len);
    for (int i=0; i<len; i++) buffer[i] = Wire.read();
    Wire.endTransmission();
}

#endif // I2C_USE_BRZO

uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    return i2c_write_buffer(address, buffer, 2);
}

uint8_t i2c_write_uint8(uint8_t address, uint8_t reg, uint8_t value1, uint8_t value2) {
    uint8_t buffer[3] = {reg, value1, value2};
    return i2c_write_buffer(address, buffer, 3);
}

uint8_t i2c_write_uint16(uint8_t address, uint8_t reg, uint16_t value) {
    uint8_t buffer[3];
    buffer[0] = reg;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 0) & 0xFF;
    return i2c_write_buffer(address, buffer, 3);
}

uint8_t i2c_write_uint16(uint8_t address, uint16_t value) {
    uint8_t buffer[2];
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = (value >> 0) & 0xFF;
    return i2c_write_buffer(address, buffer, 2);
}

uint16_t i2c_read_uint16_le(uint8_t address, uint8_t reg) {
    uint16_t temp = i2c_read_uint16(address, reg);
    return (temp / 256) | (temp * 256);
};

int16_t i2c_read_int16(uint8_t address, uint8_t reg) {
    return (int16_t) i2c_read_uint16(address, reg);
};

int16_t i2c_read_int16_le(uint8_t address, uint8_t reg)
{
    return (int16_t) i2c_read_uint16_le(address, reg);
};


void i2c_setup(uint8_t sda, uint8_t scl, uint32_t cst, uint32_t freq)
{

    #if I2C_USE_BRZO
        _i2c_scl_frequency = freq;
        brzo_i2c_setup(sda, scl, cst);
    #else
        Wire.begin(sda, scl);
    #endif


}

bool i2cCheck(unsigned char address)
{
   #if I2C_USE_BRZO
      brzo_i2c_start_transaction(address, _i2c_scl_frequency);
      brzo_i2c_ACK_polling(1000);
      return brzo_i2c_end_transaction();
   #else
      Wire.beginTransmission(address);
      return Wire.endTransmission();
   #endif
}

void i2cScan()
{
   unsigned char nDevices = 0;
   for (unsigned char address = 1; address < 127; address++)
   {
      unsigned char error = i2cCheck(address);
      if (error == 0)
      {
         DEBUG_MSG_P(PSTR("[I2C] Device found at address 0x%02X\n"), address);
         nDevices++;
      }
   }
   if (nDevices == 0) DEBUG_MSG_P(PSTR("[I2C] No devices found\n"));
}
