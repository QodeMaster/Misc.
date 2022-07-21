#include "kernel_functions.h"

int Ticks;                    /* global sysTick counter */
int KernelMode;               /* can be equal to either INIT or RUNNING (constants defined* in ?kernel_functions.h?)*/
TCB *PreviousTask, *NextTask; /* Pointers to previous and next running tasks */
list *ReadyList, *WaitingList, *TimerList;

void TimerInt() {
  Ticks++;
  listobj* timerTemp;
  listobj* timerTemp2;
  listobj* timerTemp3;
  
  timerTemp = TimerList->pHead;
  while (timerTemp != NULL) // Checking if taks should remain in TimerList or not {
    if(Ticks >= timerTemp->nTCnt) {
      removeTask(&TimerList, timerTemp);
      moveObj(&ReadyList, timerTemp);
      timerTemp = TimerList->pHead;
    } else { timerTemp = timerTemp->pNext; }
  }
  
  timerTemp2 = WaitingList->pHead;
  while (timerTemp2 != NULL) // Check if Deadline is reached in WaitingList {
    if(Ticks >=timerTemp2->pTask->Deadline) {
      removeTask(&WaitingList, timerTemp2);
      moveObj(&ReadyList, timerTemp2);
      timerTemp2 = timerTemp2->pNext;
    } else { break; }
  }
  
  timerTemp3 = TimerList->pHead;
  while (timerTemp3 != NULL) // Check if Deadline is reached in TimerList {
    if(Ticks >= timerTemp3->pTask->Deadline) {
      removeTask(&TimerList, timerTemp3);
      moveObj(&ReadyList, timerTemp3);
    }
    timerTemp3 = timerTemp3->pNext;
  }
  NextTask = ReadyList->pHead->pTask;
}

void idle() { while(1); }

exception init_kernel(void) {
  Ticks = 0;                   // Set tick counter to zero
  
  // Initialize necessary data structures
  ReadyList   = (list*)calloc(2, sizeof(listobj));
  WaitingList = (list*)calloc(2, sizeof(listobj));
  TimerList   = (list*)calloc(2, sizeof(listobj));
  
  ReadyList->pHead = NULL;     // NULL:ifying the start and the finish node
  ReadyList->pTail = NULL;
  
  create_task(&idle, UINT_MAX);// Create an Idle task
  KernelMode = INIT;           // Set the kernel in INIT mode
  // Return status
  return (ReadyList != NULL && WaitingList != NULL && TimerList != NULL) ? OK : FAIL;
}

exception create_task(void (*taskBody)(), unsigned int deadline) {
  TCB *new_tcb;
  new_tcb = (TCB *) calloc (1, sizeof(TCB));
  /* you must check if calloc was successful or not! */
 
  new_tcb->PC = taskBody;
  new_tcb->SPSR = 0x21000000;
  new_tcb->Deadline = deadline;
 
  new_tcb->StackSeg [STACK_SIZE - 2] = 0x21000000;
  new_tcb->StackSeg [STACK_SIZE - 3] = (unsigned int) taskBody;
  new_tcb->SP = &(new_tcb->StackSeg [STACK_SIZE - 9]);
  
  // after the mandatory initialization you can implement the rest of the suggested pseudocode
  if(KernelMode == INIT) {
    insertTask(ReadyList, new_tcb, deadline); // Insert new task in ReadyList
    return (new_tcb == NULL) ? FAIL : OK;     // Return status
  } else {
    isr_off();                                // Disable Interrupts
    PreviousTask = ReadyList->pHead->pTask;   // Update PreviousTask
    insertTask(ReadyList, new_tcb, deadline); // Insert new task in ReadyList
    NextTask     = ReadyList->pHead->pTask;   // Update NextTask
    SwitchContext();
  }
  
  return (new_tcb == NULL) ? FAIL : OK;       // Return status
}

void run(void) {
  Ticks = 0;                          // Initialize interrupt timer {Ticks = 0;}
  KernelMode = RUNNING;               // Set KernelMode = RUNNING
  NextTask = ReadyList->pHead->pTask; // Set NextTask to equal TCB of the task to beloaded
  LoadContext_In_Run();               // Load context using: { LoadContext_In_Run(); }
}

void terminate() {
  isr_off();                          // Disable interrupts
  
  listobj* leavingObj = (listobj*) extract(ReadyList->pHead);
  /* extract()detaches the head node from the ReadyList and
   * returns the list objectof the running task */
  
  NextTask = ReadyList->pHead->pTask; // Switch to process stack of task to beloaded { switch_to_stack_of_next_task(); }
  switch_to_stack_of_next_task();    // Set NextTask to equal TCB of the nexttask
  
  free(leavingObj->pTask);            // Remove data structures of task being terminated
  free(leavingObj);
  
  LoadContext_In_Terminate();         // Load context using: { LoadContext_In_Terminate(); }
}

