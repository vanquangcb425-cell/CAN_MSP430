#ifndef HX711_H
#define HX711_H

#include <msp430.h>

// Pin mapping (Port 2)
#define HX711_DOUT_PORT_DIR P2DIR
#define HX711_DOUT_PORT_IN  P2IN
#define HX711_DOUT_PIN      BIT1

#define HX711_SCK_PORT_DIR  P2DIR
#define HX711_SCK_PORT_OUT  P2OUT
#define HX711_SCK_PIN       BIT0

// Calibration parameters
#define HX711_GAIN_128 1
#define HX711_GAIN_64  3
#define HX711_GAIN_32  2

#define SCALE_FACTOR  210.0f // adjust to your load cell (raw units to grams)

void hx711_init(unsigned char gain_mode);
long hx711_read_average(unsigned int samples);
long hx711_read(void);
void hx711_power_down(void);
void hx711_power_up(void);
void hx711_set_tare(long tare);
long hx711_get_offset(void);
float hx711_to_grams(long raw);

