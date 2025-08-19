MCU=msp430g2553
CC=msp430-gcc
OBJCOPY=msp430-objcopy
CFLAGS= -mmcu=$(MCU) -Os -Wall -Wextra -std=c11 -ffunction-sections -fdata-sections
LDFLAGS= -Wl,-gc-sections

SRCS= src/main.c \
      src/i2c.c \
      src/lcd_i2c.c \
      src/hx711.c

OBJS=$(SRCS:.c=.o)

TARGET=build/scale.elf
HEX=build/scale.hex

all: $(TARGET) $(HEX)

build:
	mkdir -p build

$(TARGET): build $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(HEX): $(TARGET)
	$(OBJCOPY) -O ihex $< $@

clean:
	rm -rf build $(OBJS)

.PHONY: all clean

