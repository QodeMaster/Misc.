#include "system_sam3x.h"     
#include "at91sam3x8.h" 
#include <math.h> 

unsigned int i = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag 
unsigned int LIGHT_INTERRUPT_FLAG      = 0; // Defining global flag 
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;   

short trigger  = 0;
int p = 0;

unsigned int PRESCAL   = 2; 
double light1 = 0, light2 = 0; 
unsigned int status  = 0; 

int value = 0;   
int colOrder[] = {8, 9, 7};   
int rowOrder[] = {5, 2, 3, 4}; 

// [0, 1, 2..., 9] 
char digits[] =  
{0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A};   
// Plus with one on the lines below to get the characters value

void Init_Display(void); 
int returnKeypadValue(short *trigger); // Returns the keypad-value 
void Write_Data_2_Display(unsigned char Data); 
void Write_Command_2_Display(unsigned char Command); 
unsigned char Read_Status_Display();

void DisplayKeyboard(int inp);
void Delay(int Value);
void clearDisplay(int x, int y);
void locationOnDisplay(int x,int y);
void display(char ch[]);
void displayInt(int inp);
int getDigit(int spot, int n){return (int)((n % (int)(pow(10,spot+1))) / pow(10,spot));}

void CreatePulseInPin2(int pulseWidth);   
void tempInit(void);   
void StartMeasureTemp(void);   
void TC0_Handler(void);

void ADC_Handler(void); 
void lightInit(void); 
void StartMeasureLight(void);

int degree = 0; 
double currentDegree = 0; 
void Serv_Rotate(double degree); 
void servoInit(void); 

int main() {    
  SystemInit(); // Disables the Watchdog and setup the MCK 
  
  *AT91C_PIOC_OER   = (1<<6) | (255 << 12);  
   Write_Command_2_Display(0x90); 
   Init_Display(); 
   Write_Data_2_Display(0); 
   Write_Data_2_Display(0); 
   Write_Command_2_Display(0x24); 
   Delay(300000); 
   clearDisplay(0x00,0x00);
   
  // Uncomment for use
  //tempInit();  
  //StartMeasureTemp();
  //StartMeasureLight();
  //lightInit(); 
  servoInit(); 
  while(1){  
    // If flag read TC0_RB0 and TC0_RA0  
    if(0 && TEMPRATURE_INTERRUPT_FLAG == 1) { // Condition always false for debugging purposes
      //StartMeasureTemp();  
      TCA = *AT91C_TC0_RA;   
      TCB = *AT91C_TC0_RB;  

      deltaT = TCB - TCA;  
      // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
      if(temprature != (deltaT / 210) - 273) {
        // To prevent the screen from changing if the temprature hasn't changed
        temprature = (deltaT / 210) - 273;
        display("Temprature: ");
        displayInt(temprature);
      }
      TEMPRATURE_INTERRUPT_FLAG = 0; 
      StartMeasureTemp();
    }
    
    if(0) { StartMeasureLight(); }  // Condition always false for debugging purposes
    if(LIGHT_INTERRUPT_FLAG == 1) { 
        if((status & 0x4) == 0x4) {  
          //light2 = *AT91C_ADCC_CDR2; 
          light2 = (double)(*AT91C_ADCC_CDR2&0x3ff)*(3.3/4096); 
        } 
        if((status & 0x2) == 0x2) { 
          //light1 = *AT91C_ADCC_CDR1; 
          light1 = (double)(*AT91C_ADCC_CDR1&0x3ff)*(3.3/4096); 
        } 
      LIGHT_INTERRUPT_FLAG = 0;   // Setting global flag  
    }
    //Keypad
    p = returnKeypadValue(&trigger); // p = keypad value 
    if(trigger == 1) {
      DisplayKeyboard(p);   // Displays the keypad value on the screen 
      Delay(10000000);
      trigger = 0;
      Serv_Rotate(10* (value == 11 ? 0 : value));
    } else { continue; }

  }// while(1)     
} // main()    
   
