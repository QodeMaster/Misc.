/*
 * ledSegmentSingle.h
 *
 * Created: 2023-07-07 19:56:21
 *  Author: David
 */ 
#include "led.h"

#ifndef LEDSEGMENTSINGLE_H_
#define LEDSEGMENTSINGLE_H_

void writeDigit(volatile uint8_t * LED_NUM);
void setColor(uint8_t RED, uint8_t GREEN, uint8_t BLUE);
void activateDigitSlot(int index);
int getDigitFromIntger(uint16_t number, int spot);

#endif /* LEDSEGMENTSINGLE_H_ */