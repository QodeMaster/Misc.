/*
 * timerInit.c
 *
 * Created: 2023-07-17 14:48:56
 *  Author: David
 */
#include "timerInit.h"

void timer1_init_PWM() {
	TIMSK1 |= _BV(OCIE1A);				   // Enable timer interrupt for compare mode pg 162
	TCCR1A |= _BV(WGM11);                  // Fast PWM, pg 145, TOP = ICRn
	TCCR1B |= (_BV(WGM12) | _BV(WGM13));
	
	TCCR1A |= (_BV(COM1A1) | _BV(COM1A0)); // Set OC1A on compare match, pg 155
	TCCR1B |= _BV(CS11);				   // Prescaler DIV8 CLK, pg 157
	
	ICR1    = 199;						   // 1 + TOP = 200
	OCR1A   = 0;						   // Downtime from Duty cycle in range [0, 199]
}

void timer3_init_PWM() {
	TIMSK3 |= _BV(OCIE3A);				   // Enable timer interrupt for compare mode pg 162
	TCCR3A |= _BV(WGM31);                  // Fast PWM, pg 145, TOP = ICRn
	TCCR3B |= (_BV(WGM32) | _BV(WGM33));
	
	TCCR3A |= (_BV(COM3A1) | _BV(COM3A0)); // Set OC1A on compare match, pg 155
	TCCR3B |= _BV(CS31);				   // Prescaler DIV8 CLK, pg 157
	
	ICR3    = 199;						   // 1 + TOP = 200
	OCR3A   = 0;						   // Downtime from Duty cycle in range [0, 199]
}

void timer4_init_PWM() {
	TIMSK4 |= _BV(OCIE4A);				   // Enable timer interrupt for compare mode pg 162
	TCCR4A |= _BV(WGM41);                  // Fast PWM, pg 145, TOP = ICRn
	TCCR4B |= (_BV(WGM42) | _BV(WGM43));
	
	TCCR4A |= (_BV(COM4A1) | _BV(COM4A0)); // Set OC1A on compare match, pg 155
	TCCR4B |= _BV(CS41);				   // Prescaler DIV8 CLK, pg 157
	
	ICR4    = 199;						   // 1 + TOP = 200
	OCR4A   = 0;						   // Downtime from Duty cycle in range [0, 199]
}

void timer0_init() {
	TIMSK0 |= _BV(OCIE0A); // Enable timer interrupt for compare mode
	OCR0A   = 249;		   // In a ms 150 ticks will pass in a Timer0/64 CLK
	TCCR0A |= _BV(WGM01);  // Clear ticks, CTC
	TCCR0B |= (_BV(CS01) | _BV(CS00));  // prescaler DIV64 CLK
}