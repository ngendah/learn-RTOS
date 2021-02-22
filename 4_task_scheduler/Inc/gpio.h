#ifndef GPIO_H_
#define GPIO_H_
#include <stdint.h>

#define P13	13
#define P12	12

void enable_gpiod(void);

void configure_gpiod(uint8_t pin);

void toggle_gpiod(uint8_t pin);

#endif /* GPIO_H_ */