listobj* extract(listobj *item) {
  listobj* holder  = ReadyList->pHead;
  
  ReadyList->pHead->pNext->pPrevious = NULL;
  ReadyList->pHead = holder->pNext;
  
  return holder;
}

mailbox* create_mailbox (uint nMessages, uint nDataSize) {
  //To Create the mail box
  mailbox* inbox;
  inbox = (mailbox*)malloc(sizeof(mailbox));
  //Declarations of pointers and variables in the mailbox
  inbox->pHead = NULL;
  inbox->pTail = NULL;
  inbox->nDataSize = nDataSize;
  inbox->nMaxMessages = nMessages;
  inbox->nMessages = 0;
  inbox->nBlockedMsg = 0;
  //Check if inbox exsists
  if (inbox == NULL) { return NULL; }
  return inbox;
}

exception remove_mailbox (mailbox* mBox) {
  // Removes messages if mailbox is empty
  if(!(no_messages(mBox)))
  {
    free(mBox -> pHead);
    free(mBox -> pTail);
    free(mBox);
    return OK;
  } else { return NOT_EMPTY; }
}
exception send_wait( mailbox* mBox, void* pData) {
  msg* message;
  isr_off(); //Disable interrupt
  msg* receivingTask = mBox -> pHead;       // Setting receivingTask as object pHead in mailbox
  if(receivingTask -> Status == RECEIVER) { // Check if receiving task is waiting
    memcpy(pData, receivingTask -> pData, mBox -> nDataSize); // Copy sender's data (pData) to data area of receivers message box(mBox)
    
    deleteMsg(mBox, receivingTask);                  // Removes message from mBox that corrisponds with receivingTask
    PreviousTask = ReadyList->pHead->pTask;          // Update PreviousTask
    removeTask(&WaitingList, receivingTask->pBlock); // Remove receiving task's message struct from mailbox
    moveObj(&ReadyList, receivingTask->pBlock);      // Insert new task in ReadyList
    NextTask = ReadyList->pHead->pTask;              // Update NextTask
  }
  
  else
  {
    message = (msg*)malloc(sizeof(msg)); // Allocate a Message structure Set data pointer
    //Set data pointer
    message->pData = (char*)malloc(mBox->nDataSize);
    if(message->pData == NULL)
    {
      free(message);
      return FAIL;
    }
    
    memcpy(message ->pData,pData, mBox ->nDataSize);
    message ->Status = SENDER;     
    message ->pNext = NULL;
    message ->pPrevious = NULL;
    message->pBlock = ReadyList->pHead;
    
    addTailMsgBox(message, mBox); //Add message last in the mailbox
    (mBox->nBlockedMsg)++;
    PreviousTask = ReadyList->pHead->pTask; // Update PreviousTask
    removeTask(&ReadyList, message->pBlock);
    moveObj(&WaitingList, message->pBlock); //Move sending task from ReadyList to WaitingList Update NextTask
    NextTask = ReadyList ->pHead ->pTask; // Update NextTask
  }
  SwitchContext(); // Switch context
  if(Ticks >= NextTask->Deadline)
  {
    isr_off(); // Disable interrupt
    deleteMsg(mBox, ReadyList->pHead->pMessage);
    isr_on(); // Enable interrupt
    return DEADLINE_REACHED; // Return DEADLINE_REACHED
  }
  
  else
    return OK;
}

