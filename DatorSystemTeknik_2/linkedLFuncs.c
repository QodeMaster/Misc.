#include "linkedLFuncs.h"

void insertTask(list* DLL, TCB* new_tcb, int deadline) {
  listobj *holder = DLL->pHead;
  listobj *item   = (listobj*)malloc(sizeof(listobj));
  item->pTask     = new_tcb;
  item->nTCnt     = 0;
  item->pMessage  = NULL;
  item->pPrevious = NULL;
  item->pNext     = NULL;
  unsigned int i = 0;
  
  if(item == NULL) { { while(1); } }
  
  if(holder == NULL) {
    DLL->pHead = item; // Head & Tail node points to the same task
    DLL->pTail = item;
  } else {
    while(holder != NULL){
      if(deadline < holder->pTask->Deadline) {
        if(i == 0) {
          DLL->pHead = item;
          item->pNext                = holder;
          holder->pPrevious          = item;
        } else {
          listobj *holderOfHolder = holder;
          
          item->pPrevious            = holder->pPrevious;
          item->pNext                = holder;
          
          holderOfHolder->pPrevious->pNext = item;
          holderOfHolder->pPrevious        = item;
          break;
        }
      }
      holder = (holder->pNext);
      i++;
    }
  }
}

void moveObj(list** destinationDDL, listobj* obj) {
  if((*destinationDDL)->pHead == NULL) {
    if((*destinationDDL)->pTail == NULL) {
      (*destinationDDL)->pHead = obj;
      (*destinationDDL)->pTail = obj;
      return;
    } else if((*destinationDDL)->pTail == NULL) { (*destinationDDL)->pTail = obj; }
  } if(obj->pTask->Deadline > (*destinationDDL)->pTail->pTask->Deadline) {
    insertInListAfter(destinationDDL, (*destinationDDL)->pTail, obj);
    return;
  }
  listobj* temp;
  for(temp = (*destinationDDL)->pHead; temp->pNext != NULL; temp = temp->pNext) {
    if(temp->pTask->Deadline > obj->pTask->Deadline)
      break;
  }
  insertInListBefore(destinationDDL, temp, obj);
}

void insertInListBefore(list** destinationDDL, listobj* point, listobj* obj) {
  if((*destinationDDL)->pHead == point) {
    (*destinationDDL)->pHead = obj;
  } else {
    point->pPrevious->pNext = obj;
  }
  obj->pPrevious = point->pPrevious;
  point->pPrevious = obj;
  obj->pNext = point;
}

void insertInListAfter(list** destinationDDL, listobj* point, listobj* obj) {
  if((*destinationDDL)->pTail == point) {
    (*destinationDDL)->pTail = obj;
  } else {
    obj->pNext->pPrevious = obj;
  }
  obj->pNext = point->pNext;
  point->pNext = obj;
  obj->pPrevious = point;
}

void sortingLists(list** tempList) {
  list* listing = (list*)malloc(sizeof(list));
  //listing = NULL;
  listing->pHead = NULL;
  listing->pTail = NULL;
  listobj* movingTasks = NULL;
  while ((*tempList)->pHead != NULL) {
    movingTasks = (*tempList)->pHead;
    removeTask(tempList, movingTasks);
    moveObj(&listing, movingTasks);
  }
  *tempList = listing;
}

void deleteMsg(mailbox* mBox, msg* sms) {        //Removes messages from mailbox

  if (mBox->pHead == sms) {
    if(mBox -> pTail == sms) {
      mBox ->pHead = NULL;
      mBox ->pTail = NULL;
    } else {
      mBox->pHead = sms->pNext;
      mBox->pHead->pPrevious = NULL;
    }
  } else if (mBox->pTail == sms) {
    mBox->pTail = sms->pPrevious;
    mBox->pTail->pNext = NULL;
  } else {
    sms->pPrevious->pNext = sms->pNext;
    sms->pNext->pPrevious = sms->pPrevious;
  }
 (mBox->nMessages)--;
}

// Insert into the mailbox
void addTailMsgBox(msg* message, mailbox* mBox) {
  if(mBox-> pHead == NULL && mBox-> pTail == NULL) {
    mBox->pHead = message;
    mBox->pTail = message;
  } else {
    msg* tempMsg = mBox->pTail;
    mBox->pTail->pNext = message;
    mBox->pTail = mBox->pTail->pNext;
    mBox->pTail->pPrevious = tempMsg;
  }
  (mBox->nMessages)++;
}

bool isMember(list** name, listobj* node) {            //checkes if an object in the list is the object pointed to

  list* tempList = *name;
  listobj* tempObj = tempList->pHead;
  while (tempObj != NULL) {
    if (tempObj == node) { return TRUE; }
    tempObj = tempObj->pNext;
  }
  return FALSE;
}

//extract function
void removeTask(list **name, listobj *obj) {
  if ((*name)->pHead == NULL || obj == NULL)
    return;
  //if the obj is at the head of the list
  if ((*name)->pHead == obj) {
    if((*name)->pTail == obj) {
      (*name) -> pHead -> pNext= NULL;
      (*name) -> pHead -> pPrevious = NULL;
      (*name) -> pTail -> pNext = NULL;
      (*name) -> pTail -> pPrevious = NULL;
      (*name) -> pHead = NULL;
      (*name) -> pTail = NULL;
    } else {
      (*name) -> pHead = (*name) -> pHead -> pNext;
      (*name) -> pHead -> pPrevious = NULL;
    }
  }
  //If the obj is at the tail of the list
  else if((*name)->pTail == obj) {
    (*name)->pTail = (*name)->pTail ->pPrevious;
    (*name)->pTail ->pNext = NULL;
  } else {
    obj ->pPrevious ->pNext = obj ->pNext;
    obj -> pNext -> pPrevious = obj -> pPrevious;
  }
  obj -> pNext = NULL;
  obj -> pPrevious = NULL;
  return;
}
