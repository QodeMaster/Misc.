#include "system_sam3x.h"  
#include "at91sam3x8.h" 
unsigned int i = 0; 
unsigned int clockCounter = 0; 
unsigned int interruptOccurred = 0; 

void SysTick_Handler(void); 
void ReadButton(unsigned int *nButton); 
void Set_Led(unsigned int nLed); 
void PIOD_Handler(void); 

#define AT91C_PIOD_AIMER (AT91_CAST(AT91_REG *) 	0x400E14B0) 
#define AT91C_PIOD_DIFSR (AT91_CAST(AT91_REG *) 	0x400E1484) 
#define AT91C_PIOD_SCDR (AT91_CAST(AT91_REG *) 	0x400E148C) 

int main() { 
  SystemInit(); // Disables the Watchdog and setup the MCK  
  SysTick_Config(SystemCoreClock * .001); // 84MHz * 0.001, 84000 ticks/sec <=> 1000 interrupt/sec 
  
  *AT91C_PMC_PCER = (1<<14); 
  *AT91C_PIOD_PER = (5<<1); 
  *AT91C_PIOD_OER = (1<<3); 
  *AT91C_PIOD_ODR = (1<<1); 
  *AT91C_PIOD_PPUDR = (5<<1); 

  *AT91C_PIOD_AIMER = (1<<1); // Additional Interrupt Modes Enable Register, 1st bit 
  *AT91C_PIOD_IFER  = (1<<1); // Input Filter Enable Register 
  *AT91C_PIOD_DIFSR = (1<<1); // Debouncing Input Filtering Select Register 
  *AT91C_PIOD_SCDR  = (1<<1); // Tune value for sweetspot, debouncing measurement 

  *AT91C_PIOD_ISR;            // "Read" ISR to clear it 

  NVIC_ClearPendingIRQ(PIOD_IRQn); // Enable IO controller 14, PIOD_IRQn = 14 
  NVIC_SetPriority(PIOD_IRQn, 1);  // Set priority to first 
  NVIC_EnableIRQ(PIOD_IRQn);       // Enable the interrupt 

  *AT91C_PIOD_IER   = (1<<1); // Enable Interrupts on pin 26, our button 

  while(1) 
  { 
    ReadButton(&i); 
    //Set_Led(i); 
  }// while(1)  
} // main() 


void ReadButton(unsigned int *nButton) { 
  if((*AT91C_PIOD_PDSR & 0x2) == 0x2) { *nButton = 0; }  
  else  { *nButton = 1; } 
} 

void Set_Led(unsigned int nLed) { 
  //*AT91C_PMC_PCER = (1<<14); 
  //*AT91C_PIOD_PER = (1<<3); 
  //*AT91C_PIOD_ODR = (1<<3); 
  //*AT91C_PIOD_PPUDR = (1<<3); 
  if(nLed == 0) { *AT91C_PIOD_CODR = (1<<3); } 
  else { *AT91C_PIOD_SODR = (1<<3); } 
} 

void SysTick_Handler(void){ 
  if(interruptOccurred == 1) { 
    clockCounter++;                              // Counting every ms 
    Set_Led((clockCounter % 1000) < 50 ? 1 : 0); // Turns on LED every sec and keeps it on for 50ms 
  } 
} 

void PIOD_Handler(void){ 
  interruptOccurred = ((*AT91C_PIOD_ISR & 0x2) &&(interruptOccurred == 0) ? 1 : 0); 
} 
