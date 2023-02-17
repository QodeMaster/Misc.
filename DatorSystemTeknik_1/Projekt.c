#include "system_sam3x.h"
#include "at91sam3x8.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct calender {
 short date[6];
 int8_t decimalPointer;
 uint8_t sequence[14];
} calender;

typedef struct LinkedList {
  short timeStamp[6]; 
  short temprature; 
  struct LinkedList *next; 
} LinkedList;

typedef struct StatsForADay {
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

int tempMax = 99, tempMin = 0;
char allowKeyPadInput = 0;
short tempSequence[4];
short tempSequencePointer = 0;

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

unsigned int PRESCAL  = 2; 
int light1         = 0; 
unsigned int status   = 0;
int totLight        = 300; // Default measurement 300mV
int totLightDivider = 1;   // Too avoid division with 0
int avgLightVoltage = 0;

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
void moveToNextRow(int spaces);
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
void displayTime(int from, int to, short date[6]);
void addSecondToConfiguredCalender();
void addTwentyMinutesToConfiguredCalender();

void CreatePulseInPin2(int pulseWidth);
void tempInit(void);
void StartMeasureTemp(void);
void TC0_Handler(void);
void tempCalculator();

void SysTick_Handler(void);
void newTimeConfig();

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void initLL(LinkedList *first, short dateAndTime[6], short tempratureMeasurement);
void DeleteList(LinkedList **first, StatsForADay stats[7]);
void switchDay();

void servoInit();
void Serv_Rotate(double degree);

void Set_Led(int nLed);
void Set_WarningLed(int nLed);
void configureTempLimit();
void assignTempSeq(int inp);

void StartMeasureLight(void);
void lightInit();
void lampInit();
void ADC_Handler();
void lightModeForSensor();
void darkModeForSensor();

LinkedList *head;
StatsForADay stats[7];

void main(void) {
   head = (LinkedList*)malloc(sizeof(LinkedList));
   SystemInit();
   SysTick_Config(SystemCoreClock * .01); // 84MHz * 1, 840 000 ticks/sec <=> 100 interrupt/sec
   *AT91C_PIOC_OER   = (1<<6) | (255 << 12);
   
   Init_Display(); 
   Write_Data_2_Display(0); 
   Write_Data_2_Display(0); 
   Write_Command_2_Display(0x24); 
   Delay(300000); 
   clearDisplay(0x00,0x00);
   
   lampInit();
   tempInit();  
   StartMeasureTemp();
   servoInit();
   lightInit();
   StartMeasureLight();
   Serv_Rotate(0.0);

   (head->timeStamp)[0] = 14;
   
   stats[0].min = 1000;
   stats[0].max = 0;
   
   while(1) {
     if(TEMPRATURE_INTERRUPT_FLAG == 1) {
       tempCalculator();
       TEMPRATURE_INTERRUPT_FLAG = 0; 
       StartMeasureTemp();
       if(tempMax <= temprature || tempMin >= temprature) { Set_WarningLed(1); } 
       else { Set_WarningLed(0); }
     }
    
      if((*AT91C_ADCC_LCDR & (0xf << 12)) == (1 << 13))  {
        light1  = ((*AT91C_ADCC_LCDR & (0xFFF << 0)));
        StartMeasureLight();
      }
     
     if(o == 79) {
       locationOnDisplay(0x28,0x00);
       display("Configure: DD/MM/YYYY hh:mm:ss"); // 30 Character wide screen
       o=59;
     }
     
     p = returnKeypadValue(&trigger); // p = keypad value 
     if(trigger == 1) {
       DisplayKeyboard(p);   // Displays the keypad value on the screen 
       Delay(5000000);
       trigger = 0;
     }
     
     if((secSinceLastTempMeasurement == 60) && (sevenDaysHasPassed == 0)) { // Req 2 in project
       
       if(isFirstTimeMeasured == 1) {
         initLL(head, time.date, temprature);
         isFirstTimeMeasured = 0;
       } else { insertFirst(head, time.date, temprature); 
       secSinceLastTempMeasurement = 0;
     }
     if((secSinceLastTempMeasurement >= 1) && (sevenDaysHasPassed == 0) && (fastMode == 1)) {
       
       if(isFirstTimeMeasured == 1) {
         initLL(head, time.date, temprature);
         isFirstTimeMeasured = 0;
       } else  { insertFirst(head, time.date, temprature); }
       secSinceLastTempMeasurement = 0;
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

     for(int k = 0; k < 3; k++) { // Col loop   
      *AT91C_PIOC_CODR = (1<<colOrder[k]);  // Clear Col   
      for(int l = 0; l < 4; l++) { // Row loop   
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

void Delay(int Value) 
  int i;  
  for(i=0;i<Value;i++)  
    asm("nop");  
} 

void Init_Display(void) 
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
  while((Read_Status_Display()) & (12) != 12){} 
  *AT91C_PIOC_OER  = (255<<2);               // output enable pin 34 - 41 
  *AT91C_PIOC_CODR = (255<<2);               // clear databus 
  *AT91C_PIOC_SODR = ((unsigned int)Data)<<2;

  *AT91C_PIOC_CODR = (3<<12);
  *AT91C_PIOC_OER  = (255<<2);
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
    *AT91C_PIOC_CODR = (3<<12);
    *AT91C_PIOC_OER  = (255<<2);// output enable pin 34 - 41 
    *AT91C_PIOC_SODR = (1<<14); // set C/D 
    *AT91C_PIOC_CODR = (5<<15); // clear CE chip select display & clr wr disp

    Delay(200);

    *AT91C_PIOC_SODR = (1<<17); // set write display 
    *AT91C_PIOC_SODR = (9<<12); // disable output (set OE) 
    *AT91C_PIOC_ODR = (255<<2); // output disable pin 34 - 41 
} 

unsigned char Read_Status_Display() 
    unsigned char temp; 

    *AT91C_PIOC_ODR   = (255<<2); // output disable pin 34 - 41 
    *AT91C_PIOC_OER   = 0xFF;     // output enable pin 44 - 51
    *AT91C_PIOC_SODR  = (1<<13);  //  DIR, pin 50 
    *AT91C_PIOC_CODR  = (1<<12);    //clear OE (enable the chip) 
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
  if(o==0) {
    clearDisplay(0x00,0x00);
  }
  if(time.decimalPointer < 14) {
    if(9 < inp) {
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
    }
    else {
      Write_Data_2_Display(digits[inp]);
      Write_Command_2_Display(0xc0);
    }
    assignToSeq((inp == 11 ? 0 : inp));
  }
  else if(allowKeyPadInput == 0) {
    if(inp == 11) { displayOptions(); }
    else if(2 <= inp && inp <= 8) { displayDay(inp - 1 - 1); }
    else if(inp == 10) { fastMode = (fastMode + 1) % 2; }
    else if(inp == 12) { newTimeConfig(); }
    else if(inp == 9 && o == 59) {
      allowKeyPadInput = 1;
      display("Configure Highest\\Lowest allowed temerature");
      o = 49;
    }
  } else if(allowKeyPadInput == 1) {
      assignTempSeq((inp == 11 ? 0 : inp));
      Write_Data_2_Display(digits[(inp == 11 ? 0 : inp)]);
      Write_Command_2_Display(0xc0);
  }
}

void clearDisplay(int x, int y) {
  locationOnDisplay(x,y);

  for(int i=0;i<(30*16);i++) { // clear display
    Write_Data_2_Display(0x00);
    Write_Command_2_Display(0xC0);
  }
  locationOnDisplay(0x00,0x00);
}

void locationOnDisplay(int x, int y) {
  Write_Data_2_Display(x);
  Write_Data_2_Display(y); 
  Write_Command_2_Display(0x24);
}

void display(char ch[]) {
  clearDisplay(0x00,0x00);
  rawDisplay(ch);
}

void rawDisplay(char ch[]) {
  for(char m = 0; '\0' != ch[m]; m++)   {
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

void newTimeConfig() {
  DeleteList(&head, stats);
  free(head);
  time.decimalPointer = 0;
  head = (LinkedList*)malloc(sizeof(LinkedList));
  linkedListLength = 0;      
  measurementCount = 0;      
  whatDayInStatsPointer = 0; 
  sevenDaysHasPassed    = 0; 
  totalMeasuredTemp = 0;
  clockCounter = 1;
  secSinceLastTempMeasurement = 0;
  isFirstTimeMeasured = 0;
  fastMode = 0; 
  o = 79;

  for(int i = 0; i < 14; i++) { time.sequence[i] = 0; }
  for(int i = 0; i < 7; i++) {
    stats[i].max = 0;
    stats[i].min = 1000;
    stats[i].avg = 0;
  }
  for(int i = 0; i < 6; i++) { time.date[i] = 0; }
 
  initLL(head, time.date, temprature);
}

void assignToSeq(short inp) {
 if(time.decimalPointer < 14) {
   time.sequence[time.decimalPointer] = inp;
   if(time.decimalPointer == 13)
     assignDigitToCalender(time.sequence);
 } else { displayOptions(); }
 time.decimalPointer++;
}

void assignDigitToCalender(uint8_t seq[]) {
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
 clearDisplay(0x00,0x00);
 displayTime(0, 6, time.date);
 moveToNextRow(30 - 19); 
 rawDisplay("Press 0 to Proceed");
}

void displayTime(int from, int to, short date[6]) {
 for(int l = from; l < to; l++) {
  int num = date[l];
  int lg  = (num == 0 ? 0 : (int)log10(num));
  if(lg == 0) {
    Write_Data_2_Display(0x10); // 0x10 = 0
    Write_Command_2_Display(0xc0);
  }
  for(int h = lg; 0 <= h; h--) {
    int digit = getDigit(h, num);
    char ch = digits[digit];
    Write_Data_2_Display(ch);
    Write_Command_2_Display(0xc0);
  }
  if(l < 3) {
    Write_Data_2_Display(l == 2 ? 0x00 : 0x0F);
    Write_Command_2_Display(0xc0);
  } else if(l < 5) {
    Write_Data_2_Display(0x1A);
    Write_Command_2_Display(0xc0);
  }
 }

 if(time.decimalPointer == 13) {
  int boolean = 0;    // boolean = 0, means input values makes up valid time
  if((12 < time.date[1] || time.date[1] == 0) || (time.date[2] < 2021) || (23 < time.date[3]) || (59 < time.date[4]) || (59 < time.date[5])) { boolean = 1; }
  if(boolean == 0 && (months[time.date[1] - 1] < time.date[0] || time.date[0] == 0)) { boolean = 1; }
  if(boolean == 1) { 
    display("INVALID INPUT!-RESTART CONFIG");
    time.decimalPointer = 15; // To inactivate the if-statement in the sysTick-handler
    Delay(10000000);
    newTimeConfig();
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
  rawDisplay("Press 9 for Temp Limit Config");
  moveToNextRow(30 - 29);
  rawDisplay("Press # for Time Config");
  moveToNextRow(30 - 23);
  moveToNextRow(30);
  rawDisplay("Press * for fastMode");
}

void displayDay(int dayToDisplay) {
  clearDisplay(0x00,0x00);
  if(dayToDisplay < whatDayInStatsPointer) {
    StatsForADay day = stats[dayToDisplay];
    
    displayTime(0, 3, day.timeStampMin);
    moveToNextRow(30 - 10 - 1);
    
    moveToNextRow(30);
    rawDisplay("Max: ");
    displayInt(day.max);
    rawDisplay("  ");
    displayTime(3, 6, day.timeStampMax);
    moveToNextRow(30 - 17);
    
    moveToNextRow(30);
    rawDisplay("Avg: ");
    displayInt(day.avg);
    rawDisplay("  ");
    moveToNextRow(30 - 17 + 8);
    moveToNextRow(30);
    
    rawDisplay("Min: ");
    displayInt(day.min);
    rawDisplay("  ");
    displayTime(3, 6, day.timeStampMin);
    moveToNextRow(30 - 17);
  } else {
    rawDisplay("Data for that day");
    moveToNextRow(30 - 17);
    rawDisplay("does not yet exist");
    moveToNextRow(30 - 18);
    moveToNextRow(30);
    moveToNextRow(30);
    moveToNextRow(30);
    moveToNextRow(30);
    moveToNextRow(30);
  }
  
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  moveToNextRow(30);
  rawDisplay("      Press 0 to return");
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
  } if(carry + time.date[3] == 24) { // Hours
    time.date[3] = 0;
    carry = 1;
    switchDay();
  } else { 
    time.date[3] = time.date[3] + carry;
    if(6 <= time.date[3] && time.date[3] < 22) { lightModeForSensor(); }
    else { darkModeForSensor(); }
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) { // Days
    time.date[0] = 1;
    carry = 1;
  } else { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  } if(carry + time.date[1] == 13) { // Months
    time.date[1] = 1;
    carry = 1;
  } else { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
}

void addTwentyMinutesToConfiguredCalender() {
  int carry = 0;
  // Seconds won't change because only minutes and upwards are incremented
  if(20 + time.date[4] >= 60) { // Minutes, 20 minute increment
    time.date[4] = (20 + time.date[4]) % 60;
    carry = 1;
  } else  {
    if(6 <= time.date[3] && time.date[3] < 22) { lightModeForSensor(); }
    else { darkModeForSensor(); }
    time.date[4] = time.date[4] + 20; 
  }
  if(carry + time.date[3] == 24) { // Hours
    time.date[3] = 0;
    carry = 1;
    switchDay();
  } else  {
    time.date[3] = time.date[3] + carry;
    carry = 0;
  }
  if(carry + time.date[0] == months[time.date[1] - 1] + 1) { // Days
    time.date[0] = 1;
    carry = 1;
  } else { 
    time.date[0] = time.date[0] + carry;
    carry = 0;
  } if(carry + time.date[1] == 13) { // Months
    time.date[1] = 1;
    carry = 1;
  } else {
    time.date[0] = time.date[0] + carry;
    carry = 0;
  }
  time.date[2] = time.date[2] + carry; // Year
}

void SysTick_Handler(void) {
  clockCounter += ((sevenDaysHasPassed + 1) % 2); // Stops adding if seven days are fullfilled
  avgLightVoltage = totLight / totLightDivider;
  if((clockCounter % 100 == 0) && (time.decimalPointer == 14)) {
    totLight        = 350;
    totLightDivider = 1;
    if(fastMode == 0) { addSecondToConfiguredCalender(); }
    if(fastMode == 1) { addTwentyMinutesToConfiguredCalender(); }
    clockCounter = 1;
    secSinceLastTempMeasurement++;
  }
}

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

void StartMeasureTemp(void) {  
  CreatePulseInPin2(672000); // Create a reset pulse, 8ms long 
  //Create a startpuls with a Delay(25)   
  CreatePulseInPin2(25);  

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
  *AT91C_TC0_IDR = (1 << 6); // Disable interrupt for LDRBS   
  TEMPRATURE_INTERRUPT_FLAG = 1;     // Setting global flag   
}

void tempCalculator() {
  TCA = *AT91C_TC0_RA;   
  TCB = *AT91C_TC0_RB;  

  deltaT = TCB - TCA;  
  // (1/210) = 1000000 * (1/(5 * TIMER_CLOCK1))
  if(temprature != (deltaT / 210) - 273) { temprature = (deltaT / 210) - 273; }
}

void insertFirst(LinkedList *first, short dateAndTime[6], short tempratureMeasurement) {
  LinkedList *el = (LinkedList*)malloc(sizeof(LinkedList));
  if(el != NULL) {
    for(int i = 0; i < 6; i++) { (el->timeStamp)[i] = dateAndTime[i]; }
    el->temprature = tempratureMeasurement;
    el->next       = head;
    head = el;
    linkedListLength++;
    measurementCount++;
  }
  if(linkedListLength == 250) { DeleteList(&head, stats); }
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
  int max = stats[whatDayInStatsPointer].max;
  int min = stats[whatDayInStatsPointer].min;
  int nrList = 0;
  short timeStampMax[6];
  short timeStampMin[6];
  short timeStampTemp[7 - 1];
  
  for(int i = 0; i < 6; i++) { timeStampTemp[i] = head->timeStamp[i]; }
  while(head != NULL) {
    nrList++;
    if(max < head->temprature) {
      max = head->temprature;
      for(int i = 0; i < 6; i++) { timeStampMax[i] = head->timeStamp[i]; }
    }
    if(head->temprature < min) {
      min = head->temprature;
      for(int i = 0; i < 6; i++) { timeStampMin[i] = head->timeStamp[i]; }
    }
    totalMeasuredTemp += (head->temprature);
    holderHolder = head->next;
    free(head);
    head = holderHolder;
  }
  
  if(stats[whatDayInStatsPointer].max < max) { 
    // Checking if the values measured prevously in the day are greater or less than the one in this measurement
    stats[whatDayInStatsPointer].max = max;
    for(int i = 0; i < 6; i++) { stats[whatDayInStatsPointer].timeStampMax[i] = timeStampMax[i]; }
  }
  
  if(min < stats[whatDayInStatsPointer].min) {
    stats[whatDayInStatsPointer].min = min;
    for(int i = 0; i < 6; i++) { stats[whatDayInStatsPointer].timeStampMin[i] = timeStampMin[i]; }
  }
  
  head = (LinkedList*)malloc(sizeof(LinkedList));
  for(int i = 0; i < 6; i++) { head->timeStamp[i] = timeStampTemp[i]; }
  isFirstTimeMeasured = 1;
  linkedListLength = 0;
}

void switchDay() {
  DeleteList(&head, stats);
  
  // Calculate avrage temp and assign to the day
  stats[whatDayInStatsPointer].avg = totalMeasuredTemp / measurementCount;
  
  // Reset counters and such
  measurementCount  = 0;
  totalMeasuredTemp = 0;
  
  whatDayInStatsPointer++; // Move on to the next day
  if(6 < whatDayInStatsPointer) { sevenDaysHasPassed = 1; }
  else {
    stats[whatDayInStatsPointer].max = 0;
    stats[whatDayInStatsPointer].min = 1000;
  }          
}

void servoInit() {
  *AT91C_PMC_PCER = (1<<12); // PIOB
  *AT91C_PMC_PCER1 = (1<<4); // PWM
  *AT91C_PIOB_PDR = (1<<17); // Enable pin 62, Analog 8
  *AT91C_PIOB_OER = (1<<17);
  *AT91C_PIOB_PPUDR =(1<<17);

  *AT91C_PIOB_ABMR = (1<<17);

  *AT91C_PWMC_ENA = 0x2;

  *AT91C_PWMC_CH1_CMR = ((*AT91C_PWMC_CH1_CMR & 0xFFFFFFF0) | 0x5);

  *AT91C_PWMC_CH1_CPRDR= 0xCD14; // 20ms
  *AT91C_PWMC_CH1_CDTYR= 0xA41; //1ms
}

void Serv_Rotate(double degree) { *AT91C_PWMC_CH1_CDTYUPDR = (int)(1837*2+(29*(degree))); }

void Set_Led(int nLed) { 
  if(nLed == 0) { *AT91C_PIOD_CODR = (1<<3); } 
  else { *AT91C_PIOD_SODR = (1<<3); } 
}

void lightInit() { 
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
  *AT91C_ADCC_CWR = (1 << 24);
} 

void lampInit() {
  *AT91C_PIOD_PER = (1<<3); 
  *AT91C_PIOD_OER = (1<<3); 
  *AT91C_PIOD_PPUDR = (1<<3);
  
  *AT91C_PIOD_PER = (1<<1); 
  *AT91C_PIOD_OER = (1<<1); 
  *AT91C_PIOD_PPUDR = (1<<1);
}

void StartMeasureLight(void) { *AT91C_ADCC_CR  = (1<<1); } // Enabling ADC 
 
void ADC_Handler() { 
  status = *AT91C_ADCC_IMR; 
  *AT91C_ADCC_IDR = (1<<2); 
  LIGHT_INTERRUPT_FLAG = 1;     // Setting global flag  
}

void lightModeForSensor() {
  Serv_Rotate(10.0); // Servo open, to let light throught
  if(700 < light1) { Set_Led(1); } 
  else { Set_Led(0); }
}

void darkModeForSensor() {
  Serv_Rotate(100.0); // Servo closed, to block potential light
  Set_Led(0);
}

void Set_WarningLed(int nLed) {
  if(nLed == 0) { *AT91C_PIOD_CODR = (1<<1); } 
  else { *AT91C_PIOD_SODR = (1<<1); }
}

void assignTempSeq(int inp) {
  tempSequence[tempSequencePointer++] = inp;
  if(4 <= tempSequencePointer) {
    allowKeyPadInput = 0;
    configureTempLimit();
  }
}

void configureTempLimit() {
  tempMax = 10 * tempSequence[0] + tempSequence[1];
  tempMin = 10 * tempSequence[2] + tempSequence[3];
}
