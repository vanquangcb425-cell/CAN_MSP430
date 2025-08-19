#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <msp430.h>

#define LCD_I2C_ADDRESS 0x27

void lcd_init(void);
void lcd_i2c_init(void);
void lcd_clear(void);
void lcd_home(void);
void lcd_set_cursor(unsigned char col, unsigned char row);
void lcd_print(const char *s);
void lcd_print_padded(const char *s, unsigned char width);
void lcd_print_number_long(long value);

#endif

