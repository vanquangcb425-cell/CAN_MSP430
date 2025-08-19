MSP430G2553 HX711 Scale with I2C LCD
====================================

Features
--------
- HX711 24-bit ADC for load cell (1 kg)
- I2C LCD (HD44780 via PCF8574)
- Units: grams (g)
- One push-button to start/stop the weighing program
- Intro screen before the button is pressed:
  - "GVHD: NGUYEN DINH TU"
  - "SVTH: PHAM THANH TU, DANG VAN QUANG"

Wiring (default pins)
---------------------
- HX711:
  - DT -> P2.1
  - SCK -> P2.0
  - VCC 3.3V, GND
- I2C LCD (PCF8574 @ 0x27 typical):
  - SDA -> P1.7
  - SCL -> P1.6
  - VCC 5V (or 3.3V variant), GND
- Button (active low with pull-up):
  - Button -> P1.3 (with internal pull-up enabled)

Build
-----
Requires msp430-gcc toolchain.

```bash
make
```

Flash (example with mspdebug)
-----------------------------
```bash
mspdebug rf2500 "prog build/scale.elf"
```

Calibration
-----------
1. Power on, do not press the button yet. Place no weight on the load cell.
2. Press and hold the button for 2 seconds to tare and enter weighing.
3. Put a known mass (e.g. 500 g) and read the raw value (temporarily enable DEBUG define or use serial if added). Alternatively, use the on-screen value and adjust SCALE_FACTOR in `src/hx711.h` until the reading matches grams.
4. Save the final calibration in `SCALE_FACTOR`.

Notes
-----
- Ensure HX711 and LCD share ground with MSP430.
- If your LCD I2C address is different (0x3F, etc.), update `LCD_I2C_ADDRESS` in `src/lcd_i2c.h`.
- If using a 5V LCD backpack, ensure I2C lines are 5V tolerant or use level shifting; many PCF8574 boards work with 3.3V.

# CAN_MSP430