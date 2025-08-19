MSP430G2553 Electronic Scale (HX711 + 1kg Load Cell + I2C LCD)

Overview

This firmware runs on MSP430G2553 and reads a 1 kg load cell via HX711, displaying weight in grams on a 16x2 I2C LCD (PCF8574 expander). A push button toggles between an idle greeting screen and weighing mode. Upon entering weighing mode, the system performs a tare (zero) automatically.

Features

- Idle greeting shows:
  - "GVHD: NGUYEN DINH TU"
  - "SVTH: PHAM THANH TU, DANG VAN QUANG" (marquee scroll if longer than 16 chars)
- Single button toggles idle/active
- Auto-tare on start of weighing
- Weight in grams (rounded)

Hardware

- MCU: MSP430G2553 (LaunchPad)
- ADC: HX711
- Load cell: 1 kg
- LCD: 16x2 with I2C backpack (PCF8574); default address 0x27 (change via LCD_I2C_ADDR)

Pin Mapping

- HX711
  - P2.0 -> HX711 SCK (PD_SCK)
  - P2.1 -> HX711 DOUT
- I2C LCD (USCI_B0)
  - P1.6 -> SCL
  - P1.7 -> SDA
- Button (LaunchPad S2)
  - P1.3 -> Button (active low, pull-up)

Build

- Clock: 1 MHz DCO
- I2C: 100 kHz
- Files: `src/main.c`

Calibration

1. Flash firmware.
2. With no weight, enter weighing mode; the device auto-tares.
3. Place a known weight (e.g., 500 g).
4. Read displayed grams value G. Compute calibration constant:
   CALIBRATION_SCALE = (raw - offset) / grams
   In this code, adjust `#define CALIBRATION_SCALE` in `src/main.c` until display matches. A simple method:
   - If display reads 420 g for a 500 g mass, multiply CALIBRATION_SCALE by (420/500) to increase sensitivity.

Configuration

- Change `LCD_I2C_ADDR` (0x27 or 0x3F common)
- Adjust `CALIBRATION_SCALE` after calibration
- Tune `HX711_SAMPLE_AVG` for smoothing vs responsiveness

Notes

- The LCD uses a common PCF8574 bit map: P0=RS, P1=RW, P2=EN, P3=BL, P4..P7=D4..D7.
- Ensure HX711 and MCU share ground; power HX711 from 3.3V.
- Debounce for the button is 200 ms.

