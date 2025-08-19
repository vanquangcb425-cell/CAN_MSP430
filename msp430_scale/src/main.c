#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// =============================================
// Configuration (adjust to your hardware)
// =============================================

// LCD I2C backpack address (PCF8574). Common values: 0x27 or 0x3F
#ifndef LCD_I2C_ADDR
#define LCD_I2C_ADDR        0x27
#endif

// LCD PCF8574 bit mapping (LiquidCrystal_I2C common mapping)
// P0=RS, P1=RW, P2=EN, P3=Backlight, P4=D4, P5=D5, P6=D6, P7=D7
#define LCD_BIT_RS          0x01
#define LCD_BIT_RW          0x02
#define LCD_BIT_EN          0x04
#define LCD_BIT_BL          0x08

// HX711 pin mapping
// P2.0 -> HX711 PD_SCK (output)
// P2.1 -> HX711 DOUT   (input)
#define HX711_PORT_DIR      P2DIR
#define HX711_PORT_OUT      P2OUT
#define HX711_PORT_IN       P2IN
#define HX711_PORT_REN      P2REN
#define HX711_SCK_BIT       BIT0
#define HX711_DOUT_BIT      BIT1

// Button on LaunchPad S2 (P1.3), active low with pull-up
#define BUTTON_PORT_DIR     P1DIR
#define BUTTON_PORT_OUT     P1OUT
#define BUTTON_PORT_IN      P1IN
#define BUTTON_PORT_REN     P1REN
#define BUTTON_PORT_IE      P1IE
#define BUTTON_PORT_IES     P1IES
#define BUTTON_PORT_IFG     P1IFG
#define BUTTON_BIT          BIT3

// Clock frequency for delay calculations
#define CPU_F_HZ            1000000UL  // 1 MHz DCO

// HX711 conversion tuning
#define HX711_SAMPLE_AVG    10         // samples to average for display
#define HX711_TARE_SAMPLES  20         // samples to average for tare

// Scale calibration: grams = (raw - offset) / CALIBRATION_SCALE
// You must calibrate this constant for your specific load cell.
#ifndef CALIBRATION_SCALE
#define CALIBRATION_SCALE   1000.0f    // placeholder; adjust after calibration
#endif

// =============================================
// Utility macros
// =============================================

#define SET_BIT(REG, BIT)     ((REG) |= (BIT))
#define CLR_BIT(REG, BIT)     ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)    (((REG) & (BIT)) != 0)

// =============================================
// Global state
// =============================================

typedef enum {
	PROGRAM_IDLE = 0,
	PROGRAM_ACTIVE = 1
} program_state_t;

static volatile bool g_buttonPressed = false;
static volatile uint32_t g_millis = 0;
static volatile uint32_t g_lastButtonMs = 0;
static volatile bool g_backlightOn = true;
static int32_t g_tareOffset = 0;

// =============================================
// Forward declarations
// =============================================

static void init_clock_1mhz(void);
static void init_timer_ms(void);
static void init_button(void);
static void init_hx711(void);
static void init_i2c_lcd(void);

static void lcd_init(void);
static void lcd_clear(void);
static void lcd_set_cursor(uint8_t col, uint8_t row);
static void lcd_print(const char *s);
static void lcd_print_n(const char *s, uint8_t n);
static void lcd_print_int(int32_t value);
static void lcd_backlight(bool on);

static void i2c_send_byte(uint8_t addr7, uint8_t data);
static void lcd_write_nibble(uint8_t nibble, uint8_t controlFlags);
static void lcd_send(uint8_t value, bool rs);
static void delay_us(uint16_t us);

static int32_t hx711_read_raw(void);
static int32_t hx711_read_average(uint8_t numSamples);

// =============================================
// Implementation
// =============================================

static void init_clock_1mhz(void) {
	// Set DCO to ~1MHz
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;
}

