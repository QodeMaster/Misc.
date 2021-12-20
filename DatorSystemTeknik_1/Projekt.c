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

---------------------------------------------------------------------------------------------------------------------------
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
  int timeStamp[6]; 
  int temprature; 
  struct LinkedList *next; 
};

unsigned int i = 0;
short trigger  = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;  

struct calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;
int secSinceLastTempMeasurement = 0;
int isFirstTimeMeasured = 1;

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
void tempCalculator();

void SysTick_Handler(void);

void insertFirst(struct LinkedList **first, int dateAndTime[6], int tempratureMeasurement);
void initLL(struct LinkedList **first, int dateAndTime[6], int tempratureMeasurement);
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
   StartMeasureTemp();
   
   struct LinkedList *list;
   
   while(1) {
     if(TEMPRATURE_INTERRUPT_FLAG == 1) {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
     }
     
     if(o == 79) {
       locationOnDisplay(0x28,0x00);
       display("Configure: DD/MM/YYYY hh/mm/ss");
       //display("By David & Magnus"); // 30 Character wide screen
       o=59;
     }
     
     p = returnKeypadValue(&trigger); // p = keypad value 
     if(trigger == 1) {
       DisplayKeyboard(p);   // Displays the keypad value on the screen 
       Delay(10000000);
       trigger = 0;
     } else { continue; }
     
     if(secSinceLastTempMeasurement == 60) { // Req 2 in project
       tempCalculator();
       if(isFirstTimeMeasured == 1) {
         initLL(&list, time.date, temprature);
         isFirstTimeMeasured = 0;
       } else { 
         insertFirst(&list, time.date, temprature);
       }
       secSinceLastTempMeasurement = 0;
     }
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

    Delay(200); // Originally Delay(20000);

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

void DisplayKeyboard(int inp){
  if(o==0) { clearDisplay(0x00,0x00); }
  if(time.decimalPointer < 14) {
    if(9 < inp){ 
      switch(inp){ 
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
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14)) {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
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
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator() {
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273) {
    // To prevent the screen from changing if the temprature hasn't changed
    temprature = (deltaT / 210) - 273;
    //display("Temprature: ");
    //displayInt(temprature);
  }
}

void insertFirst(struct LinkedList **first, int dateAndTime[6], int tempratureMeasurement) {
  struct LinkedList *temp = *first;
  struct LinkedList *el;
  
  el->timeStamp = dateAndTime;
  el->temprature = tempratureMeasurement;
  el->next = NULL;
  
  *first = el;
  (*first)->next = temp;
}

void initLL(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement) {
  struct LinkedList initial;
  
  initial.timeStamp  = dateAndTime[6];
  initial.temprature = tempratureMeasurement;
  initial.next       = NULL;
  
  first->timeStamp  = initial.timeStamp;
  first->temprature = initial.temprature;
  first->next       = initial.next;
}
-------------------------------------------------------------------------------------------------------------------------------------------------
 #include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

struct calender {
 int date[6];
 int decimalPointer;
 int sequence[14];
};

struct LinkedList {
  int timeStamp[6]; 
  int temprature; 
  struct LinkedList *next; 
};

unsigned int i = 0;
short trigger  = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;  

struct calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;
int secSinceLastTempMeasurement = 0;
int isFirstTimeMeasured = 1;


int day0 = 0;
int day1 = 0;
int day2 = 0;
int day3 = 0;
int day4 = 0;
int day5 = 0;
int temp101 = 0;


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
void tempCalculator();

void SysTick_Handler(void);

void insertFirst(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement);
void initLL(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement);
void printList(struct LinkedList *first);

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
   StartMeasureTemp();
   
   struct LinkedList list;
   (list.timeStamp)[0] = 14;
   
   // 20/10/2021 - 20/55/55
   time.date[0] = 20;
   time.date[1] = 10;
   time.date[2] = 2021;
   time.date[3] = 20;
   time.date[4] = 55;
   time.date[5] = 55;
   time.decimalPointer = 14;
   
   while(1) {
     if(TEMPRATURE_INTERRUPT_FLAG == 1)
     {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
     }
     
     /*if(o == 79)
     {
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
     } 
     else
     {
       continue;
     }*/
     
     if(secSinceLastTempMeasurement == 2) 
     { // Req 2 in project
       
       if(isFirstTimeMeasured == 1) {
         initLL(&list, time.date, temprature);
         isFirstTimeMeasured = 0;
       }
       else 
       { 
         insertFirst(&list, time.date, temprature);
       }
       //printList(list);
       secSinceLastTempMeasurement = 0;
     }
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

    Delay(200); // Originally Delay(20000);

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

void DisplayKeyboard(int inp){
  if(o==0) { clearDisplay(0x00,0x00); }
  if(time.decimalPointer < 14) {
    if(9 < inp){ 
      switch(inp){ 
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
}

void clearDisplay(int x, int y)
{
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
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14)) {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
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
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator() {
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273)
  {
    // To prevent the screen from changing if the temprature hasn't changed
    temprature = (deltaT / 210) - 273;
    //display("Temprature: ");
    //displayInt(temprature);
  }
}

void insertFirst(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement)
{
  struct LinkedList *temp = first;
  struct LinkedList *el;
  
  for(int i = 0; i < 6; i++) { (el->timeStamp)[i] = dateAndTime[i]; }
  el->temprature = tempratureMeasurement;
  el->next = NULL;
  
  first = el;
  (first)->next = temp;
}

void initLL(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement)
{
  struct LinkedList *pot = first;
  struct LinkedList initial;
  
  for(int i = 0; i < 6; i++) { initial.timeStamp[i] = dateAndTime[i]; }
  initial.temprature = tempratureMeasurement;
  initial.next       = NULL;
  pot->timeStamp[0] = initial.timeStamp[0];
  for(int i = 0; i < 6; i++) { (first)->timeStamp[i] = initial.timeStamp[i]; }
  (first)->temprature = initial.temprature;
  (first)->next       = initial.next;
}

void printList(struct LinkedList *first)
{
  struct LinkedList *holder = first;

    day0 =  holder->timeStamp[0];
    day1 =  holder->timeStamp[1];
    day2 =  holder->timeStamp[2];
    day3 =  holder->timeStamp[3];
    day4 =  holder->timeStamp[4];
    day5 =  holder->timeStamp[5];
    //temp101 = holder->temprature;
}
---------------------------------------------------------------------------------------------------------------------------------------
  #include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

struct calender
{
 int date[6];
 int decimalPointer;
 int sequence[14];
};

struct LinkedList
{
  int timeStamp[6]; 
  int temprature; 
  struct LinkedList *next; 
};

int try = 0;


unsigned int i = 0;
short trigger  = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;  

struct calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;
int secSinceLastTempMeasurement = 0;
int isFirstTimeMeasured = 1;

int day0 = 0;
int day1 = 0;
int day2 = 0;
int day3 = 0;
int day4 = 0;
int day5 = 0;
int temp101 = 0;

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
void tempCalculator();

void SysTick_Handler(void);

void insertFirst(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement);
void initLL(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement);
void printList(struct LinkedList *first);

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
struct LinkedList *head;

void main(void)
{
  head = (struct LinkedList*)malloc(sizeof(struct LinkedList));
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
   StartMeasureTemp();
   
   //struct LinkedList list;
   (head->timeStamp)[0] = 14;
   
   // 20/10/2021 - 20/55/55
   time.date[0] = 20;
   time.date[1] = 10;
   time.date[2] = 2021;
   time.date[3] = 20;
   time.date[4] = 55;
   time.date[5] = 55;
   time.decimalPointer = 14;
   
   while(1) {
     if(TEMPRATURE_INTERRUPT_FLAG == 1)
     {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
     }
     
     /*if(o == 79)
     {
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
     } 
     else
     {
       continue;
     }*/
     
     if(secSinceLastTempMeasurement == 2) 
     { // Req 2 in project
       
       if(isFirstTimeMeasured == 1)
       {
         initLL(head, time.date, temprature);
         isFirstTimeMeasured = 0;
       }
       else
       {
         insertFirst(head, time.date, temprature);
       }
       //printList(list);
       secSinceLastTempMeasurement = 0;
     }
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

void Write_Command_2_Display(unsigned char Command)
{ 

  while((Read_Status_Display()) & (0xC) != 0xC){} 

    *AT91C_PIOC_OER  = (255<<2); // output enable pin 34 - 41 
    *AT91C_PIOC_CODR = (255<<2); // clear databus 

    *AT91C_PIOC_SODR = ((unsigned int)Command)<<2; 
    *AT91C_PIOC_CODR = (3<<12); // enabel output (clear OE) 
    *AT91C_PIOC_OER  = (255<<2);// output enable pin 34 - 41 
    *AT91C_PIOC_SODR = (1<<14); // set C/D 
    *AT91C_PIOC_CODR = (5<<15); // clear CE chip select display & clr wr disp

    Delay(200); // Originally Delay(20000);

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

void DisplayKeyboard(int inp){
  if(o==0) { clearDisplay(0x00,0x00); }
  if(time.decimalPointer < 14) {
    if(9 < inp){ 
      switch(inp){ 
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
}

void clearDisplay(int x, int y)
{
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
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14)) {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
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
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator() {
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273)
  {
    // To prevent the screen from changing if the temprature hasn't changed
    temprature = (deltaT / 210) - 273;
    //display("Temprature: ");
    //displayInt(temprature);
  }
}

void insertFirst(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement)
{
  struct LinkedList *el = (struct LinkedList*)malloc(sizeof(struct LinkedList));
  for(int i = 0; i < 6; i++) { (el->timeStamp)[i] = dateAndTime[i]; }
  el->temprature = tempratureMeasurement;
  el->next = head;
  head = el;
}

void initLL(struct LinkedList *first, int dateAndTime[6], int tempratureMeasurement){
  struct LinkedList initial;
  
  for(int i = 0; i < 6; i++) { initial.timeStamp[i] = dateAndTime[i]; }
  initial.temprature = tempratureMeasurement;
  initial.next       = NULL;
  
  for(int i = 0; i < 6; i++) { (first)->timeStamp[i] = initial.timeStamp[i]; }
  (first)->temprature = initial.temprature;
  first->next       = initial.next;
}

void printList(struct LinkedList *first)
{
  struct LinkedList *holder = first;

    day0 =  holder->timeStamp[0];
    day1 =  holder->timeStamp[1];
    day2 =  holder->timeStamp[2];
    day3 =  holder->timeStamp[3];
    day4 =  holder->timeStamp[4];
    day5 =  holder->timeStamp[5];
    //temp101 = holder->temprature;
}

______________________________________________________________________________________________________________
 
 #include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct calender
{
 short date[6];
 uint8_t decimalPointer;
 uint8_t sequence[14];
} calender;

typedef struct LinkedList
{
  short timeStamp[6]; 
  short temprature; 
  struct LinkedList *next; 
} LinkedList;

int try = 0;

unsigned int i = 0;
short trigger  = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;  

calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;
int secSinceLastTempMeasurement = 0;
int isFirstTimeMeasured = 1;

int day0 = 0;
int day1 = 0;
int day2 = 0;
int day3 = 0;
int day4 = 0;
int day5 = 0;
int temp101 = 0;

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

void assignDigitToCalender(uint8_t seq[]);
void assignToSeq(short inp);
void displayTime();
void addSecondToConfiguredCalender();

void CreatePulseInPin2(int pulseWidth);
void tempInit(void);
void StartMeasureTemp(void);
void TC0_Handler(void);
void tempCalculator();

void SysTick_Handler(void);

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void printList(LinkedList *first);

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
LinkedList *head;

void main(void)
{
  head = (LinkedList*)malloc(sizeof(LinkedList));
   SystemInit();
   SysTick_Config(SystemCoreClock * .01); // 84MHz * 1, 840 000 ticks/sec <=> 100 interrupt/sec
   //char ch=0x0;
   *AT91C_PIOC_OER   = (1<<6) | (255 << 12);
   
   Init_Display(); 
   Write_Data_2_Display(0); 
   Write_Data_2_Display(0); 
   Write_Command_2_Display(0x24); 
   Delay(300000); 
   clearDisplay(0x00,0x00);
   
   tempInit();  
   StartMeasureTemp();
   
   //struct LinkedList list;
   (head->timeStamp)[0] = 14;
   
   // 20/10/2021 - 20/55/55
   time.date[0] = 20;
   time.date[1] = 10;
   time.date[2] = 2021;
   time.date[3] = 20;
   time.date[4] = 55;
   time.date[5] = 55;
   time.decimalPointer = 14;
   
   while(1) {
     if(TEMPRATURE_INTERRUPT_FLAG == 1)
     {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
     }
     
     /*if(o == 79)
     {
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
     } 
     else
     {
       continue;
     }*/
     
     if(secSinceLastTempMeasurement == 2) 
     { // Req 2 in project
       
       if(isFirstTimeMeasured == 1)
       {
         initLL(head, time.date, temprature);
         isFirstTimeMeasured = 0;
       }
       else 
       {
         insertFirst(head, time.date, temprature);
       }
       secSinceLastTempMeasurement = 0;
     }
   }
}

int returnKeypadValue(short* trigger)
{
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
        if((*AT91C_PIOC_PDSR & (1<<rowOrder[l])) == 0)
        {
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

void Delay(int Value)
{
  int i;  
  for(i=0;i<Value;i++)  
    asm("nop");  
} 

void Init_Display(void)
{
  *AT91C_PMC_PCER=(3<<13);      // Port C, D

  *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
  *AT91C_PIOC_PPUDR = 0xffffffff; 
  *AT91C_PIOD_PER   = 1; 
  *AT91C_PIOD_PPUDR = 1; 
  
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

void Write_Data_2_Display(unsigned char Data)
{
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

void Write_Command_2_Display(unsigned char Command)
{ 

  while((Read_Status_Display()) & (0xC) != 0xC){} 

    *AT91C_PIOC_OER  = (255<<2); // output enable pin 34 - 41 
    *AT91C_PIOC_CODR = (255<<2); // clear databus 

    *AT91C_PIOC_SODR = ((unsigned int)Command)<<2; 
    *AT91C_PIOC_CODR = (3<<12); // enabel output (clear OE) 
    *AT91C_PIOC_OER  = (255<<2);// output enable pin 34 - 41 
    *AT91C_PIOC_SODR = (1<<14); // set C/D 
    *AT91C_PIOC_CODR = (5<<15); // clear CE chip select display & clr wr disp

    Delay(200); // Originally Delay(20000);

    *AT91C_PIOC_SODR = (1<<17); // set write display 
    *AT91C_PIOC_SODR = (9<<12); // disable output (set OE) 
    *AT91C_PIOC_ODR = (255<<2); // output disable pin 34 - 41 
} 

unsigned char Read_Status_Display()
{
    unsigned char temp; 

    /*
    *AT91C_PMC_PCER=(3<<13);      // Port C, D

    *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
    *AT91C_PIOC_PPUDR = 0xffffffff; 
    *AT91C_PIOD_PER   = 1; 
    *AT91C_PIOD_PPUDR = 1; */
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
  {
    clearDisplay(0x00,0x00);
  }
  if(time.decimalPointer < 14)
  {
    if(9 < inp)
    {
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
    else
    {
      Write_Data_2_Display(digits[inp]);
      //Delay(2000000);
      Write_Command_2_Display(0xc0);
    }
    assignToSeq((inp == 11 ? 0 : inp));
  } 
  else 
    addSecondToConfiguredCalender();
}

void clearDisplay(int x, int y)
{
  locationOnDisplay(x,y);

  for(int i=0;i<(30*16);i++) // clear display
  {
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xC0);
  }
  locationOnDisplay(0x00,0x00);
}

void locationOnDisplay(int x,int y)
{
  Write_Data_2_Display(x);
  Write_Data_2_Display(y); 
  Write_Command_2_Display(0x24);
}

void display(char ch[])
{
  clearDisplay(0x00,0x00);
  //char temp;

  for(m = 0; '\0' !=ch[m];m++)
  {
    temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void assignToSeq(short inp)
{
 //clearDisplay(0x00,0x00);
 //if(time.decimalPointer == 14) {}
 if(time.decimalPointer < 14)
 {
   time.sequence[time.decimalPointer] = inp;
   if(time.decimalPointer == 13)
     assignDigitToCalender(time.sequence);
 }
 time.decimalPointer++;
}

void assignDigitToCalender(uint8_t seq[])
{
 int yearPow = 3;
 int otherPow = 1;
 int offset = 0;
 for(int i = 0; i < 14; i++)
 {
  int digit = seq[i];

  if(4 <= i && i <= 7)
  {
    time.date[2] += (digit * (int)(pow(10, yearPow--)));
    offset = 2;
  }
  else
  {
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

void addSecondToConfiguredCalender() 
{
  int carry = 0;
  if(time.date[5] == 59) 
  { // Seconds
    time.date[5] = 0;
    carry = 1;
  }
  else 
   time.date[5]++; 
  if(carry + time.date[4] == 60) 
  { // Minutes
    time.date[4] = 0;
    carry = 1;
  } 
  else 
  { 
    time.date[4] = time.date[4] + carry;
    carry = 0;
  }
  if(carry + time.date[3] == 24) 
  { // Hours
    time.date[3] = 0;
    carry = 1;
  }
  else 
  { 
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) 
  { // Days
    time.date[0] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) 
  { // Months
    time.date[1] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  //displayTime();
}

void SysTick_Handler(void)
{
  //clockCounter++;
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14) && (speed != 1))
  {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
  /*
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14) && (speed == 1))
  {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
  */
}

void tempInit()
{   
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
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator() {
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273)
  {
    // To prevent the screen from changing if the temprature hasn't changed
    temprature = (deltaT / 210) - 273;
    //display("Temprature: ");
    //displayInt(temprature);
  }
}

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement)
{
  LinkedList *el = (LinkedList*)malloc(sizeof(LinkedList));
  for(int i = 0; i < 6; i++) { (el->timeStamp)[i] = dateAndTime[i]; }
  el->temprature = tempratureMeasurement;
  el->next       = head;
  head = el;
}

void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement){
  LinkedList initial;
  
  for(int i = 0; i < 6; i++) { initial.timeStamp[i] = dateAndTime[i]; }
  initial.temprature = tempratureMeasurement;
  initial.next       = NULL;
  
  for(int i = 0; i < 6; i++) { (first)->timeStamp[i] = initial.timeStamp[i]; }
  (first)->temprature = initial.temprature;
  first->next         = initial.next;
}

_________________________________________________________________________________________________________________
 
 #include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct calender {
 short date[6];
 uint8_t decimalPointer;
 uint8_t sequence[14];
} calender;

typedef struct LinkedList
{
  short timeStamp[6]; 
  short temprature; 
  struct LinkedList *next; 
} LinkedList;

typedef struct StatsForADay
{
  short timeStampMax[6]; 
  short max; 
  short avg;
  short timeStampMin[6]; 
  short min;
} StatsForADay;

int linkedListLength = 0; // a length of 1440 means 1 day of measuring has been achieved

int try = 0;

unsigned int i = 0;
short trigger  = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;

calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int tempMessuredDay = 0;
int alTemp = 0;
char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;
int secSinceLastTempMeasurement = 0;
int isFirstTimeMeasured = 1;
int fastMode = 0; // fastMode = false,
char memoryIsFull_FLAG = 0; // will be set if no more memory can get allocated

int day0 = 0;
int day1 = 0;
int day2 = 0;
int day3 = 0;
int day4 = 0;
int day5 = 0;
int temp101 = 0;

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
void rawDisplay(char ch[]);
void displayOptions();
void displayDay();
int getDigit(int spot, int n){return (int)((n % (int)(pow(10,spot+1))) / pow(10,spot));}

void assignDigitToCalender(uint8_t seq[]);
void assignToSeq(short inp);
void displayTime();
void addSecondToConfiguredCalender();
void addTwentyMinutesToConfiguredCalender();

void CreatePulseInPin2(int pulseWidth);
void tempInit(void);
void StartMeasureTemp(void);
void TC0_Handler(void);
void tempCalculator();

void SysTick_Handler(void);

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void DeleteList(LinkedList **first, StatsForADay stats[7]);

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
LinkedList *head;
StatsForADay stats[7];

void main(void)
{
   head = (LinkedList*)malloc(sizeof(LinkedList));
   //StatsForADay stats[7];
   SystemInit();
   SysTick_Config(SystemCoreClock * .01); // 84MHz * 1, 840 000 ticks/sec <=> 100 interrupt/sec
   //char ch=0x0;
   *AT91C_PIOC_OER   = (1<<6) | (255 << 12);
   
   Init_Display(); 
   Write_Data_2_Display(0); 
   Write_Data_2_Display(0); 
   Write_Command_2_Display(0x24); 
   Delay(300000); 
   clearDisplay(0x00,0x00);
   
   tempInit();  
   StartMeasureTemp();
   
   //struct LinkedList list;
   (head->timeStamp)[0] = 14;
   
   // 20/10/2021 - 20/55/55
   time.date[0] = 20;
   time.date[1] = 10;
   time.date[2] = 2021;
   time.date[3] = 00;
   time.date[4] = 00;
   time.date[5] = 00;
   time.decimalPointer = 14;
   
   stats[0].min = 1000;
   stats[0].max = 0;
   
   while(1)
   {
     if(TEMPRATURE_INTERRUPT_FLAG == 1)
     {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
     }
     
     /*if(o == 79)
     {
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
     } 
     else
     {
       continue;
     }*/
     
     if(secSinceLastTempMeasurement == 2) 
     { // Req 2 in project
       
       if(isFirstTimeMeasured == 1)
       {
         initLL(head, time.date, temprature);
         isFirstTimeMeasured = 0;
       }
       else 
       {
         insertFirst(head, time.date, temprature);
       }
       secSinceLastTempMeasurement = 0;
       displayOptions();
       //displayDay();
     }
   }
}

int returnKeypadValue(short* trigger) {
   *AT91C_PMC_PCER   = (3<<13);                        // Enable control over PIOC and PIOD, also defined in InitDisplay()
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
        if((*AT91C_PIOC_PDSR & (1<<rowOrder[l])) == 0)
        {
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

void Delay(int Value) {
  int i;  
  for(i=0;i<Value;i++)  
    asm("nop");  
} 

void Init_Display(void)
{
  *AT91C_PMC_PCER=(3<<13);      // Port C, D

  *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
  *AT91C_PIOC_PPUDR = 0xffffffff; 
  *AT91C_PIOD_PER   = 1; 
  *AT91C_PIOD_PPUDR = 1; 
  
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

void Write_Data_2_Display(unsigned char Data)
{
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

void Write_Command_2_Display(unsigned char Command) { 
  while((Read_Status_Display()) & (0xC) != 0xC){} 

    *AT91C_PIOC_OER  = (255<<2); // output enable pin 34 - 41 
    *AT91C_PIOC_CODR = (255<<2); // clear databus 

    *AT91C_PIOC_SODR = ((unsigned int)Command)<<2; 
    *AT91C_PIOC_CODR = (3<<12); // enabel output (clear OE) 
    *AT91C_PIOC_OER  = (255<<2);// output enable pin 34 - 41 
    *AT91C_PIOC_SODR = (1<<14); // set C/D 
    *AT91C_PIOC_CODR = (5<<15); // clear CE chip select display & clr wr disp

    Delay(200); // Originally Delay(20000);

    *AT91C_PIOC_SODR = (1<<17); // set write display 
    *AT91C_PIOC_SODR = (9<<12); // disable output (set OE) 
    *AT91C_PIOC_ODR = (255<<2); // output disable pin 34 - 41 
} 

unsigned char Read_Status_Display()
{
    unsigned char temp; 

    /*
    *AT91C_PMC_PCER=(3<<13);      // Port C, D
    *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
    *AT91C_PIOC_PPUDR = 0xffffffff; 
    *AT91C_PIOD_PER   = 1; 
    *AT91C_PIOD_PPUDR = 1; */
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
  {
    clearDisplay(0x00,0x00);
  }
  if(time.decimalPointer < 14)
  {
    if(9 < inp)
    {
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
    else
    {
      Write_Data_2_Display(digits[inp]);
      //Delay(2000000);
      Write_Command_2_Display(0xc0);
    }
    assignToSeq((inp == 11 ? 0 : inp));
  } 
  else 
    addSecondToConfiguredCalender();
}

void clearDisplay(int x, int y)
{
  locationOnDisplay(x,y);

  for(int i=0;i<(30*16);i++) // clear display
  {
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xC0);
  }
  locationOnDisplay(0x00,0x00);
}

void locationOnDisplay(int x,int y)
{
  Write_Data_2_Display(x);
  Write_Data_2_Display(y); 
  Write_Command_2_Display(0x24);
}

void display(char ch[]) {
  clearDisplay(0x00,0x00);
  //char temp;

  for(m = 0; '\0' !=ch[m];m++)
  {
    temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void rawDisplay(char ch[]) {

  for(char m = 0; '\0' !=ch[m];m++) {

    char temp = ch[m] - 0x20;

    Write_Data_2_Display(temp);

    Write_Command_2_Display(0xC0);

  }

}

void displayInt(int inp) {

  int lg = (inp == 0 ? 0 : (int)log10(inp)
);

  for(int i = lg; 0 <= i; i--)
{

    int digit = getDigit(i, inp);

    Write_Data_2_Display(digits[digit]
);

    Write_Command_2_Display(0xc0);

  }

}

void assignToSeq(short inp)
{
 //clearDisplay(0x00,0x00);
 //if(time.decimalPointer == 14) {}
 if(time.decimalPointer < 14)
 {
   time.sequence[time.decimalPointer] = inp;
   if(time.decimalPointer == 13)
     assignDigitToCalender(time.sequence);
 }
 time.decimalPointer++;
}

void assignDigitToCalender(uint8_t seq[])
{
 int yearPow = 3;
 int otherPow = 1;
 int offset = 0;
 for(int i = 0; i < 14; i++)
 {
  int digit = seq[i];

  if(4 <= i && i <= 7)
  {
    time.date[2] += (digit * (int)(pow(10, yearPow--)));
    offset = 2;
  }
  else
  {
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

void moveToNextRow(int spaces) {

  for(int i = 0; i < spaces; i++) {

    Write_Data_2_Display(0x00);

    Write_Command_2_Display(0xc0);

  }

}

void displayOptions() {
  clearDisplay(0x00,0x00);
  
  rawDisplay("Press 2 for Day 1");
  moveToNextRow(30 - 17);
  rawDisplay("Press 3 for Day 2");
  moveToNextRow(30 - 17);
  rawDisplay("Press 4 for Day 3");
  moveToNextRow(30 - 17);
  rawDisplay("Press 5 for Day 4");
  moveToNextRow(30 - 17);
  rawDisplay("Press 6 for Day 5");
  moveToNextRow(30 - 17);
  rawDisplay("Press 7 for Day 6");
  moveToNextRow(30 - 17);
  rawDisplay("Press 8 for Day 7");
  moveToNextRow(30 - 17);
  moveToNextRow(30);
  rawDisplay("Press * for fastMode");
}

void displayDay() {
  clearDisplay(0x00,0x00);
  
  rawDisplay("DD/MM/YYYY");
  moveToNextRow(30 - 10);
  moveToNextRow(30);
  rawDisplay("Max: 29  hh/mm/ss");
  moveToNextRow(30 - 17);
  moveToNextRow(30);
  rawDisplay("Avg: 26  ");
  moveToNextRow(30 - 17 + 8);
  moveToNextRow(30);
  rawDisplay("Min: 23  hh/mm/ss");
}

void addSecondToConfiguredCalender() 
{
  int carry = 0;
  int max = 0;
  int min = 1000;
  if(time.date[5] == 59) 
  { // Seconds
    time.date[5] = 0;
    carry = 1;
  }
  else 
   time.date[5]++; 
  if(carry + time.date[4] == 60) 
  { // Minutes
    time.date[4] = 0;
    carry = 1;
  } 
  else 
  { 
    time.date[4] = time.date[4] + carry;
    carry = 0;
  }
  if(carry + time.date[3] == 24) 
  { // Hours
    time.date[3] = 0;
    carry = 1;
    DeleteList(&head, stats);
    while(stats != NULL)
    {
      if(max < stats->temprature)
      {
        max = head->temprature;
        for(int i = 0; i < 6; i++) 
        {
          timeStampMax[i] = head->timeStamp[i];
        }
      }
    }
    tempMessuredDay = 0;
  }
  else 
  { 
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) 
  { // Days
    time.date[0] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) 
  { // Months
    time.date[1] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  //displayTime();
}

void addTwentyMinutesToConfiguredCalender()
{
  int carry = 0;
  // Seconds won't change because only minutes and upwards are incremented
  if(20 + time.date[4] >= 60) { // Minutes, 20 minute increment
    time.date[4] = time.date[4] % 60;
    carry = 1;
  } 
  else 
  { 
    time.date[4] = time.date[4] + 20;
  }
  if(carry + time.date[3] == 24) 
  { // Hours
    time.date[3] = 0;
    carry = 1;
  }
  else 
  { 
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) 
  { // Days
    time.date[0] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) 
  { // Months
    time.date[1] = 1;
    carry = 1;
  } 
  else 
  {
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  //displayTime();
}

void SysTick_Handler(void)
{
  //clockCounter++;
  // 10x speed clock
  if((clockCounter++ % 10 == 0) && (time.decimalPointer == 14) && (fastMode != 1))
  {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
  /*
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14) && (speed == 1))
  {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
  */
}

void tempInit()
{   
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

void StartMeasureTemp(void)
{  
  CreatePulseInPin2(672000); // Create a reset pulse, 8ms long 
  //Create a startpuls with a Delay(25)   
  CreatePulseInPin2(25);  

  //*AT91C_TC0_CCR  = (*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4;// SW reset, TC0_CCR0 
  *AT91C_TC0_CCR  = ((*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4);// SW reset, TC0_CCR0   
  *AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IER = (1 << 6); // Enable interrupt for LDRBS   
} 

void CreatePulseInPin2(int pulseWidth)
{   
  *AT91C_PIOB_OER  = (1 << 25); // Make pin 2 an output   
  *AT91C_PIOB_CODR = (1 << 25); // Put a low signal on the I/O line   
  Delay(pulseWidth); 
  *AT91C_PIOB_SODR = (1 << 25); // Put a high signal on the I/O line   
  *AT91C_PIOB_ODR  = (1 << 25); // Make pin 2 an input   
} 

void TC0_Handler()
{   
  //*AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IDR = (1 << 6); // Disable interrupt for LDRBS   
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator()
{
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273)
  {
    // To prevent the screen from changing if the temprature hasn't changed
    temprature = (deltaT / 210) - 273;
    //display("Temprature: ");
    //displayInt(temprature);
  }
}

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement)
{
  LinkedList *el = (LinkedList*)malloc(sizeof(LinkedList)); // Still needs to freed after use
  if(el != NULL)
  {
    for(int i = 0; i < 6; i++) { (el->timeStamp)[i] = dateAndTime[i]; }
    el->temprature = tempratureMeasurement;
    el->next       = head;
    head = el;
    linkedListLength++;
    tempMessuredDay++;
  }
  if(linkedListLength == 250)
  {
    memoryIsFull_FLAG = 1;
    DeleteList(&head, stats);
    // Program will arrive here if memory is full.
  }
}

void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement)
{
  LinkedList initial;
  
  for(int i = 0; i < 6; i++)
  {
    initial.timeStamp[i] = dateAndTime[i]; 
  }
  tempMessuredDay++;
  initial.temprature = tempratureMeasurement;
  initial.next       = NULL;
  
  for(int i = 0; i < 6; i++) 
  {
    (first)->timeStamp[i] = initial.timeStamp[i]; 
  }
  (first)->temprature = initial.temprature;
  first->next         = initial.next;
  
  linkedListLength = 1;
}

void DeleteList(LinkedList **first, StatsForADay stats[7])
{
  LinkedList *holderHolder;
  int max = stats[0].max;
  int min = stats[0].min;
  short timeStampMax[6];
  short timeStampMin[6];
  short timeStampTemp[6];
  for(int i = 0; i < 6; i++)
  {
    timeStampTemp[i] = head->timeStamp[i];
  }
  while(head != NULL)
  {
    if(max < head->temprature)
    {
      max = head->temprature;
      for(int i = 0; i < 6; i++) 
      {
        timeStampMax[i] = head->timeStamp[i];
      }
    }
    if(head->temprature < min)
    {
      min = head->temprature;
      for(int i = 0; i < 6; i++)
      {
        timeStampMin[i] = head->timeStamp[i];
      }
    }
    alTemp += (head->temprature);
    holderHolder = head->next;
    free(head);
    head = holderHolder;
    /*if(head->next == NULL)
    {
      for(int i = 0; i < 6; i++)
      {
        timeStampTemp[i] = head->timeStamp[i];
      }
    }*/
  }
  
  stats[0].max = max;
  for(int i = 0; i < 6; i++) 
  {
    stats[0].timeStampMax[i] = timeStampMax[i];
  }
  stats[0].min = min;
  for(int i = 0; i < 6; i++) 
  {
    stats[0].timeStampMin[i] = timeStampMin[i];
  }
  head = (LinkedList*)malloc(sizeof(LinkedList));
  for(int i = 0; i < 6; i++)
  {
    head->timeStamp[i] = timeStampTemp[i];
  }
  isFirstTimeMeasured = 1;
  linkedListLength = 0;
  memoryIsFull_FLAG = 0;
}

--------------------------------------------------------------------------------------------------------------------------------
 #include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct calender {
 short date[6];
 uint8_t decimalPointer;
 uint8_t sequence[14];
} calender;

typedef struct LinkedList
{
  short timeStamp[6]; 
  short temprature; 
  struct LinkedList *next; 
} LinkedList;

typedef struct StatsForADay
{
  short timeStampMax[6]; 
  short max; 
  short avg;
  short timeStampMin[6]; 
  short min;
} StatsForADay;

short linkedListLength = 0;      // A length of 1440 means 1 day of measuring has been achieved
short measurementCount = 0;      // Used as total sample count to calculate the average temprature for a day
short whatDayInStatsPointer = 0; // Used as total sample count to calculate the average temprature for a day

int try = 0;

unsigned int i = 0;
short trigger  = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;

calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int totalMeasuredTemp = 0;
char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;
int secSinceLastTempMeasurement = 0;
int isFirstTimeMeasured = 1;
int fastMode = 0; // fastMode = false,
char memoryIsFull_FLAG = 0; // will be set if no more memory can get allocated

int day0 = 0;
int day1 = 0;
int day2 = 0;
int day3 = 0;
int day4 = 0;
int day5 = 0;
int temp101 = 0;

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
void rawDisplay(char ch[]);
void displayOptions();
void displayDay();
int getDigit(int spot, int n){return (int)((n % (int)(pow(10,spot+1))) / pow(10,spot));}

void assignDigitToCalender(uint8_t seq[]);
void assignToSeq(short inp);
void displayTime();
void addSecondToConfiguredCalender();
void addTwentyMinutesToConfiguredCalender();

void CreatePulseInPin2(int pulseWidth);
void tempInit(void);
void StartMeasureTemp(void);
void TC0_Handler(void);
void tempCalculator();

void SysTick_Handler(void);

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void DeleteList(LinkedList **first, StatsForADay stats[7]);
void switchDay();

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
LinkedList *head;
StatsForADay stats[7];

void main(void)
{
   head = (LinkedList*)malloc(sizeof(LinkedList));
   //StatsForADay stats[7];
   SystemInit();
   SysTick_Config(SystemCoreClock * .01); // 84MHz * 1, 840 000 ticks/sec <=> 100 interrupt/sec
   //char ch=0x0;
   *AT91C_PIOC_OER   = (1<<6) | (255 << 12);
   
   Init_Display(); 
   Write_Data_2_Display(0); 
   Write_Data_2_Display(0); 
   Write_Command_2_Display(0x24); 
   Delay(300000); 
   clearDisplay(0x00,0x00);
   
   tempInit();  
   StartMeasureTemp();
   
   //struct LinkedList list;
   (head->timeStamp)[0] = 14;
   
   // 20/10/2021 - 20/55/55
   time.date[0] = 20;
   time.date[1] = 10;
   time.date[2] = 2021;
   time.date[3] = 23;
   time.date[4] = 48;
   time.date[5] = 3;
   time.decimalPointer = 14;
   
   stats[0].min = 1000;
   stats[0].max = 0;
   
   while(1) {
     if(TEMPRATURE_INTERRUPT_FLAG == 1)
     {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
     }
     
     /*if(o == 79)
     {
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
     } 
     else
     {
       continue;
     }*/
     
     if(secSinceLastTempMeasurement == 2) 
     { // Req 2 in project
       
       if(isFirstTimeMeasured == 1)
       {
         initLL(head, time.date, temprature);
         isFirstTimeMeasured = 0;
       }
       else 
       {
         insertFirst(head, time.date, temprature);
       }
       secSinceLastTempMeasurement = 0;
       displayOptions();
       //displayDay();
     }
   }
}

int returnKeypadValue(short* trigger) {
   *AT91C_PMC_PCER   = (3<<13);                        // Enable control over PIOC and PIOD, also defined in InitDisplay()
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
        if((*AT91C_PIOC_PDSR & (1<<rowOrder[l])) == 0)
        {
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

void Delay(int Value) {
  int i;  
  for(i=0;i<Value;i++)  
    asm("nop");  
} 

void Init_Display(void)
{
  *AT91C_PMC_PCER=(3<<13);      // Port C, D

  *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
  *AT91C_PIOC_PPUDR = 0xffffffff; 
  *AT91C_PIOD_PER   = 1; 
  *AT91C_PIOD_PPUDR = 1; 
  
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

void Write_Data_2_Display(unsigned char Data)
{
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

void Write_Command_2_Display(unsigned char Command) { 
  while((Read_Status_Display()) & (0xC) != 0xC){} 

    *AT91C_PIOC_OER  = (255<<2); // output enable pin 34 - 41 
    *AT91C_PIOC_CODR = (255<<2); // clear databus 

    *AT91C_PIOC_SODR = ((unsigned int)Command)<<2; 
    *AT91C_PIOC_CODR = (3<<12); // enabel output (clear OE) 
    *AT91C_PIOC_OER  = (255<<2);// output enable pin 34 - 41 
    *AT91C_PIOC_SODR = (1<<14); // set C/D 
    *AT91C_PIOC_CODR = (5<<15); // clear CE chip select display & clr wr disp

    Delay(200); // Originally Delay(20000);

    *AT91C_PIOC_SODR = (1<<17); // set write display 
    *AT91C_PIOC_SODR = (9<<12); // disable output (set OE) 
    *AT91C_PIOC_ODR = (255<<2); // output disable pin 34 - 41 
} 

unsigned char Read_Status_Display()
{
    unsigned char temp; 

    /*
    *AT91C_PMC_PCER=(3<<13);      // Port C, D
    *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
    *AT91C_PIOC_PPUDR = 0xffffffff; 
    *AT91C_PIOD_PER   = 1; 
    *AT91C_PIOD_PPUDR = 1; */
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
  {
    clearDisplay(0x00,0x00);
  }
  if(time.decimalPointer < 14)
  {
    if(9 < inp)
    {
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
    else
    {
      Write_Data_2_Display(digits[inp]);
      //Delay(2000000);
      Write_Command_2_Display(0xc0);
    }
    assignToSeq((inp == 11 ? 0 : inp));
  } 
  else 
    addSecondToConfiguredCalender();
}

void clearDisplay(int x, int y)
{
  locationOnDisplay(x,y);

  for(int i=0;i<(30*16);i++) // clear display
  {
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xC0);
  }
  locationOnDisplay(0x00,0x00);
}

void locationOnDisplay(int x,int y)
{
  Write_Data_2_Display(x);
  Write_Data_2_Display(y); 
  Write_Command_2_Display(0x24);
}

void display(char ch[]) {
  clearDisplay(0x00,0x00);
  //char temp;

  for(m = 0; '\0' !=ch[m];m++)
  {
    temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void rawDisplay(char ch[]) {
  for(char m = 0; '\0' !=ch[m];m++) {
    char temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void displayInt(int inp) {
  int lg = (inp == 0 ? 0 : (int)log10(inp));

  for(int i = lg; 0 <= i; i--) {
    int digit = getDigit(i, inp);
    Write_Data_2_Display(digits[digit]);
    Write_Command_2_Display(0xc0);
  }
}

void assignToSeq(short inp)
{
 //clearDisplay(0x00,0x00);
 //if(time.decimalPointer == 14) {}
 if(time.decimalPointer < 14)
 {
   time.sequence[time.decimalPointer] = inp;
   if(time.decimalPointer == 13)
     assignDigitToCalender(time.sequence);
 }
 time.decimalPointer++;
}

void assignDigitToCalender(uint8_t seq[])
{
 int yearPow = 3;
 int otherPow = 1;
 int offset = 0;
 for(int i = 0; i < 14; i++)
 {
  int digit = seq[i];

  if(4 <= i && i <= 7)
  {
    time.date[2] += (digit * (int)(pow(10, yearPow--)));
    offset = 2;
  }
  else
  {
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

void moveToNextRow(int spaces) {
  for(int i = 0; i < spaces; i++) {
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xc0);
  }
}

void displayOptions() {
  clearDisplay(0x00,0x00);
  
  rawDisplay("Press 2 for Day 1");
  moveToNextRow(30 - 17);
  rawDisplay("Press 3 for Day 2");
  moveToNextRow(30 - 17);
  rawDisplay("Press 4 for Day 3");
  moveToNextRow(30 - 17);
  rawDisplay("Press 5 for Day 4");
  moveToNextRow(30 - 17);
  rawDisplay("Press 6 for Day 5");
  moveToNextRow(30 - 17);
  rawDisplay("Press 7 for Day 6");
  moveToNextRow(30 - 17);
  rawDisplay("Press 8 for Day 7");
  moveToNextRow(30 - 17);
  moveToNextRow(30);
  rawDisplay("Press * for fastMode");
}

void displayDay() {
  clearDisplay(0x00,0x00);
  
  rawDisplay("DD/MM/YYYY");
  moveToNextRow(30 - 10);
  moveToNextRow(30);
  rawDisplay("Max: 29  hh/mm/ss");
  moveToNextRow(30 - 17);
  moveToNextRow(30);
  rawDisplay("Avg: 26  ");
  moveToNextRow(30 - 17 + 8);
  moveToNextRow(30);
  rawDisplay("Min: 23  hh/mm/ss");
}

void addSecondToConfiguredCalender() 
{
  int carry = 0;
  if(time.date[5] == 59) 
  { // Seconds
    time.date[5] = 0;
    carry = 1;
  }
  else 
   time.date[5]++; 
  if(carry + time.date[4] == 60) 
  { // Minutes
    time.date[4] = 0;
    carry = 1;
  } 
  else 
  { 
    time.date[4] = time.date[4] + carry;
    carry = 0;
  }
  if(carry + time.date[3] == 24) 
  { // Hours
    time.date[3] = 0;
    carry = 1;
    switchDay();
  }
  else 
  { 
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) 
  { // Days
    time.date[0] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) 
  { // Months
    time.date[1] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  //displayTime();
}

void addTwentyMinutesToConfiguredCalender()
{
  int carry = 0;
  // Seconds won't change because only minutes and upwards are incremented
  if(20 + time.date[4] >= 60) { // Minutes, 20 minute increment
    time.date[4] = time.date[4] % 60;
    carry = 1;
  } 
  else 
  { 
    time.date[4] = time.date[4] + 20;
  }
  if(carry + time.date[3] == 24) 
  { // Hours
    time.date[3] = 0;
    carry = 1;
  }
  else 
  { 
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) 
  { // Days
    time.date[0] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) 
  { // Months
    time.date[1] = 1;
    carry = 1;
  } 
  else 
  {
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  //displayTime();
}

void SysTick_Handler(void)
{
  //clockCounter++;
  // 10x speed clock
  if((clockCounter++ % 10 == 0) && (time.decimalPointer == 14) && (fastMode != 1))
  {
    addSecondToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
  /*
  if((clockCounter++ % 100 == 0) && (time.decimalPointer == 14) && (speed == 1))
  {
    addTwentyMinutesToConfiguredCalender(); 
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
  */
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

void StartMeasureTemp(void)
{  
  CreatePulseInPin2(672000); // Create a reset pulse, 8ms long 
  //Create a startpuls with a Delay(25)   
  CreatePulseInPin2(25);  

  //*AT91C_TC0_CCR  = (*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4;// SW reset, TC0_CCR0 
  *AT91C_TC0_CCR  = ((*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4);// SW reset, TC0_CCR0   
  *AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IER = (1 << 6); // Enable interrupt for LDRBS   
} 

void CreatePulseInPin2(int pulseWidth)
{   
  *AT91C_PIOB_OER  = (1 << 25); // Make pin 2 an output   
  *AT91C_PIOB_CODR = (1 << 25); // Put a low signal on the I/O line   
  Delay(pulseWidth); 
  *AT91C_PIOB_SODR = (1 << 25); // Put a high signal on the I/O line   
  *AT91C_PIOB_ODR  = (1 << 25); // Make pin 2 an input   
} 

void TC0_Handler()
{   
  //*AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IDR = (1 << 6); // Disable interrupt for LDRBS   
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator()
{
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273)
  {
    // To prevent the screen from changing if the temprature hasn't changed
    temprature = (deltaT / 210) - 273;
    //display("Temprature: ");
    //displayInt(temprature);
  }
}

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement)
{
  LinkedList *el = (LinkedList*)malloc(sizeof(LinkedList)); // Still needs to freed after use
  if(el != NULL)
  {
    for(int i = 0; i < 6; i++) { (el->timeStamp)[i] = dateAndTime[i]; }
    el->temprature = tempratureMeasurement;
    el->next       = head;
    head = el;
    linkedListLength++;
    measurementCount++;
  }
  if(linkedListLength == 250)
  {
    memoryIsFull_FLAG = 1;
    DeleteList(&head, stats);
    // Program will arrive here if memory is full.
  }
}

void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement) {
  LinkedList initial;
  
  for(int i = 0; i < 6; i++) { initial.timeStamp[i] = dateAndTime[i]; }
  initial.temprature = tempratureMeasurement;
  initial.next       = NULL;
  
  for(int i = 0; i < 6; i++) { (first)->timeStamp[i] = initial.timeStamp[i]; }
  (first)->temprature = initial.temprature;
  first->next         = initial.next;
  
  linkedListLength = 1;
  measurementCount++;
}

void DeleteList(LinkedList **first, StatsForADay stats[7]) {
  LinkedList *holderHolder;
  int max = stats[0].max;
  int min = stats[0].min;
  
  short timeStampMax[6];
  short timeStampMin[6];
  short timeStampTemp[7 - 1];
  
  for(int i = 0; i < 6; i++) { timeStampTemp[i] = head->timeStamp[i];}
  while(head != NULL) {
    if(max < head->temprature) {
      max = head->temprature;
      for(int i = 0; i < 6; i++) {timeStampMax[i] = head->timeStamp[i];}
    }
    if(head->temprature < min) {
      min = head->temprature;
      for(int i = 0; i < 6; i++) {timeStampMin[i] = head->timeStamp[i];}
    }
    totalMeasuredTemp += (head->temprature);
    holderHolder = head->next;
    free(head);
    head = holderHolder;
    /*if(head->next == NULL)
    {
      for(int i = 0; i < 6; i++)
      {
        timeStampTemp[i] = head->timeStamp[i];
      }
    }*/
  }
  
  if(stats[whatDayInStatsPointer].max < max) { 
    // Checking if the values measured prevously in the day are greater or less than the one in this measurement
    stats[whatDayInStatsPointer].max = max;
    for(int i = 0; i < 6; i++) { stats[whatDayInStatsPointer].timeStampMax[i] = timeStampMax[i];}
  }
  
  if(min < stats[whatDayInStatsPointer].min) {
    stats[whatDayInStatsPointer].min = min;
    for(int i = 0; i < 6; i++) { stats[whatDayInStatsPointer].timeStampMin[i] = timeStampMin[i];}
  }
  
  head = (LinkedList*)malloc(sizeof(LinkedList));
  for(int i = 0; i < 6; i++) { head->timeStamp[i] = timeStampTemp[i];}
  isFirstTimeMeasured = 1;
  linkedListLength = 0;
  memoryIsFull_FLAG = 0;
}

void switchDay() {
  /* 
  Se vad Magnus har gjort i addSecondToConfiguredCalender() 
  Kopiera hit och repetera i addTwentyMinutesToConfiguredCalender()
  Övrigt: Testa addTwentyMinutesToConfiguredCalender() och se om den funkar
  */
  
  DeleteList(&head, stats);
  
  // Calculate avrage temp and assign to the day
  stats[whatDayInStatsPointer].avg = totalMeasuredTemp / measurementCount;
  
  // Reset counters and such
  measurementCount  = 0;
  totalMeasuredTemp = 0;
  
  whatDayInStatsPointer++; // Move on to the next day
  // OPPPPSSS!!! Implement logic to keep whatDayInStatsPointer under 7, aka 7days.
            
}

------------------------------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------------------------------
 
#include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct calender {
 short date[6];
 uint8_t decimalPointer;
 uint8_t sequence[14];
} calender;

typedef struct LinkedList
{
  short timeStamp[6]; 
  short temprature; 
  struct LinkedList *next; 
} LinkedList;

typedef struct StatsForADay
{
  short timeStampMax[6]; 
  short max; 
  short avg;
  short timeStampMin[6]; 
  short min;
} StatsForADay;

short linkedListLength = 0;      // A length of 1440 means 1 day of measuring has been achieved
short measurementCount = 0;      // Used as total sample count to calculate the average temprature for a day
short whatDayInStatsPointer = 0; // Keep track of what day it is when measuring samples
short sevenDaysHasPassed    = 0; // 0 if false

int try = 0;

unsigned int i = 0;
short trigger  = 0; 
unsigned int TEMPRATURE_INTERRUPT_FLAG = 0; // Defining global flag
unsigned int LIGHT_INTERRUPT_FLAG      = 0; // Defining global flag 
unsigned int deltaT = 0;  
int temprature = 0;  
int TCA = 0;  
int TCB = 0;

calender time;
int months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

unsigned int PRESCAL = 2; 
double light1 = 0; 
unsigned int status  = 0; 

int totalMeasuredTemp = 0;
char temp;
int m;
int p, o = 79;
char c[1];
char ch=0x0;
int digit;
int clockCounter = 1;
int secSinceLastTempMeasurement = 0;
int isFirstTimeMeasured = 1;
int fastMode = 1; // fastMode = false,
char memoryIsFull_FLAG = 0; // will be set if no more memory can get allocated

int day0 = 0;
int day1 = 0;
int day2 = 0;
int day3 = 0;
int day4 = 0;
int day5 = 0;
int temp101 = 0;

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
void rawDisplay(char ch[]);
void displayOptions();
void displayDay();
int getDigit(int spot, int n){return (int)((n % (int)(pow(10,spot+1))) / pow(10,spot));}

void assignDigitToCalender(uint8_t seq[]);
void assignToSeq(short inp);
void displayTime();
void addSecondToConfiguredCalender();
void addTwentyMinutesToConfiguredCalender();

void CreatePulseInPin2(int pulseWidth);
void tempInit(void);
void StartMeasureTemp(void);
void TC0_Handler(void);
void tempCalculator();

void SysTick_Handler(void);

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void DeleteList(LinkedList **first, StatsForADay stats[7]);
void switchDay();

void servoInit();
void Serv_Rotate(double degree);

void Set_Led(int nLed);

void StartMeasureLight(void);
void lightInit();
void ADC_Handler();


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
LinkedList *head;
StatsForADay stats[7];

void main(void)
{
   head = (LinkedList*)malloc(sizeof(LinkedList));
   //StatsForADay stats[7];
   SystemInit();
   SysTick_Config(SystemCoreClock * .01); // 84MHz * 1, 840 000 ticks/sec <=> 100 interrupt/sec
   //char ch=0x0;
   *AT91C_PIOC_OER   = (1<<6) | (255 << 12);
   
   Init_Display(); 
   Write_Data_2_Display(0); 
   Write_Data_2_Display(0); 
   Write_Command_2_Display(0x24); 
   Delay(300000); 
   clearDisplay(0x00,0x00);
   
   *AT91C_PIOD_PER = (1<<3); 
   *AT91C_PIOD_OER = (1<<3); 
   *AT91C_PIOD_PPUDR = (1<<3);
   
   tempInit();  
   StartMeasureTemp();
   servoInit();
   lightInit();
   StartMeasureLight();
   Serv_Rotate(0.0);
      
   //struct LinkedList list;
   (head->timeStamp)[0] = 14;
   
   // 20/10/2021 - 20/55/55
   time.date[0] = 20;
   time.date[1] = 10;
   time.date[2] = 2021;
   time.date[3] = 22;
   time.date[4] = 34;
   time.date[5] = 30;
   time.decimalPointer = 14;
   
   stats[0].min = 1000;
   stats[0].max = 0;
   
   //*AT91C_PIOD_SODR = (1<<3); 
   Set_Led(1);
   ////////////////////////////
   initLL(head, time.date, temprature);
   isFirstTimeMeasured = 0;
   ///////////////////////////
   
   while(1)
   {
     if(TEMPRATURE_INTERRUPT_FLAG == 1)
     {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
     }
     
      if(0) { StartMeasureLight(); }  // Condition always false for debugging purposes
      if(LIGHT_INTERRUPT_FLAG == 1) { 
          if((status & 0x4) == 0x4) {  
            //light2 = *AT91C_ADCC_CDR2; 
            light1 = (double)(*AT91C_ADCC_CDR2&0x3ff)*(3.3/4096); 
          } 
          /*if((status & 0x2) == 0x2) { 
            //light1 = *AT91C_ADCC_CDR1; 
            light1 = (double)(*AT91C_ADCC_CDR1&0x3ff)*(3.3/4096); 
          } */
        LIGHT_INTERRUPT_FLAG = 0;   // Setting global flag  
        StartMeasureLight();
      }
     
     /*if(o == 79)
     {
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
     } 
     else
     {
       continue;
     }*/
     if((secSinceLastTempMeasurement == 1) && (sevenDaysHasPassed == 0)) 
     { // Req 2 in project
       
       if(isFirstTimeMeasured == 1)
       {
         initLL(head, time.date, temprature);
         isFirstTimeMeasured = 0;
       }
       else 
       {
         insertFirst(head, time.date, temprature);
       }
       secSinceLastTempMeasurement = 0;
       //displayOptions();
       //displayDay();
     }
   }
}

int returnKeypadValue(short* trigger)
{
   *AT91C_PMC_PCER   = (3<<13);                        // Enable control over PIOC and PIOD, also defined in InitDisplay()
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
        if((*AT91C_PIOC_PDSR & (1<<rowOrder[l])) == 0)
        {
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

void Delay(int Value)
{
  int i;  
  for(i=0;i<Value;i++)  
    asm("nop");  
} 

void Init_Display(void)
{
  *AT91C_PMC_PCER=(3<<13);      // Port C, D

  *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
  *AT91C_PIOC_PPUDR = 0xffffffff; 
  *AT91C_PIOD_PER   = 1; 
  *AT91C_PIOD_PPUDR = 1; 
  
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

void Write_Data_2_Display(unsigned char Data)
{
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

void Write_Command_2_Display(unsigned char Command) 
{ 
  while((Read_Status_Display()) & (0xC) != 0xC){} 

    *AT91C_PIOC_OER  = (255<<2); // output enable pin 34 - 41 
    *AT91C_PIOC_CODR = (255<<2); // clear databus 

    *AT91C_PIOC_SODR = ((unsigned int)Command)<<2; 
    *AT91C_PIOC_CODR = (3<<12); // enabel output (clear OE) 
    *AT91C_PIOC_OER  = (255<<2);// output enable pin 34 - 41 
    *AT91C_PIOC_SODR = (1<<14); // set C/D 
    *AT91C_PIOC_CODR = (5<<15); // clear CE chip select display & clr wr disp

    Delay(200); // Originally Delay(20000);

    *AT91C_PIOC_SODR = (1<<17); // set write display 
    *AT91C_PIOC_SODR = (9<<12); // disable output (set OE) 
    *AT91C_PIOC_ODR = (255<<2); // output disable pin 34 - 41 
} 

unsigned char Read_Status_Display()
{
    unsigned char temp; 

    /*
    *AT91C_PMC_PCER=(3<<13);      // Port C, D
    *AT91C_PIOC_PER   = 0xffffffff; // Enable every pins
    *AT91C_PIOC_PPUDR = 0xffffffff; 
    *AT91C_PIOD_PER   = 1; 
    *AT91C_PIOD_PPUDR = 1; */
    *AT91C_PIOC_ODR   = (255<<2); // output disable pin 34 - 41 
    *AT91C_PIOC_OER   = 0xFF;     // output enable pin 44 - 51
    *AT91C_PIOC_SODR  = (1<<13);  //  DIR, pin 50 
//  *AT91C_PIOD_OER   = 1;        // output enable pin 25 
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
  {
    clearDisplay(0x00,0x00);
  }
  if(time.decimalPointer < 14)
  {
    if(9 < inp)
    {
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
    else
    {
      Write_Data_2_Display(digits[inp]);
      //Delay(2000000);
      Write_Command_2_Display(0xc0);
    }
    assignToSeq((inp == 11 ? 0 : inp));
  } 
  else 
    addSecondToConfiguredCalender();
}

void clearDisplay(int x, int y)
{
  locationOnDisplay(x,y);

  for(int i=0;i<(30*16);i++) // clear display
  {
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xC0);
  }
  locationOnDisplay(0x00,0x00);
}

void locationOnDisplay(int x,int y)
{
  Write_Data_2_Display(x);
  Write_Data_2_Display(y); 
  Write_Command_2_Display(0x24);
}

void display(char ch[]) 
{
  clearDisplay(0x00,0x00);
  //char temp;

  for(m = 0; '\0' !=ch[m];m++)
  {
    temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void rawDisplay(char ch[]) 
{
  for(char m = 0; '\0' !=ch[m];m++) 
  {
    char temp = ch[m] - 0x20;
    Write_Data_2_Display(temp);
    Write_Command_2_Display(0xC0);
  }
}

void displayInt(int inp) 
{
  int lg = (inp == 0 ? 0 : (int)log10(inp));

  for(int i = lg; 0 <= i; i--) 
  {
    int digit = getDigit(i, inp);
    Write_Data_2_Display(digits[digit]);
    Write_Command_2_Display(0xc0);
  }
}

void assignToSeq(short inp)
{
 //clearDisplay(0x00,0x00);
 //if(time.decimalPointer == 14) {}
 if(time.decimalPointer < 14)
 {
   time.sequence[time.decimalPointer] = inp;
   if(time.decimalPointer == 13)
     assignDigitToCalender(time.sequence);
 }
 time.decimalPointer++;
}

void assignDigitToCalender(uint8_t seq[])
{
 int yearPow = 3;
 int otherPow = 1;
 int offset = 0;
 for(int i = 0; i < 14; i++)
 {
  int digit = seq[i];

  if(4 <= i && i <= 7)
  {
    time.date[2] += (digit * (int)(pow(10, yearPow--)));
    offset = 2;
  }
  else
  {
    time.date[(i - offset) / 2] += (digit * (int)(pow(10, otherPow)));
    otherPow = (otherPow + 1) % 2; // Flip between 1 and 0
  }
 }
 displayTime();
}

void displayTime() 
{
 clearDisplay(0x00,0x00);

 for(int l = 0; l < 6; l++) 
 {
  int num = time.date[l];
  int lg  = (num == 0 ? 0 : (int)log10(num));
  if(lg == 0) 
  {
    Write_Data_2_Display(0x10);
    Write_Command_2_Display(0xc0);
  }
  for(int h = lg; 0 <= h; h--) 
  {
    int digit = getDigit(h, num);
    char ch = digits[digit];
    Write_Data_2_Display(ch);
    Write_Command_2_Display(0xc0);
  }
  if(l != 5) 
  {
    Write_Data_2_Display((l == 2 ? 0x00 : 0x0F));
    Write_Command_2_Display(0xc0);
  }
 }

 o=1;
 if(time.decimalPointer == 13)
 {
  int boolean = 0;    // boolean = 0, means input values makes up valid time
  if((12 < time.date[1]) || (time.date[2] < 2021) || (23 < time.date[3]) || (59 < time.date[4]) || (59 < time.date[5])) 
  {
    boolean = 1; 
  }
  if(boolean == 0 && (months[time.date[1] - 1] < time.date[0])) 
  {
    boolean = 1;
  }
  if(boolean == 1) 
  { 
    display("INVALID INPUT!-RESTART PROGRAM");
    time.decimalPointer = 15; // To inactivate the if-statement in the sysTick-handler
  }
 }
}

void moveToNextRow(int spaces) 
{
  for(int i = 0; i < spaces; i++) 
  {
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xc0);
  }
}

void displayOptions()
{
  clearDisplay(0x00,0x00);
  
  rawDisplay("Press 2 for Day 1");
  moveToNextRow(30 - 17);
  rawDisplay("Press 3 for Day 2");
  moveToNextRow(30 - 17);
  rawDisplay("Press 4 for Day 3");
  moveToNextRow(30 - 17);
  rawDisplay("Press 5 for Day 4");
  moveToNextRow(30 - 17);
  rawDisplay("Press 6 for Day 5");
  moveToNextRow(30 - 17);
  rawDisplay("Press 7 for Day 6");
  moveToNextRow(30 - 17);
  rawDisplay("Press 8 for Day 7");
  moveToNextRow(30 - 17);
  moveToNextRow(30);
  rawDisplay("Press * for fastMode");
}

void displayDay()
{
  clearDisplay(0x00,0x00);
  
  rawDisplay("DD/MM/YYYY");
  moveToNextRow(30 - 10);
  moveToNextRow(30);
  rawDisplay("Max: 29  hh/mm/ss");
  moveToNextRow(30 - 17);
  moveToNextRow(30);
  rawDisplay("Avg: 26  ");
  moveToNextRow(30 - 17 + 8);
  moveToNextRow(30);
  rawDisplay("Min: 23  hh/mm/ss");
  moveToNextRow(30 - 17);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  rawDisplay("Press 0 to return");
}

void addSecondToConfiguredCalender() 
{
  int carry = 0;
  if(time.date[5] == 59) { // Seconds
    time.date[5] = 0;
    carry = 1;
  }
  else 
   time.date[5]++; 
  if(carry + time.date[4] == 60) 
  { // Minutes
    time.date[4] = 0;
    carry = 1;
  } 
  else 
  { 
    time.date[4] = time.date[4] + carry;
    carry = 0;
  }
  if(carry + time.date[3] == 24) 
  { // Hours
    time.date[3] = 0;
    carry = 1;
    switchDay();
  }
  else 
  { 
    time.date[3] = time.date[3] + carry;
    if(time.date[3] == 6)
         Serv_Rotate(10.0);
    if(time.date[3] == 22)
         Serv_Rotate(100.0);
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) 
  { // Days
    time.date[0] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) 
  { // Months
    time.date[1] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  //displayTime();
}

void addTwentyMinutesToConfiguredCalender()
{
  int carry = 0;
  // Seconds won't change because only minutes and upwards are incremented
  if(20 + time.date[4] >= 60)  // Minutes, 20 minute increment
  {
    time.date[4] = (20 + time.date[4]) % 60;
    carry = 1;
  } 
  else
  {
    if(time.date[3] == 6)
    {
      Serv_Rotate(10.0);
      Set_Led(1);
    }
    if(time.date[3] == 22)
    {
      Serv_Rotate(100.0);
      Set_Led(0);
    } 
    time.date[4] = time.date[4] + 20; 
  }
  if(carry + time.date[3] == 24) 
  { // Hours
    time.date[3] = 0;
    carry = 1;
    switchDay();
  }
  else 
  {
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) 
  { // Days
    time.date[0] = 1;
    carry = 1;
  } 
  else 
  { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  if(carry + time.date[1] == 13) 
  { // Months
    time.date[1] = 1;
    carry = 1;
  } 
  else 
  {
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
  //displayTime();
}

void SysTick_Handler(void)
{
  // 10x speed clock
  if((clockCounter++ % 10 == 0) && (time.decimalPointer == 14))
  {
    if(fastMode == 0)
    {
      addSecondToConfiguredCalender();
    }
    if(fastMode == 1)
    {
      addTwentyMinutesToConfiguredCalender();
    }
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
}

void tempInit()
{   
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

void StartMeasureTemp(void)
{  
  CreatePulseInPin2(672000); // Create a reset pulse, 8ms long 
  //Create a startpuls with a Delay(25)   
  CreatePulseInPin2(25);  

  //*AT91C_TC0_CCR  = (*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4;// SW reset, TC0_CCR0 
  *AT91C_TC0_CCR  = ((*AT91C_TC0_CCR & 0xFFFFFFFB) | 0x4);// SW reset, TC0_CCR0   
  *AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IER = (1 << 6); // Enable interrupt for LDRBS   
} 

void CreatePulseInPin2(int pulseWidth)
{   
  *AT91C_PIOB_OER  = (1 << 25); // Make pin 2 an output   
  *AT91C_PIOB_CODR = (1 << 25); // Put a low signal on the I/O line   
  Delay(pulseWidth); 
  *AT91C_PIOB_SODR = (1 << 25); // Put a high signal on the I/O line   
  *AT91C_PIOB_ODR  = (1 << 25); // Make pin 2 an input   
} 

void TC0_Handler()
{   
  //*AT91C_TC0_SR;             // Clearing old interrupts   
  *AT91C_TC0_IDR = (1 << 6); // Disable interrupt for LDRBS   
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator()
{
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273)
  {
    // To prevent the screen from changing if the temprature hasn't changed
    temprature = (deltaT / 210) - 273;
    //display("Temprature: ");
    //displayInt(temprature);
  }
}

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement)
{
  LinkedList *el = (LinkedList*)malloc(sizeof(LinkedList)); // Still needs to freed after use
  if(el != NULL)
  {
    for(int i = 0; i < 6; i++)
    {
      (el->timeStamp)[i] = dateAndTime[i];
    }
    el->temprature = tempratureMeasurement;
    el->next       = head;
    head = el;
    linkedListLength++;
    measurementCount++;
  }
  if(linkedListLength == 250)
  {
    memoryIsFull_FLAG = 1;
    DeleteList(&head, stats);
    // Program will arrive here if memory is full.
  }
}

void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement) 
{
  LinkedList initial;
  
  for(int i = 0; i < 6; i++) 
  {
    initial.timeStamp[i] = dateAndTime[i];
  }
  initial.temprature = tempratureMeasurement;
  initial.next       = NULL;
  
  for(int i = 0; i < 6; i++)
  {
    (first)->timeStamp[i] = initial.timeStamp[i];
  }
  (first)->temprature = initial.temprature;
  first->next         = initial.next;
  
  linkedListLength = 1;
  measurementCount++;
}

void DeleteList(LinkedList **first, StatsForADay stats[7]) 
{
  LinkedList *holderHolder;
  int max = stats[whatDayInStatsPointer].max;
  int min = stats[whatDayInStatsPointer].min;
  
  short timeStampMax[6];
  short timeStampMin[6];
  short timeStampTemp[7 - 1];
  
  for(int i = 0; i < 6; i++) 
  {
    timeStampTemp[i] = head->timeStamp[i];
  }
  while(head != NULL) 
  {
    if(max < head->temprature)
    {
      max = head->temprature;
      for(int i = 0; i < 6; i++) 
      {
        timeStampMax[i] = head->timeStamp[i];
      }
    }
    if(head->temprature < min)
    {
      min = head->temprature;
      for(int i = 0; i < 6; i++)
      {
        timeStampMin[i] = head->timeStamp[i];
      }
    }
    totalMeasuredTemp += (head->temprature);
    holderHolder = head->next;
    free(head);
    head = holderHolder;
  }
  
  if(stats[whatDayInStatsPointer].max < max) 
  { 
    // Checking if the values measured prevously in the day are greater or less than the one in this measurement
    stats[whatDayInStatsPointer].max = max;
    for(int i = 0; i < 6; i++)
    {
      stats[whatDayInStatsPointer].timeStampMax[i] = timeStampMax[i];
    }
  }
  
  if(min < stats[whatDayInStatsPointer].min) 
  {
    stats[whatDayInStatsPointer].min = min;
    for(int i = 0; i < 6; i++) 
    {
      stats[whatDayInStatsPointer].timeStampMin[i] = timeStampMin[i];
    }
  }
  
  head = (LinkedList*)malloc(sizeof(LinkedList));
  for(int i = 0; i < 6; i++)
  {
    head->timeStamp[i] = timeStampTemp[i];
  }
  isFirstTimeMeasured = 1;
  linkedListLength = 0;
  memoryIsFull_FLAG = 0;
}

void switchDay() 
{
  /* 
  Se vad Magnus har gjort i addSecondToConfiguredCalender() 
  Kopiera hit och repetera i addTwentyMinutesToConfiguredCalender()
  Övrigt: Testa addTwentyMinutesToConfiguredCalender() och se om den funkar
  */
  
  DeleteList(&head, stats);
  
  // Calculate avrage temp and assign to the day
  stats[whatDayInStatsPointer].avg = totalMeasuredTemp / measurementCount;
  
  // Reset counters and such
  measurementCount  = 0;
  totalMeasuredTemp = 0;
  
  whatDayInStatsPointer++; // Move on to the next day
  if(6 < whatDayInStatsPointer) 
  {
    sevenDaysHasPassed = 1;
  }
  else 
  {
    stats[whatDayInStatsPointer].max = 0;
    stats[whatDayInStatsPointer].min = 1000;
  }          
}

void servoInit()
{
  *AT91C_PMC_PCER = (1<<12); // PIOB
  *AT91C_PMC_PCER1 = (1<<4); // PWM
  *AT91C_PIOB_PDR = (1<<17); // Enable pin 62, Analog 8
  *AT91C_PIOB_OER = (1<<17);
  *AT91C_PIOB_PPUDR =(1<<17);

  *AT91C_PIOB_ABMR = (1<<17);

  *AT91C_PWMC_ENA = 0x2;

  *AT91C_PWMC_CH1_CMR = (
(*AT91C_PWMC_CH1_CMR & 0xFFFFFFF0) | 0x5);

  *AT91C_PWMC_CH1_CPRDR= 0xCD14; // 20ms
  *AT91C_PWMC_CH1_CDTYR= 0xA41; //1ms
}

void Serv_Rotate(double degree) 
{

  //currentDegree = degree;

  *AT91C_PWMC_CH1_CDTYUPDR = (int)
(1837*2+(29*(degree)
)
);

}
void Set_Led(int nLed)
{ 
  //*AT91C_PIOD_PER = (1<<3); 
  //*AT91C_PIOD_ODR = (1<<3); 
  //*AT91C_PIOD_PPUDR = (1<<3); 
  if(nLed == 0) 
  {
    *AT91C_PIOD_CODR = (1<<3); 
  } 
  else 
  {
    *AT91C_PIOD_SODR = (1<<3); 
  } 
}

void lightInit()
{ 
  *AT91C_PMC_PCER  = (1<<11);// Enabling peripheral PIOA 
  *AT91C_PMC_PCER1 = (1<<5); // ADC 
  *AT91C_PIOA_PER = (1 << 4);

  *AT91C_PIOA_PPUDR = (1 << 4); 

  *AT91C_PIOA_ODR = (1 << 4); 

  *AT91C_ADCC_MR  = (*AT91C_ADCC_MR & 0xFFFF00FF) | (PRESCAL << 8);// Setting PRESCAL, sampling rate:14MHz 
  *AT91C_ADCC_CHER = (1<<2); // Enable channel 2 

  NVIC_ClearPendingIRQ(ADC_IRQn); // Enable IO controller 37, TC0_IRQn = 37    
  NVIC_SetPriority(ADC_IRQn, 1);  // Set priority to first    
  NVIC_EnableIRQ(ADC_IRQn);       // Enable the interrupt  

  *AT91C_ADCC_SR; 
} 

void StartMeasureLight(void)
{ 
  *AT91C_ADCC_CR   = (1<<1);          // Enabling ADC 
  *AT91C_ADCC_SR; 
  *AT91C_ADCC_IER = (1<<2);  // Enabling interrupt for Ch 2 
} 

void ADC_Handler()
{ 
  status = *AT91C_ADCC_IMR; 
  *AT91C_ADCC_IDR = (1<<2); 
  LIGHT_INTERRUPT_FLAG = 1;     // Setting global flag  
}
