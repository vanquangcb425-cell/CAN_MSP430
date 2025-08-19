#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include "i2c.h"
#include "lcd_i2c.h"
#include "hx711.h"

// Button on P1.3 with pull-up (active low)
static void button_init(void) {
	P1REN |= BIT3; // enable resistor
	P1OUT |= BIT3; // pull-up
	P1DIR &= ~BIT3; // input
}

static unsigned char button_is_pressed(void) {
	return (P1IN & BIT3) == 0;
}

static void clock_init(void) {
	BCSCTL1 = CALBC1_1MHZ; // 1 MHz
	DCOCTL = CALDCO_1MHZ;
}

static void intro_until_button(void) {
	const char *line1 = "GVHD: NGUYEN DINH TU";
	const char *line2 = "SVTH: PHAM THANH TU, DANG VAN QUANG";
	const unsigned int width = 16;
	unsigned int len1 = (unsigned int)strlen(line1);
	unsigned int len2 = (unsigned int)strlen(line2);
	unsigned int idx1 = 0, idx2 = 0;
	char buf[17];

	while (!button_is_pressed()) {
		lcd_set_cursor(0,0);
		for (unsigned int j = 0; j < width; j++) {
			unsigned int k = idx1 + j;
			char c = (k < len1) ? line1[k] : ' ';
			buf[j] = c;
		}
		buf[width] = '\0';
		lcd_print(buf);

		lcd_set_cursor(0,1);
		for (unsigned int j = 0; j < width; j++) {
			unsigned int k = idx2 + j;
			char c = (k < len2) ? line2[k] : ' ';
			buf[j] = c;
		}
		buf[width] = '\0';
		lcd_print(buf);

		idx1 = (len1 > width) ? ((idx1 + 1) % (len1 + width)) : 0;
		idx2 = (len2 > width) ? ((idx2 + 1) % (len2 + width)) : 0;

		for (unsigned int d = 0; d < 6; d++) {
			if (button_is_pressed()) {
				__delay_cycles(500000);
				if (button_is_pressed()) {
					while (button_is_pressed()) { }
					return;
				}
			}
			__delay_cycles(50000);
		}
	}
	__delay_cycles(500000);
	while (button_is_pressed()) { }
}

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;
	clock_init();
	i2c_init();
	lcd_init();
	button_init();
	hx711_init(HX711_GAIN_128);

	intro_until_button();

	// Tare when entering
	long tare = hx711_read_average(10);
	hx711_set_tare(tare);

	while (1) {
		// If button pressed again, power down and show intro until pressed again
		if (button_is_pressed()) {
			__delay_cycles(500000);
			if (button_is_pressed()) {
				// toggle off state: power down, show intro until press
				hx711_power_down();
				intro_until_button();
				hx711_power_up();
				long new_tare = hx711_read_average(10);
				hx711_set_tare(new_tare);
			}
		}

		long raw = hx711_read_average(5);
		float grams = hx711_to_grams(raw);

		// Display
		char line[17];
		lcd_set_cursor(0,0);
		lcd_print_padded("WEIGHT (g):", 16);
		lcd_set_cursor(0,1);
		int whole = (int)grams;
		int frac = (int)((grams - (float)whole) * 10.0f);
		if (frac < 0) frac = -frac;
		snprintf(line, sizeof(line), "%6d.%1d g       ", whole, frac);
		lcd_print_padded(line, 16);

		__delay_cycles(200000); // ~200 ms
	}
}