static void init_timer_ms(void) {
	// Timer_A: SMCLK/8 -> 125 kHz, CCR0 = 125 -> 1ms
	TA0CTL = TASSEL_2 | ID_3 | MC_1 | TACLR; // SMCLK, /8, up mode
	TA0CCR0 = 125 - 1;
	TA0CCTL0 = CCIE;
}

static void init_button(void) {
	// Configure P1.3 as input with pull-up, interrupt on falling edge
	CLR_BIT(BUTTON_PORT_DIR, BUTTON_BIT);
	SET_BIT(BUTTON_PORT_REN, BUTTON_BIT);
	SET_BIT(BUTTON_PORT_OUT, BUTTON_BIT); // pull-up
	SET_BIT(BUTTON_PORT_IES, BUTTON_BIT); // high-to-low
	CLR_BIT(BUTTON_PORT_IFG, BUTTON_BIT);
	SET_BIT(BUTTON_PORT_IE, BUTTON_BIT);
}

static void init_hx711(void) {
	// SCK output low; DOUT input with pull-up
	SET_BIT(HX711_PORT_DIR, HX711_SCK_BIT);
	CLR_BIT(HX711_PORT_OUT, HX711_SCK_BIT);
	CLR_BIT(HX711_PORT_DIR, HX711_DOUT_BIT);
	SET_BIT(HX711_PORT_REN, HX711_DOUT_BIT);
	SET_BIT(HX711_PORT_OUT, HX711_DOUT_BIT); // pull-up
}

static void init_i2c_lcd(void) {
	// I2C on USCI_B0: P1.6=SCL, P1.7=SDA
	P1SEL  |= BIT6 | BIT7;
	P1SEL2 |= BIT6 | BIT7;

	UCB0CTL1 |= UCSWRST;                 // hold in reset
	UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC; // I2C master, sync
	UCB0CTL1 = UCSSEL_2 | UCSWRST;        // SMCLK
	UCB0BR0 = 10;  // SMCLK/10 -> 100kHz at 1MHz
	UCB0BR1 = 0;
	UCB0I2CSA = LCD_I2C_ADDR;
	UCB0CTL1 &= ~UCSWRST;                // release for operation

	// Initialize LCD module
	lcd_init();
}

static void delay_us(uint16_t us) {
	// Busy-wait using CPU cycles; at 1 MHz, 1 us ~ 1 cycle overhead is more
	while (us--) {
		__delay_cycles(1);
	}
}

static void i2c_send_byte(uint8_t addr7, uint8_t data) {
	// Set slave address each call for safety
	UCB0I2CSA = addr7;

	// Ensure bus is idle
	while (UCB0STAT & UCBBUSY) {
		; // wait
	}

	UCB0CTL1 |= UCTR | UCTXSTT; // TX mode, send START
	while (UCB0CTL1 & UCTXSTT) {
		; // wait for start to send
	}

	// Send data byte
	UCB0TXBUF = data;
	while (!(IFG2 & UCB0TXIFG)) {
		; // wait for TX complete
	}

	UCB0CTL1 |= UCTXSTP; // send STOP
	while (UCB0CTL1 & UCTXSTP) {
		; // wait for STOP
	}
}

static void lcd_pulse_enable(uint8_t data) {
	i2c_send_byte(LCD_I2C_ADDR, data | LCD_BIT_EN);
	delay_us(1);
	i2c_send_byte(LCD_I2C_ADDR, data & ~LCD_BIT_EN);
	delay_us(50);
}

static void lcd_write_nibble(uint8_t nibble, uint8_t controlFlags) {
	uint8_t data = ((nibble & 0x0F) << 4) | controlFlags;
	if (g_backlightOn) {
		data |= LCD_BIT_BL;
	}
	i2c_send_byte(LCD_I2C_ADDR, data);
	lcd_pulse_enable(data);
}

static void lcd_send(uint8_t value, bool rs) {
	uint8_t ctrl = rs ? LCD_BIT_RS : 0x00; // RW always 0 for write

	// Send high nibble then low nibble
	lcd_write_nibble((value >> 4) & 0x0F, ctrl);
	lcd_write_nibble(value & 0x0F, ctrl);
}

