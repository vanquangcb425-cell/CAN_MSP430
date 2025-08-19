#include "hx711.h"
#ifdef __IAR_SYSTEMS_ICC__
#include <intrinsics.h>
#endif

static unsigned char hx711_gain = HX711_GAIN_128; // default A 128
static long hx711_offset = 0;

static inline void hx711_set_sck(unsigned char high) {
	if (high) HX711_SCK_PORT_OUT |= HX711_SCK_PIN; else HX711_SCK_PORT_OUT &= ~HX711_SCK_PIN;
}

void hx711_init(unsigned char gain_mode) {
	hx711_gain = gain_mode;
	HX711_SCK_PORT_DIR |= HX711_SCK_PIN;  // SCK output
	HX711_DOUT_PORT_DIR &= ~HX711_DOUT_PIN; // DOUT input
	hx711_set_sck(0);
}

static void hx711_wait_ready(void) {
	while (HX711_DOUT_PORT_IN & HX711_DOUT_PIN) { }
}

long hx711_read(void) {
	unsigned long value = 0;
	hx711_wait_ready();
	__disable_interrupt();
	for (unsigned char i = 0; i < 24; i++) {
		hx711_set_sck(1);
		__delay_cycles(1);
		value = (value << 1) | ((HX711_DOUT_PORT_IN & HX711_DOUT_PIN) ? 1 : 0);
		hx711_set_sck(0);
		__delay_cycles(1);
	}
	for (unsigned char i = 0; i < hx711_gain; i++) {
		hx711_set_sck(1);
		__delay_cycles(1);
		hx711_set_sck(0);
		__delay_cycles(1);
	}
	__enable_interrupt();
	if (value & 0x800000) value |= 0xFF000000; // sign extend 24 -> 32
	return (long)value;
}

long hx711_read_average(unsigned int samples) {
	long sum = 0;
	for (unsigned int i = 0; i < samples; i++) {
		sum += hx711_read();
	}
	return sum / (long)samples;
}

void hx711_power_down(void) {
	hx711_set_sck(0);
	hx711_set_sck(1);
	__delay_cycles(6000); // > 60 us
}

void hx711_power_up(void) {
	hx711_set_sck(0);
}

void hx711_set_tare(long tare) { hx711_offset = tare; }
long hx711_get_offset(void) { return hx711_offset; }

float hx711_to_grams(long raw) {
	float net = (float)(raw - hx711_offset);
	return net / SCALE_FACTOR;
}

