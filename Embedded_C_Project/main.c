/*
 * GccApplication1.c
 *
 * Created: 2023-06-30 18:26:25
 * Author : Yousef
 */ 
#include "ledSegmentSingle.h"
#include "timerInit.h"

/* Interrupt Service Routines */
//extern uint8_t countTimeSwitch;
//ISR(PCINT0_vect) { countTimeSwitch = 1; }
	
uint16_t LED_4_DIGIT_NUMBER = 0;
ISR(ADC_vect) { 
	uint8_t adcl = ADCL;
	uint8_t adch = ADCH;
	
	int diff = (LED_4_DIGIT_NUMBER - ((adch << 8) | adcl)) * (LED_4_DIGIT_NUMBER - ((adch << 8) | adcl));
	
	if(625 < diff) { 
		LED_4_DIGIT_NUMBER = (adch << 8) | adcl; 
	}
	//LED_4_DIGIT_NUMBER = (adch << 8) | adcl;
	//LED_4_DIGIT_NUMBER = (LED_4_DIGIT_NUMBER + 1) % 10000;
	ADCSRA |= _BV(ADSC);
}
	
extern volatile uint8_t numberCodeIndex;
extern uint8_t numberCode[];
	
void pinChange_init();
void ACD_Init();

int main(void) {
	//DDRC |= _BV(DDC0);  // Make pin 37 output
	//DDRB |= _BV(DDB7);  // Activate built-in LED
	//DDRB &= ~_BV(DDB1); // Make pin 52 input
	
	/* LED display pins */
	DDRL |= _BV(DDL4);  // Make pin 45 output N
	DDRL |= _BV(DDL2);  // Make pin 47 output NE
	DDRC |= _BV(DDC3);  // Make pin 34 output NW
	DDRA |= _BV(DDA6);  // Make pin 28 output M
	
	DDRC |= _BV(DDC4);  // Make pin 33 output POINT
	DDRL |= _BV(DDL5);  // Make pin 44 output SE
	DDRG |= _BV(DDG1);  // Make pin 40 output S
	DDRC |= _BV(DDC5);  // Make pin 32 output SW
	
	// 4 digit ctrl
	DDRB |= _BV(DDB0);  // Make pin 53 output D1
	DDRL |= _BV(DDL0);  // Make pin 49 output D2
	DDRB |= _BV(DDB2);  // Make pin 51 output D3
	DDRA |= _BV(DDA0);  // Make pin 22 output D4
	
	/* KY-009 RGB Full Color LED SMD ---- PWM pins */
	DDRB |= _BV(DDB5);  // Make pin 11 output B
	DDRE |= _BV(DDE3);  // Make pin 5  output E
	DDRH |= _BV(DDH3);  // Make pin 6  output H
	
	timer4_init_PWM();
	timer3_init_PWM();
	timer1_init_PWM();
	timer0_init();
	ACD_Init();
	
	sei();			    // SREG 7th bit

	ADCSRA |= _BV(ADSC);
    while (1) {
		//GPUI8 = colorCode[nonInterruptAsyncIncrementor % 6];
		//setColor((GPUI8 >> 0) & 1, (GPUI8 >> 1) & 1, (GPUI8 >> 2) & 1);
		//nonInterruptAsyncIncrementor++;
		//writeDigit(&numberCode[numberCodeIndex++ % 10]);
		//_delay_ms(2000);
		// ADMUX ADCSRA
    }
	
	return 0;
}

void pinChange_init() {
	PCMSK0 |= _BV(PCINT1); // Enable pin interrupt
	PCICR  |= _BV(PCIE0);  // Enable port 0 interrupt
}

void ACD_Init() {
	ADCSRA |= _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
	ADMUX  |= _BV(REFS0);
}

void writeDigit(volatile uint8_t * LED_NUM) {
	// reg_data = (reg_data & (~bit_mask)) | (new_value<<5)
	PORTL = (PORTL & ~_BV(PL4)) | (((*LED_NUM & _BV(0)) >> 0) << PL4);
	PORTL = (PORTL & ~_BV(PL2)) | (((*LED_NUM & _BV(1)) >> 1) << PL2);
	PORTC = (PORTC & ~_BV(PC3)) | (((*LED_NUM & _BV(2)) >> 2) << PC3);
	PORTA = (PORTA & ~_BV(PA6)) | (((*LED_NUM & _BV(3)) >> 3) << PA6);
	PORTC = (PORTC & ~_BV(PC4)) | (((*LED_NUM & _BV(4)) >> 4) << PC4);
	PORTL = (PORTL & ~_BV(PL5)) | (((*LED_NUM & _BV(5)) >> 5) << PL5);
	PORTG = (PORTG & ~_BV(PG1)) | (((*LED_NUM & _BV(6)) >> 6) << PG1);
	PORTC = (PORTC & ~_BV(PC5)) | (((*LED_NUM & _BV(7)) >> 7) << PC5);
}

void activateDigitSlot(int index) {
	switch(index) {
		// Active low activation
		case 0:
			// Activate D1
			PORTB &= ~_BV(PB0);  // D1
			PORTL |=  _BV(PL0);  // D2
			PORTB |=  _BV(PB2);  // D3
			PORTA |=  _BV(PA0);  // D4
			break;
		case 1:
			// Activate D2
			PORTB |=  _BV(PB0);  // D1
			PORTL &= ~_BV(PL0);  // D2
			PORTB |=  _BV(PB2);  // D3
			PORTA |=  _BV(PA0);  // D4
			break;
		case 2:
			// Activate D3
			PORTB |=  _BV(PB0);  // D1
			PORTL |=  _BV(PL0);  // D2
			PORTB &= ~_BV(PB2);  // D3
			PORTA |=  _BV(PA0);  // D4
			break;
		case 3:
			// Activate D4
			PORTB |=  _BV(PB0);  // D1
			PORTL |=  _BV(PL0);  // D2
			PORTB |=  _BV(PB2);  // D3
			PORTA &= ~_BV(PA0);  // D4
			break;
		default:
			// Activate All
			PORTB &= ~_BV(PB0);  // D1
			PORTL &= ~_BV(PL0);  // D2
			PORTB &= ~_BV(PB2);  // D3
			PORTA &= ~_BV(PA0);  // D4
	}
}

int getDigitFromIntger(uint16_t number, int spot) {
	
	while(spot) {
		number /= 10;
		spot--;
	}
	
	return number % 10;
}

void write4DigitNum(uint16_t number, uint8_t digitCount) {
	while(digitCount) {
		
		digitCount--;
	}
}

void setColor(uint8_t RED, uint8_t GREEN, uint8_t BLUE) {
	PORTB = (PORTB & ~_BV(PB2)) | (RED   << PB2);
	PORTB = (PORTB & ~_BV(PB0)) | (GREEN << PB0);
	PORTL = (PORTL & ~_BV(PL0)) | (BLUE  << PL0);
}
