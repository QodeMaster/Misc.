#include "kernel_functions.h"

void insertTask(list* DDL, TCB* new_tcb, int deadline);

void deleteMsg(mailbox* mBox, msg* sms);

void removeTask(list **name, listobj *node);

bool isMember(list** name, listobj* node);

void addTailMsgBox(msg* message, mailbox* mBox);

void moveObj(list** destinationDDL, listobj* obj);

void sortingLists(list** tempList);

void insertInListAfter(list** destinationDDL, listobj* point, listobj* obj);

void insertInListBefore(list** destinationDDL, listobj* temp, listobj* obj);
