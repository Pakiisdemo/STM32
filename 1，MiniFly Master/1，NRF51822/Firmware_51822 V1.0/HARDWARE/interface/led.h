#ifndef __LED_H
#define __LED_H

#include "pinout.h"
#include "nrf_gpio.h"

#define LED_INIT()		nrf_gpio_cfg_output(LED_PIN)
#define LED_ON() 		nrf_gpio_pin_set(LED_PIN)
#define LED_OFF() 		nrf_gpio_pin_clear(LED_PIN)
#define LED_TOGGLE() 	nrf_gpio_pin_toggle(LED_PIN)


#endif //__LED_H