exception receive_wait ( mailbox* mBox, void* pData)
{
  isr_off(); //Disable interrupt
  msg* sendingMsg = mBox -> pHead;
  if(sendingMsg -> Status == SENDER)
  {
    memcpy(pData, sendingMsg -> pData, mBox -> nDataSize); //Copy sender's data to data area of receivers message msg in (mBox)
    deleteMsg(mBox, sendingMsg);
    
    if(isMember(&WaitingList, sendingMsg->pBlock))
    {
      PreviousTask = ReadyList->pHead->pTask; // Update PreviousTask
      removeTask(&WaitingList, sendingMsg->pBlock);
      moveObj(&ReadyList, sendingMsg->pBlock); // Insert new task in ReadyList
      NextTask = ReadyList->pHead->pTask; // Update NextTask
      (mBox->nBlockedMsg)--;
    }
    else
    {
      free(sendingMsg->pData);
    }
  }
  else
  {
    msg* message = (msg*)malloc(sizeof(msg)); //Allocate memmory to massage struct
    if(message == NULL)
    {
      return FAIL;
    }
    
    message->pData = (char*)malloc(mBox->nDataSize);
    memcpy(message ->pData,pData, mBox ->nDataSize);
    message->Status = RECEIVER;
    message->pNext = NULL;
    message->pPrevious = NULL;
    message->pBlock = ReadyList->pHead;
    
    addTailMsgBox(message, mBox);
    PreviousTask = ReadyList->pHead->pTask; // Update PreviousTask
    removeTask(&ReadyList, message ->pBlock); //Remove Task from ReadyList
    moveObj(&WaitingList, message->pBlock); // Insert new task in WaitList
    NextTask = ReadyList ->pHead ->pTask; // Update NextTask
  }
  SwitchContext();
  if(Ticks >= NextTask->Deadline)
  {
    isr_off(); // Disable interrupt
    deleteMsg(mBox, ReadyList->pHead->pMessage);
    isr_on(); // Enable interrupt
    return DEADLINE_REACHED; // Return DEADLINE_REACHED
  }
  else
    return OK;
}
exception send_no_wait( mailbox *mBox, void *pData)
{
  isr_off(); //Disable interrupt
  msg* receivingTask = mBox->pHead;
  if(receivingTask->Status == RECEIVER) //Checking if a receiving task is waiting   //Needs to get called for testing
  {
    memcpy(pData, receivingTask -> pData, mBox -> nDataSize); //Copy sender's data (pData) to data area of receivers message box(mBox)
    //Remove receiving task's message struct from mailbox
    deleteMsg(mBox, receivingTask); //Removes message from mBox that corrisponds with receivingTask
    PreviousTask = ReadyList->pHead->pTask; // Update PreviousTask
    removeTask(&WaitingList, receivingTask->pBlock);
    moveObj(&ReadyList, receivingTask->pBlock); // Insert new task in ReadyList
    NextTask = ReadyList->pHead->pTask; // Update NextTask
  }
  else
  {
    msg* message = (msg*)malloc(sizeof(msg)); //Allocate memmory to massage struct
    if(message == NULL)
      return FAIL;
    message->pNext = NULL;
    message->pPrevious = NULL;
    message->pData = (char*)malloc(mBox->nDataSize);
    if(message->pData == NULL)
    {
      free(message);
      return FAIL;
    }
    memcpy(message->pData, pData, mBox->nDataSize);
    message->pBlock = NULL;
    message->Status = SENDER;
    
    if(mBox->nMaxMessages <= mBox->nMessages)
    {
      mBox->pHead = mBox->pHead->pNext;
      free(mBox->pHead->pPrevious);
    }
    addTailMsgBox(message, mBox);
    isr_on();
  }
  return OK;
}
exception receive_no_wait( mailbox* mBox, void* pData)
{
  msg* sendingMsg = mBox -> pHead;
  if(sendingMsg->Status == SENDER)
  {
    isr_off(); // Disable interrupt
    memcpy(pData, sendingMsg -> pData, mBox -> nDataSize);
    deleteMsg(mBox, sendingMsg);
    if(isMember(&WaitingList, sendingMsg->pBlock))                  //Needs to get called for testing
    {
      PreviousTask = ReadyList->pHead->pTask; // Update PreviousTask
      removeTask(&WaitingList, sendingMsg ->pBlock); // Remove task from waiting list
      moveObj(&ReadyList, sendingMsg->pBlock); // Move sending task to ReadyList
      NextTask = ReadyList->pHead->pTask; // Update NextTask
      SwitchContext();
    }
    else
    {
      free(sendingMsg->pData);
    }
  }
  else
  {
    return FAIL;
  }
  return OK;
}

exception wait(uint nTicks)
{
  isr_off();
  PreviousTask = ReadyList->pHead->pTask; // Update PreviousTask
  listobj* temp = ReadyList->pHead;
  temp->nTCnt = nTicks;
  removeTask(&ReadyList, temp);
  moveObj(&TimerList, temp);
  NextTask = ReadyList->pHead->pTask; // Update NextTask
  SwitchContext();
  
  if(Ticks >= NextTask->Deadline)//if deadline is reached THEN
    return DEADLINE_REACHED;
  else
    return OK;
}

void set_ticks( uint nTicks)
{
  Ticks = nTicks;
}

uint ticks(void)
{
  return Ticks;
}

uint deadline(void) 
{
  return NextTask->Deadline;
}

void set_deadline(uint deadline)
{
  isr_off();
  NextTask->Deadline = deadline;//Set deadline field in the calling TCB
  PreviousTask = ReadyList->pHead->pTask; // Update PreviousTask
  sortingLists(&ReadyList);//Reschedule ReadyList
  NextTask = ReadyList->pHead->pTask; // Update NextTask
  SwitchContext();
}
