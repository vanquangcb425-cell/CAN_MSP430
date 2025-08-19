#ifndef I2C_H
#define I2C_H

#include <msp430.h>

void i2c_init(void);
unsigned char i2c_write_byte(unsigned char address, unsigned char data);
unsigned char i2c_write_bytes(unsigned char address, const unsigned char *data, unsigned int length);

#endif

