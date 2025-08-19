#include "lcd_i2c.h"
#include "i2c.h"
#ifdef __IAR_SYSTEMS_ICC__
#include <intrinsics.h>
#endif

// LCD via PCF8574 backpack (HD44780)
// Control bits in expander nibble
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RW        0x02
#define LCD_RS        0x01

static void lcd_write4(unsigned char data) {
	unsigned char buf[1];
	buf[0] = data | LCD_BACKLIGHT;
	i2c_write_bytes(LCD_I2C_ADDRESS, buf, 1);
	buf[0] = data | LCD_ENABLE | LCD_BACKLIGHT;
	i2c_write_bytes(LCD_I2C_ADDRESS, buf, 1);
	__delay_cycles(2000); // ~2 ms at 1 MHz to meet enable pulse
	buf[0] = (data & ~LCD_ENABLE) | LCD_BACKLIGHT;
	i2c_write_bytes(LCD_I2C_ADDRESS, buf, 1);
}

static void lcd_send(unsigned char value, unsigned char mode) {
	unsigned char high = (value & 0xF0) | mode;
	unsigned char low  = ((value << 4) & 0xF0) | mode;
	lcd_write4(high);
	lcd_write4(low);
	__delay_cycles(2000);
}

static void lcd_command(unsigned char cmd) { lcd_send(cmd, 0); }
static void lcd_write_char(unsigned char ch) { lcd_send(ch, LCD_RS); }

void lcd_init(void) {
	__delay_cycles(50000);
	// 4-bit init sequence
	lcd_write4(0x30);
	__delay_cycles(50000);
	lcd_write4(0x30);
	__delay_cycles(5000);
	lcd_write4(0x20); // 4-bit mode
	__delay_cycles(5000);

	lcd_command(0x28); // 4-bit, 2 lines, 5x8
	lcd_command(0x08); // display off
	lcd_command(0x01); // clear
	__delay_cycles(50000);
	lcd_command(0x06); // entry mode
	lcd_command(0x0C); // display on, cursor off
}

void lcd_i2c_init(void) { lcd_init(); }

void lcd_clear(void) { lcd_command(0x01); __delay_cycles(50000); }
void lcd_home(void) { lcd_command(0x02); __delay_cycles(50000); }

void lcd_set_cursor(unsigned char col, unsigned char row) {
	static const unsigned char row_offsets[] = {0x00, 0x40, 0x14, 0x54};
	if (row > 1) row = 1;
	lcd_command(0x80 | (col + row_offsets[row]));
}

void lcd_print(const char *s) {
	while (*s) {
		lcd_write_char((unsigned char)*s++);
	}
}

void lcd_print_padded(const char *s, unsigned char width) {
	unsigned char i = 0;
	while (*s && i < width) { lcd_write_char((unsigned char)*s++); i++; }
	for (; i < width; i++) lcd_write_char(' ');
}

void lcd_print_number_long(long value) {
	char buf[16];
	char *p = buf + sizeof(buf) - 1;
	unsigned char negative = 0;
	*p = '\0';
	if (value == 0) { lcd_write_char('0'); return; }
	if (value < 0) { negative = 1; value = -value; }
	while (value && p > buf) {
		*--p = (char)('0' + (value % 10));
		value /= 10;
	}
	if (negative && p > buf) *--p = '-';
	lcd_print(p);
}

