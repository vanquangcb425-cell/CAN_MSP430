PROJECT := msp430_scale
MCU := msp430g2553
SRC := src/main.c
BUILD_DIR := build

# Detect MSP430 toolchain explicitly (avoid host cc)
TOOLCHAINS := msp430-elf-gcc msp430-gcc
MSPCC := $(firstword $(foreach t,$(TOOLCHAINS),$(if $(shell command -v $(t) 2>/dev/null),$(t),)))

# Derive objcopy from compiler choice
ifeq ($(findstring msp430-elf-gcc,$(MSPCC)),msp430-elf-gcc)
  MSPOBJCOPY := msp430-elf-objcopy
else ifeq ($(findstring msp430-gcc,$(MSPCC)),msp430-gcc)
  MSPOBJCOPY := msp430-objcopy
else
  MSPOBJCOPY := msp430-objcopy
endif

CFLAGS := -mmcu=$(MCU) -Os -ffunction-sections -fdata-sections -fno-common -Wall -Wextra -std=c99
LDFLAGS := -Wl,-gc-sections

ELF := $(BUILD_DIR)/$(PROJECT).elf
HEX := $(BUILD_DIR)/$(PROJECT).hex

.PHONY: all clean flash

all: $(HEX)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(ELF): $(SRC) | $(BUILD_DIR)
	@if [ -z "$(MSPCC)" ]; then \
		echo "No MSP430 GCC toolchain found. Install one of:"; \
		echo "  - Ubuntu/Debian: sudo apt-get install gcc-msp430 binutils-msp430 msp430mcu"; \
		echo "  - Or install xPack msp430-elf-gcc and ensure msp430-elf-gcc is in PATH"; \
		exit 1; \
	fi
	$(MSPCC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

$(HEX): $(ELF)
	$(MSPOBJCOPY) -O ihex $< $@

clean:
	rm -rf $(BUILD_DIR)

# Example flash target using mspdebug (adjust interface)
flash: $(HEX)
	mspdebug rf2500 'prog $(HEX)'