void tempInit() {   
  *AT91C_PMC_PCER = (1 << 12) | (1 << 27); // Enable peripheral PIOB and TC0  

  *AT91C_TC0_CMR  = (*AT91C_TC0_CMR & 0xFFFE); 
  *AT91C_TC0_CCR  = (5<<0); 

  *AT91C_TC0_CMR  = (*AT91C_TC0_CMR | (2<<16)); 
  *AT91C_TC0_CMR  = (*AT91C_TC0_CMR | (1<<18)); 

  *AT91C_PIOB_PER = (1 << 25);     // pin 2, TC0, TIOA0   
  *AT91C_PIOB_CODR = (1 << 25); 

  // Create an inital pulse, 4.6 ms < t_RESET < 16ms   
  CreatePulseInPin2(672000);       // Pulse width, 672000cycles = 8ms, t_RESET = 8ms  

  *AT91C_TC0_SR;             // Clearing old interrupts   

  NVIC_ClearPendingIRQ(TC0_IRQn); // Enable IO controller 27, TC0_IRQn = 27    
  NVIC_SetPriority(TC0_IRQn, 1);  // Set priority to first    
  NVIC_EnableIRQ(TC0_IRQn);       // Enable the interrupt   
}   

void Delay(int Value) { 
  int i;     
  for(i=0;i<Value;i++)     
    asm("nop");  
} 

