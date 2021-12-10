#include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>

struct calender {
 int date[6];
 int decimalPointer;
 int sequence[14];
};

struct LinkedList { 
  int id; 
  double sensorData; 
  struct LinkedList *next; 
};

unsigned int i = 0;
short trigger  = 0; 
unsigned int INTERRUPT_FLAG = 0; // Defining global flag
int temprature = 0;
struct calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;

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

void Delay(int Value);  
void DisplayKeyboard(int inp); 
void displayInt(int number);
void clearDisplay(int x, int y);
void locationOnDisplay(int x,int y);
void display(char ch[]);
int getDigit(int spot, int n){return (int)((n % (int)(pow(10,spot+1))) / pow(10,spot));}

void assignDigitToCalender(int seq[]);
void assignToSeq(int inp);
void displayTime();
void addSecondToConfiguredCalender();

void CreatePulseInPin2(int pulseWidth);
void tempInit(void);
void StartMeasureTemp(void);
void TC0_Handler(void);

void SysTick_Handler(void);
/*   
27 G or ~OE, PD2      
36 ROW4   , PC4   
35 ROW3   , PC3   
34 ROW2   , PC2   
37 ROW1   , PC5   
39 COL3   , PC7    
41 COL2   , PC9   
40 COl1   , PC8 
*/ 

