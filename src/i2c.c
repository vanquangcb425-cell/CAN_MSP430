#include "i2c.h"

// I2C on USCI_B0, P1.6 = SCL, P1.7 = SDA for MSP430G2553

void i2c_init(void) {
	WDTCTL = WDTPW | WDTHOLD;
	UCB0CTL1 |= UCSWRST; // put USCI_B0 in reset
	P1SEL |= BIT6 | BIT7; // I2C pins
	P1SEL2 |= BIT6 | BIT7;
	UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC; // I2C master, synchronous
	UCB0CTL1 = UCSWRST | UCSSEL_2; // SMCLK
	UCB0BR0 = 12; // 1 MHz / 12 ~ 83 kHz
	UCB0BR1 = 0;
	UCB0CTL1 &= ~UCSWRST; // enable USCI_B0
}

static unsigned char i2c_wait_tx(void) {
	while (UCB0CTL1 & UCTXSTP) { }
	while (!(IFG2 & UCB0TXIFG)) { }
	return 0;
}

unsigned char i2c_write_byte(unsigned char address, unsigned char data) {
	UCB0I2CSA = address;
	UCB0CTL1 |= UCTR | UCTXSTT; // transmitter, start
	while (UCB0CTL1 & UCTXSTT) { }
	if (UCB0STAT & UCNACKIFG) {
		UCB0CTL1 |= UCTXSTP;
		UCB0STAT &= ~UCNACKIFG;
		return 1;
	}
	UCB0TXBUF = data;
	i2c_wait_tx();
	UCB0CTL1 |= UCTXSTP;
	while (UCB0CTL1 & UCTXSTP) { }
	return 0;
}

unsigned char i2c_write_bytes(unsigned char address, const unsigned char *data, unsigned int length) {
	UCB0I2CSA = address;
	UCB0CTL1 |= UCTR | UCTXSTT;
	while (UCB0CTL1 & UCTXSTT) { }
	if (UCB0STAT & UCNACKIFG) {
		UCB0CTL1 |= UCTXSTP;
		UCB0STAT &= ~UCNACKIFG;
		return 1;
	}
	for (unsigned int i = 0; i < length; i++) {
		UCB0TXBUF = data[i];
		i2c_wait_tx();
	}
	UCB0CTL1 |= UCTXSTP;
	while (UCB0CTL1 & UCTXSTP) { }
	return 0;
}