static void lcd_command(uint8_t cmd) {
	lcd_send(cmd, false);
}

static void lcd_write_char(char c) {
	lcd_send((uint8_t)c, true);
}

static void lcd_init(void) {
	// Wait for LCD to power up
	__delay_cycles(50000); // ~50ms at 1MHz

	// Initialize expander with backlight on
	uint8_t initData = g_backlightOn ? LCD_BIT_BL : 0x00;
	i2c_send_byte(LCD_I2C_ADDR, initData);

	// 4-bit init sequence
	lcd_write_nibble(0x03, 0); // Function set
	__delay_cycles(5000);
	lcd_write_nibble(0x03, 0);
	__delay_cycles(5000);
	lcd_write_nibble(0x03, 0);
	__delay_cycles(150);
	lcd_write_nibble(0x02, 0); // 4-bit mode

	// 4-bit, 2 line, 5x8 dots
	lcd_command(0x28);
	// Display on, cursor off, blink off
	lcd_command(0x0C);
	// Clear display
	lcd_command(0x01);
	__delay_cycles(2000);
	// Entry mode set: increment, no shift
	lcd_command(0x06);
}

static void lcd_clear(void) {
	lcd_command(0x01);
	__delay_cycles(2000);
}

static void lcd_set_cursor(uint8_t col, uint8_t row) {
	static const uint8_t rowOffsets[] = {0x00, 0x40, 0x14, 0x54};
	if (row > 1) row = 1; // only 2 rows used
	lcd_command(0x80 | (col + rowOffsets[row]));
}

static void lcd_print(const char *s) {
	while (*s) {
		lcd_write_char(*s++);
	}
}

static void lcd_print_n(const char *s, uint8_t n) {
	for (uint8_t i = 0; i < n && s[i]; i++) {
		lcd_write_char(s[i]);
	}
}

static void lcd_print_int(int32_t value) {
	char buf[16];
	char *p = buf + sizeof(buf) - 1;
	*p = '\0';
	bool neg = value < 0;
	uint32_t v = neg ? (uint32_t)(-value) : (uint32_t)value;
	if (v == 0) {
		*--p = '0';
	} else {
		while (v > 0 && p > buf) {
			*--p = (char)('0' + (v % 10));
			v /= 10;
		}
	}
	if (neg && p > buf) {
		*--p = '-';
	}
	lcd_print(p);
}

static void lcd_backlight(bool on) {
	g_backlightOn = on;
	// Send a no-op write to update BL state
	i2c_send_byte(LCD_I2C_ADDR, on ? LCD_BIT_BL : 0x00);
}

static void hx711_set_sck_high(void) {
	SET_BIT(HX711_PORT_OUT, HX711_SCK_BIT);
}

static void hx711_set_sck_low(void) {
	CLR_BIT(HX711_PORT_OUT, HX711_SCK_BIT);
}

static bool hx711_is_ready(void) {
	return !READ_BIT(HX711_PORT_IN, HX711_DOUT_BIT);
}

static int32_t hx711_read_raw(void) {
	int32_t count = 0;
	// Wait for data ready (DOUT low)
	while (!hx711_is_ready()) {
		; // blocking wait
	}

	__disable_interrupt();
	// Read 24 bits, MSB first
	for (uint8_t i = 0; i < 24; i++) {
		hx711_set_sck_high();
		// minimal high time
		delay_us(1);
		count <<= 1;
		if (READ_BIT(HX711_PORT_IN, HX711_DOUT_BIT)) {
			count++;
		}
		hx711_set_sck_low();
		delay_us(1);
	}

	// Set gain to 128 (Channel A): 1 additional clock
	hx711_set_sck_high();
	delay_us(1);
	hx711_set_sck_low();
	__enable_interrupt();

	// Sign extend 24-bit two's complement
	if (count & 0x800000) {
		count |= ~0xFFFFFF;
	}
	return count;
}