void StartMeasureTemp(void) {  
  CreatePulseInPin2(336000); // Create a reset pulse, 8ms long  
  //Create a startpuls with a Delay(25)   
  CreatePulseInPin2(25);  

  //*AT91C_TC0_CCR  = (*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4;// SW reset, TC0_CCR0 
  *AT91C_TC0_CCR  = ((*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4);// SW reset, TC0_CCR0   
  *AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IER = (1 << 6); // Enable interrupt for LDRBS   
} 

void CreatePulseInPin2(int pulseWidth) {   
  *AT91C_PIOB_OER  = (1 << 25); // Make pin 2 an output   
  *AT91C_PIOB_CODR = (1 << 25); // Put a low signal on the I/O line   

  Delay(pulseWidth); 
  *AT91C_PIOB_SODR = (1 << 25); // Put a high signal on the I/O line   
  *AT91C_PIOB_ODR  = (1 << 25); // Make pin 2 an input   
} 

void TC0_Handler() {   
  *AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IDR = (1 << 6); // Disable interrupt for LDRBS   
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void Init_Display(void) { 
  *AT91C_PIOD_CODR = 1; 
  Delay(10);  
  *AT91C_PIOD_SODR = 1; 

  Write_Data_2_Display(0x00);  
  Write_Data_2_Display(0x00);  
  Write_Command_2_Display(0x40);//Set text home address  
  Write_Data_2_Display(0x00);  
  Write_Data_2_Display(0x40);  
  Write_Command_2_Display(0x42); //Set graphic home address  
  Write_Data_2_Display(0x1e);  
  Write_Data_2_Display(0x00);  
  Write_Command_2_Display(0x41); // Set text area  
  Write_Data_2_Display(0x1e);  
  Write_Data_2_Display(0x00);  
  Write_Command_2_Display(0x43); // Set graphic area  
  Write_Command_2_Display(0x80); // text mode  
  Write_Command_2_Display(0x94); // Text on graphic off  
} 

void Write_Data_2_Display(unsigned char Data) { 
  while((Read_Status_Display()) & (12) != 12){} 
  *AT91C_PIOC_OER  = (255<<2);               // output enable pin 34 - 41 
  *AT91C_PIOC_CODR = (255<<2);               // clear databus 
  *AT91C_PIOC_SODR = ((unsigned int)Data)<<2;

  *AT91C_PIOC_CODR = (3<<12);
  *AT91C_PIOC_OER  = (255<<2); // output enable pin 34 - 41 
  *AT91C_PIOC_CODR = (3<<14); // clear C/D, clear CE
  *AT91C_PIOC_CODR = (1<<17); // clear write display 
  Delay(1000);

  *AT91C_PIOC_SODR = (1<<15); // set chip enable display 
  *AT91C_PIOC_SODR = (1<<17); // set write display 
  *AT91C_PIOC_SODR = (1<<12); // disable output for OE 
  *AT91C_PIOC_ODR  = (255<<2); // output disable pin 34 - 41 
} 

void Write_Command_2_Display(unsigned char Command){ 

  while((Read_Status_Display()) & (0xC) != 0xC){} 

    *AT91C_PIOC_OER  = (255<<2); // output enable pin 34 - 41 
    *AT91C_PIOC_CODR = (255<<2); // clear databus 

    *AT91C_PIOC_SODR = ((unsigned int)Command)<<2; 
    *AT91C_PIOC_CODR = (3<<12); // enabel output (clear OE) 
    *AT91C_PIOC_OER  = (255<<2);// output enable pin 34 - 41 
    *AT91C_PIOC_SODR = (1<<14); // set C/D 
    *AT91C_PIOC_CODR = (5<<15); // clear CE chip select display & clr wr disp

    Delay(2000); // Originally Delay(20000);

    *AT91C_PIOC_SODR = (1<<17); // set write display 
    *AT91C_PIOC_SODR = (9<<12); // disable output (set OE) 
    *AT91C_PIOC_ODR = (255<<2); // output disable pin 34 - 41 
} 

unsigned char Read_Status_Display() { 
    unsigned char temp; 

    *AT91C_PMC_PCER=(3<<13);      // Port C, D

    *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
    *AT91C_PIOC_PPUDR = 0xffffffff; 
    *AT91C_PIOD_PER   = 1; 
    *AT91C_PIOD_PPUDR = 1; 
    *AT91C_PIOC_ODR   = (255<<2); // output disable pin 34 - 41 
    *AT91C_PIOC_OER   = 0xFF;     // output enable pin 44 - 51
    *AT91C_PIOC_SODR  = (1<<13);  //  DIR, pin 50 
//    *AT91C_PIOD_OER   = 1;        // output enable pin 25 
    *AT91C_PIOC_CODR  = (1<<12);    //clear OE (enabel the chip) 
    *AT91C_PIOC_SODR  = (1<<14);    // set C/D 
    *AT91C_PIOC_CODR  = (3<<15);    //clear CE chip select display, read display 

    Delay(10); 

    temp=((*AT91C_PIOC_PDSR)&(255<<2)); 

    *AT91C_PIOC_SODR  = (3<<15); // set CE chip select display, Read screen
    (*AT91C_PIOC_SODR)= (1<<12); // set OE (disable) the chip 
    *AT91C_PIOC_CODR  = (1<<13); // DIR 
    return temp; 
}   

void clearDisplay(int x, int y) {
  locationOnDisplay(x,y);

  for(int i=0;i<(30*16);i++) // clear display
  {
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xC0);
  }

  locationOnDisplay(0x00,0x00);
}

void locationOnDisplay(int x,int y) {
  Write_Data_2_Display(x);
  Write_Data_2_Display(y); 
  Write_Command_2_Display(0x24);
}

void display(char ch[]) {
  clearDisplay(0x00,0x00);
  //char temp;

  for(char m = 0; '\0' !=ch[m];m++) {
    char temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void displayInt(int inp) {
  int lg = (inp == 0 ? 0 : (int)log10(inp));
  for(int i = lg; 0 <= i; i--){
    int digit = getDigit(i, inp);
    Write_Data_2_Display(digits[digit]);
    Write_Command_2_Display(0xc0);
  }
}

void lightInit() { 
  *AT91C_PMC_PCER  = (1<<11);// Enabling peripheral PIOA 
  *AT91C_PMC_PCER1 = (1<<5); // ADC 
  *AT91C_PIOA_PER = (1 << 4); 
  *AT91C_PIOA_PER = (1 << 3); 

  *AT91C_PIOA_PPUDR = (1 << 4); 
  *AT91C_PIOA_PPUDR = (1 << 3); 

  *AT91C_PIOA_ODR = (1 << 4); 
  *AT91C_PIOA_ODR = (1 << 3); 

  *AT91C_ADCC_MR  = (*AT91C_ADCC_MR & 0xFFFF00FF) | (PRESCAL << 8);// Setting PRESCAL, sampling rate:14MHz 
  *AT91C_ADCC_CHER = (3<<1); // Enable channel 1 and 2 

  NVIC_ClearPendingIRQ(ADC_IRQn); // Enable IO controller 37, TC0_IRQn = 37    
  NVIC_SetPriority(ADC_IRQn, 1);  // Set priority to first    
  NVIC_EnableIRQ(ADC_IRQn);       // Enable the interrupt  

  *AT91C_ADCC_SR; 
} 

void StartMeasureLight(void) { 
  *AT91C_ADCC_CR   = (1<<1);          // Enabling ADC 
  *AT91C_ADCC_SR; 
  *AT91C_ADCC_IER = (3<<1);  // Enabling interrupt for Ch 1 and 2 
} 

void ADC_Handler() { 
  status = *AT91C_ADCC_IMR; 
  *AT91C_ADCC_IDR = (3<<1); 
  LIGHT_INTERRUPT_FLAG = 1;     // Setting global flag  
}

void servoInit() {   
  *AT91C_PMC_PCER  = (1<<12);// PIOB  
  *AT91C_PMC_PCER1 = (1<<4);// PWM   

  *AT91C_PIOB_PDR  = (1<<17);// Enable pin 62, Analog 8 
  *AT91C_PIOB_OER  = (1<<17); 
  *AT91C_PIOB_PPUDR =(1<<17); 

  //*AT91C_PIOB_ABMR = ((*AT91C_PIOB_ABMR & 0xFFDFFFF) | (1<<17));  
  *AT91C_PIOB_ABMR = (1<<17);  
  *AT91C_PWMC_ENA  = 0x2;  

  *AT91C_PWMC_CH1_CMR = ((*AT91C_PWMC_CH1_CMR & 0xFFFFFFF0) | 0x5);  
  //*AT91C_PWMC_CH1_CMR = *AT91C_PWMC_CH1_CMR | 0x5;  
  //*AT91C_PWMC_CH1_CMR = *AT91C_PWMC_CH1_CMR & (~(1<<8));  
  //*AT91C_PWMC_CH1_CPRDR= 0x3211620; // 20ms  
  *AT91C_PWMC_CH1_CPRDR= 0xCD14; // 20ms  
  //*AT91C_PWMC_CH1_CDTYR=  84000000/32000; //1ms 
  *AT91C_PWMC_CH1_CDTYR= 0xA41; //1ms 
}  

void Serv_Rotate(double degree) { 
  //*AT91C_PWMC_CH1_CDTYUPDR = 0xFFFFFF; 

  //if(degree > 180)     { degree = 180; } 
  //else if(degree <= 0) { degree = 0; }
  currentDegree = degree; 
  //*AT91C_PWMC_CH1_CDTYUPDR = (int)(1350+(28*(degree+1))); 
  if(degree <= 90) 
    *AT91C_PWMC_CH1_CDTYUPDR = (int)(1350+(28*(degree+1)));//(MCK*565)/32=1350 -> 0degree 
  else 
    *AT91C_PWMC_CH1_CDTYUPDR = (int)(1350+(28*(degree+1))); 
}

int returnKeypadValue(short* trigger) { 
   *AT91C_PMC_PCER   = (3<<13);                        // Enable control over PIOC and PIOD 
   *AT91C_PIOD_PER   = (1<<2);                         // Enable PIO on G, ~OE    
   *AT91C_PIOD_OER   = (1<<2);                         // Enable output on G, ~OE     
   *AT91C_PIOD_CODR  = (1<<2);                         // Clear G, ~OE    
   *AT91C_PIOD_PPUDR = (1<<2);                         // Disable pull-up resistance on port D bit 2    

   *AT91C_PIOC_PER   = (7<<7) | (15<<2);               // Enable PIO on the keyboard pins | 1110111100    
   *AT91C_PIOC_ODR   = (15<<2);                        // Disable output on ROWs so that it becomes an input   
   *AT91C_PIOC_OER   = (7<<7);                         // Enable all COLs outputs   
   *AT91C_PIOC_PPUDR = (7<<7) | (15<<2);               // Disable pull-up resistance on port C bit 2,3,4,5,7,8,9   
   *AT91C_PIOC_SODR  = (7<<7);                         // Setting all col pin as high   

     for(int k = 0; k < 3; k++)  
     { // Col loop   
      *AT91C_PIOC_CODR = (1<<colOrder[k]);  // Clear Col   
      for(int l = 0; l < 4; l++)  
      { // Row loop   
        if((*AT91C_PIOC_PDSR & (1<<rowOrder[l])) == 0) {   
          value = l * 3 + k + 1; 
          *trigger = 1; 
        }   
      }   
      *AT91C_PIOC_SODR = (1<<colOrder[k]);
   }
   *AT91C_PIOD_SODR  = (1<<2);                         // Set G, ~OE   
   *AT91C_PIOC_ODR   = (7<<7);                         // Disable all COLs outputs 
   return value; 
}

void DisplayKeyboard(int inp) {
  if(9 < inp){ 
    switch(inp) { 
    case 10: 
      Write_Data_2_Display(0x0A); 
      Write_Command_2_Display(0xc0); 
      break; 
    case 11: 
      Write_Data_2_Display(0x10); 
      Write_Command_2_Display(0xc0); 
      break; 
    case 12: 
      Write_Data_2_Display(0x03); 
      Write_Command_2_Display(0xc0); 
      break; 
    default: 
      Write_Data_2_Display(0x21); // If the input is not recognized the display will print char: "A" 
      Write_Command_2_Display(0xc0); 
      break; 
    } 
  } else {
    Write_Data_2_Display(digits[inp]);
    //Delay(2000000);
    Write_Command_2_Display(0xc0);
  }
}