void main(void) {      
   SystemInit();
   SysTick_Config(SystemCoreClock * .01); // 84MHz * 1, 84000000 ticks/sec <=> 1 interrupt/sec
   
   //char ch=0x0;
   *AT91C_PIOC_OER   = (1<<6) | (255 << 12);  
   Write_Command_2_Display(0x90); 
   Init_Display(); 
   Write_Data_2_Display(0); 
   Write_Data_2_Display(0); 
   Write_Command_2_Display(0x24); 
   Delay(300000); 
   clearDisplay(0x00,0x00);
   
   tempInit();  

   while(1) {
     if(o == 79) {
       locationOnDisplay(0x28,0x00);
       display("Configure: DD/MM/YYYY hh/mm/ss");
       //display("By David & Magnus"); // 30 Character wide screen
       o=59;
     }
     p = returnKeypadValue(&trigger); // p = keypad value 
     if(trigger == 1)
     {
       DisplayKeyboard(p);   // Displays the keypad value on the screen 
       Delay(10000000);
       trigger = 0;
     } else { continue; }
   }
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

void Delay(int Value)  { 
  int i;  
  for(i=0;i<Value;i++)  
    asm("nop");  
} 


void Init_Display(void) { 
  time.decimalPointer = 0;
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

void DisplayKeyboard(int inp)
{
  if(o==0)
    clearDisplay(0x00,0x00);
  //int lg = (int)log10(inp);
//    for(int i = lg; 0 <= i; i--)
//    {
//      digit = getDigit(i, inp);
//      Write_Data_2_Display(digits[inp - 1]);
//      Write_Command_2_Display(0xc0);
//    }
//  if(o!=59)
//    clearDisplay(0x00,0x00);
//  else
//  {
//    int lg = (int)log10(inp);
//    for(int i = lg; 0 <= i; i--)
//    {
//      int digit = getDigit(i, inp);
//      Write_Data_2_Display(digits[digit]);
//      Write_Command_2_Display(0xc0);
//    }
//  }
  if(time.decimalPointer < 14) {
    if(9 < inp){ 
      switch(inp)
      { 
  
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
    }
    else {
      Write_Data_2_Display(digits[inp]);
      //Delay(2000000);
      Write_Command_2_Display(0xc0);
    }
    assignToSeq((inp == 11 ? 0 : inp));
  } else { addSecondToConfiguredCalender(); }
  //time.date[0]++;
}

/*void displayInt(int number)
{
  //number = number*10;
  Write_Data_2_Display(16 + (number/10)%10);
  Write_Command_2_Display(0xC0);
  Write_Data_2_Display(16 + (number%10));
  Write_Command_2_Display(0xC0);
}
*/

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

  for(m = 0; '\0' !=ch[m];m++) {
    temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void assignToSeq(int inp) {
 //clearDisplay(0x00,0x00);
 //if(time.decimalPointer == 14) {}
 if(time.decimalPointer < 14) {
   time.sequence[time.decimalPointer] = inp;
   if(time.decimalPointer == 13) {assignDigitToCalender(time.sequence);}
 }
 time.decimalPointer++;
}

void assignDigitToCalender(int seq[]){
 int yearPow = 3;
 int otherPow = 1;
 int offset = 0;
 for(int i = 0; i < 14; i++) {
  int digit = seq[i];

  if(4 <= i && i <= 7) {
    time.date[2] += (digit * (int)(pow(10, yearPow--)));
    offset = 2;
  } else {
    time.date[(i - offset) / 2] += (digit * (int)(pow(10, otherPow)));
    otherPow = (otherPow + 1) % 2; // Flip between 1 and 0
  }
 }
 displayTime();
}

void displayTime() {
 clearDisplay(0x00,0x00);

 for(int l = 0; l < 6; l++) {
  int num = time.date[l];
  int lg  = (num == 0 ? 0 : (int)log10(num));
  if(lg == 0) {
    Write_Data_2_Display(0x10);
    Write_Command_2_Display(0xc0);
  }
  for(int h = lg; 0 <= h; h--) {
    int digit = getDigit(h, num);
    char ch = digits[digit];
    Write_Data_2_Display(ch);
    Write_Command_2_Display(0xc0);
  }
  if(l != 5) {
    Write_Data_2_Display((l == 2 ? 0x00 : 0x0F));
    Write_Command_2_Display(0xc0);
  }
 }

 o=1;
 if(time.decimalPointer == 13) {
  int boolean = 0;    // boolean = 0, means input values makes up valid time
  if((12 < time.date[1]) || (time.date[2] < 2021) || (23 < time.date[3]) || (59 < time.date[4]) || (59 < time.date[5])) { boolean = 1; }
  if(boolean == 0 && (months[time.date[1] - 1] < time.date[0])) { boolean = 1;}
  if(boolean == 1) { 
    display("INVALID INPUT!-RESTART PROGRAM");
    time.decimalPointer = 15; // To inactivate the if-statement in the sysTick-handler
  }
 }
}

void addSecondToConfiguredCalender() {
  int carry = 0;
  if(time.date[5] == 59) { // Seconds
    time.date[5] = 0;
    carry = 1;
  } else { time.date[5]++; }
  if(carry + time.date[4] == 60) { // Minutes
    time.date[4] = 0;
    carry = 1;
  } else { 
    time.date[4] = time.date[4] + carry;
    carry = 0;
  }
  if(carry + time.date[3] == 24) { // Hours
    time.date[3] = 0;
    carry = 1;
  } else { 
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) { // Days
    time.date[0] = 1;
    carry = 1;
  } else { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) { // Months
    time.date[1] = 1;
    carry = 1;
  } else { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  displayTime();
}

void SysTick_Handler(void){
  //clockCounter++;
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14)) { addSecondToConfiguredCalender(); }
}

void tempInit() {   
  *AT91C_PMC_PCER = (1 << 12) | (1 << 27); // Enable peripheral PIOB and TC0  

  // TIMER_CLOCK1 = MCK / 2; 
  //*AT91C_TC0_CMR  = ((*AT91C_TC0_CMR & 0xFFFFFFF8) | 0x1); // bit nr. 0, TIMER_CLOCK1   
  //*AT91C_TC0_CCR  = ((*AT91C_TC0_CCR & 0xFFFFFFFA) | 0x5); // Enable clk & SW reset, 0x5 = 0b101   

  //*AT91C_TC0_CMR  = ((*AT91C_TC0_CMR & 0xFFFCFFFF) | (2<<16));// LDRA, 2 = Falling edge of TIOA   
  //*AT91C_TC0_CMR  = ((*AT91C_TC0_CMR & 0xFFF3FFFF) | (1<<18));// LDRB, 1 = Rising edge of TIOB 

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

void StartMeasureTemp(void) {  
  CreatePulseInPin2(672000); // Create a reset pulse, 8ms long 
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
  //*AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IDR = (1 << 6); // Disable interrupt for LDRBS   
  INTERRUPT_FLAG = 1;     // Setting global flag   
}  