static int32_t hx711_read_average(uint8_t numSamples) {
	int64_t sum = 0;
	for (uint8_t i = 0; i < numSamples; i++) {
		int32_t v = hx711_read_raw();
		sum += v;
	}
	return (int32_t)(sum / (int32_t)numSamples);
}

static void tare_scale(void) {
	g_tareOffset = hx711_read_average(HX711_TARE_SAMPLES);
}

// =============================================
// Interrupts
// =============================================

#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {
	g_millis++;
}

#pragma vector = PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
	if (BUTTON_PORT_IFG & BUTTON_BIT) {
		// Debounce: require 200 ms between presses
		uint32_t now = g_millis;
		if ((now - g_lastButtonMs) > 200) {
			g_lastButtonMs = now;
			g_buttonPressed = true;
		}
		CLR_BIT(BUTTON_PORT_IFG, BUTTON_BIT);
	}
}

// =============================================
// Main
// =============================================

static void show_greeting_idle(void) {
	// Marquee scroll both lines until button is pressed
	const char *full1 = "GVHD: NGUYEN DINH TU";
	const char *full2 = "SVTH: PHAM THANH TU, DANG VAN QUANG";

	char win1[17];
	char win2[17];
	win1[16] = '\0';
	win2[16] = '\0';

	uint8_t len1 = 0, len2 = 0;
	while (full1[len1] != '\0') len1++;
	while (full2[len2] != '\0') len2++;

	uint8_t start1 = 0, start2 = 0;

	lcd_clear();
	while (!g_buttonPressed) {
		for (uint8_t i = 0; i < 16; i++) {
			uint8_t idx1 = start1 + i;
			uint8_t idx2 = start2 + i;
			win1[i] = (idx1 < len1) ? full1[idx1] : ' ';
			win2[i] = (idx2 < len2) ? full2[idx2] : ' ';
		}
		lcd_set_cursor(0, 0);
		lcd_print(win1);
		lcd_set_cursor(0, 1);
		lcd_print(win2);

		start1 = (start1 + 1 > len1) ? 0 : (start1 + 1);
		start2 = (start2 + 1 > len2) ? 0 : (start2 + 1);

		uint32_t target = g_millis + 250;
		while ((int32_t)(target - g_millis) > 0) {
			if (g_buttonPressed) break;
		}
	}
}

int main(void) {
	WDTCTL = WDTPW | WDTHOLD; // Stop watchdog

	init_clock_1mhz();
	init_timer_ms();
	init_button();
	init_hx711();
	init_i2c_lcd();

	__enable_interrupt();

	program_state_t state = PROGRAM_IDLE;

	for (;;) {
		if (state == PROGRAM_IDLE) {
			g_buttonPressed = false;
			lcd_backlight(true);
			show_greeting_idle();
			// Button pressed -> transition to active
			if (g_buttonPressed) {
				g_buttonPressed = false;
				lcd_clear();
				lcd_set_cursor(0, 0);
				lcd_print("Dang zeroing...");
				tare_scale();
				lcd_clear();
				state = PROGRAM_ACTIVE;
			}
		} else {
			// Active weighing mode
			int32_t rawAverage = hx711_read_average(HX711_SAMPLE_AVG);
			int32_t net = rawAverage - g_tareOffset;
			float gramsF = ((float)net) / CALIBRATION_SCALE;
			int32_t grams = (int32_t)(gramsF + (gramsF >= 0 ? 0.5f : -0.5f));

			lcd_set_cursor(0, 0);
			lcd_print("KHOI LUONG:");
			lcd_set_cursor(0, 1);
			lcd_print("      ");
			lcd_set_cursor(0, 1);
			lcd_print_int(grams);
			lcd_print(" g");

			// Check button to return to idle
			if (g_buttonPressed) {
				g_buttonPressed = false;
				state = PROGRAM_IDLE;
				lcd_clear();
			}
		}
	}
}

