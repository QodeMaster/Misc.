/*
 * led.h
 *
 * Created: 2023-07-05 13:25:33
 *  Author: David
 */ 
#include "GLOBAL_VARIABLES.h"

#ifndef LED_H_
#define LED_H_

void turnLedOn(volatile uint8_t * adr, int bit);
void turnLedOff(volatile uint8_t * adr, int bit);


#endif /* LED_H_ */