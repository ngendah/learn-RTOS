#include "gpio.h"

void enable_gpiod(void){
	uint32_t *rcc_ahb1en = (uint32_t*) 0x40023830;
	*rcc_ahb1en |= (1 << 3);
}

void configure_gpiod(uint8_t pin){ //general purpose mode, only
	uint32_t *gpiod = (uint32_t*) 0x40020C00;
	*gpiod |= (1 << 2 *pin);
}

void toggle_gpiod(uint8_t pin) {
	uint32_t *gpiod = (uint32_t*) 0x40020C14;
	*gpiod ^= (1 << pin);
}
