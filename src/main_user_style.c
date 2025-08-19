#include <msp430g2553.h>
#include <intrinsics.h>
#include "i2c.h"
#include "lcd_i2c.h"
#include "hx711.h"
#include <stdio.h>

#define BUTTON BIT3
static unsigned char scale_on = 0;

static void button_init(void) {
	P1DIR &= ~BUTTON;   // P1.3 input
	P1REN |= BUTTON;    // enable resistor
	P1OUT |= BUTTON;    // pull-up
}

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL  = CALDCO_1MHZ;

	// I2C pins
	P1SEL  |= BIT6 | BIT7;
	P1SEL2 |= BIT6 | BIT7;

	i2c_init();
	lcd_i2c_init();
	hx711_init(HX711_GAIN_128);
	button_init();

	lcd_clear();
	lcd_set_cursor(0,0);
	lcd_print("GVHD: NGUYEN DINH TU");
	lcd_set_cursor(0,1);
	lcd_print("SVTH: PHAM THANH TU,");

	__delay_cycles(1000000);
	lcd_set_cursor(0,1);
	lcd_print("DANG VAN QUANG   ");

	while (1) {
		if (!(P1IN & BUTTON)) {
			__delay_cycles(200000);
			if (!(P1IN & BUTTON)) {
				scale_on = !scale_on;
				lcd_clear();
				lcd_set_cursor(0,0);
				lcd_print(scale_on ? "Scale ON" : "Scale OFF");
				while (!(P1IN & BUTTON)) { }
			}
		}

		if (scale_on) {
			float grams = hx711_get_units(10);
			char buf[17];
			int whole = (int)grams;
			int frac = (int)((grams - (float)whole) * 10.0f);
			if (frac < 0) frac = -frac;
			lcd_set_cursor(0,1);
			snprintf(buf, sizeof(buf), "Wt:%6d.%1dg", whole, frac);
			lcd_print(buf);
		}
	}
}

