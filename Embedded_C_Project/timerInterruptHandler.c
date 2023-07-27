
#define F_CPU 16000000UL
/*
 * timerInterruptHandler.c
 *
 * Created: 2023-07-17 13:51:26
 *  Author: David
 */
#include "ledSegmentSingle.h"

uint16_t tick = 0;

// Button and Debounce
uint8_t buttonPushed = 0;
uint8_t debounceTime = 0;
uint8_t countTimeSwitch = 0;

// LED Segment Display
volatile uint8_t numberCodeIndex = 0;
volatile uint8_t numberLogIndex  = 0;
uint8_t numberCode[]    = {
	0b11100111, // 0
	0b00100010, // 1
	0b11001011, // 2
	0b01101011, // 3
	0b00101110, // 4
	0b01101101, // 5
	0b11101101, // 6
	0b00100011, // 7
	0b11101111, // 8
	0b01101111, // 9
};
volatile uint8_t LED_NUMBER   = 0;
/*
	0bit: N     LSB
	1bit: NE
	2bit: NW
	3bit: M
	4bit: POINT
	5bit: SE
	6bit: S
	7bit: SW    MSB
*/

// LED Segment Display
uint8_t colorCode[] = { 
	0b00000001, // RED
	0b00000011, // YELLOW
	0b00000010, // GREEN
	0b00000110, // CYAN
	0b00000100, // BLUE
	0b00000101  // MAGENTA
};

// Length = 140
uint8_t BLU_PWM[] = {  1,   4,  10,  18,  28,  39,  52,  66,  81,  96, 111, 125, 139, 152, 163, 173, 181, 187, 190, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 190, 187, 181, 173, 163, 152, 139, 125, 111,  96,  80,  66,  52,  39,  28,  18,  10,   4,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
uint8_t GRE_PWM[] = {  1,   4,  10,  18,  28,  39,  52,  66,  81,  96, 111, 125, 139, 152, 163, 173, 181, 187, 190, 192, 192, 190, 187, 181, 173, 163, 152, 139, 125, 111,  96,  80,  66,  52,  39,  28,  18,  10,   4,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   4,  10,  18,  28,  39,  52,  66,  81,  96, 111, 125, 139, 152, 163, 173, 181, 187, 190, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 190, 187, 181, 173, 163, 152, 139, 125, 111,  96,  80,  66,  52,  39,  28,  18,  10,   4,   1};
uint8_t RED_PWM[] = {  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   4,  10,  18,  28,  39,  52,  66,  81,  96, 111, 125, 139, 152, 163, 173, 181, 187, 190, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 190, 187, 181, 173, 163, 152, 139, 125, 111,  96,  80,  66,  52,  39,  28,  18,  10,   4,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
	
/* Misc */
volatile uint8_t GPUI8 = 0;
volatile uint8_t pwmInterruptCounter = 0;
volatile uint8_t nonInterruptAsyncIncrementor = 0;
extern uint16_t LED_4_DIGIT_NUMBER;

/* Interrupt Service Routines */
ISR(TIMER0_COMPA_vect) {
	tick++;
	if(tick % 128 == 0) {
		tick = 0;
		OCR1A = BLU_PWM[pwmInterruptCounter]; // BLUE
		OCR3A = RED_PWM[pwmInterruptCounter]; // RED
		OCR4A = GRE_PWM[pwmInterruptCounter]; // GREEN
		pwmInterruptCounter = (pwmInterruptCounter + 1) % 140;
		//LED_4_DIGIT_NUMBER = (LED_4_DIGIT_NUMBER + 1) % 10000;
	}
	if(tick % 4 == 0) {
		
		writeDigit(&numberCode[getDigitFromIntger(LED_4_DIGIT_NUMBER * (uint16_t)(1.422 * 3300.0/1023.0), (3 - numberLogIndex))]);
		activateDigitSlot(numberLogIndex);
		numberLogIndex = (numberLogIndex + 1) % 4;
	}
	/*debounceTime += countTimeSwitch;
	if(debounceTime == 2) {
		debounceTime = 0;
		countTimeSwitch = 0;
		if(!(PINB & _BV(PINB1))) PORTB ^= _BV(PB7); // Toggle built-in LED
		writeDigit(&numberCode[(numberCodeIndex++ >> 1) % 10]);
	}*/
}

ISR(TIMER4_COMPA_vect) {
	//PORTB |= _BV(PB5);
}
ISR(TIMER3_COMPA_vect) {
	//PORTB |= _BV(PB5);
}
ISR(TIMER1_COMPA_vect) {
	//PORTB |= _BV(PB5);
}