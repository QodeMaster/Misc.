#include <stdio.h>
#include <stdlib.h>

struct LinkedList { 
  int id; 
  double sensorData; 
  struct LinkedList *next; 
};

void insertFirst(struct LinkedList **first, struct LinkedList *el) {
  struct LinkedList *temp = *first;
  *first = el;
  (*first)->next = temp;
}

void printLL(struct LinkedList **first) {
  struct LinkedList *holder = *first;
  while(holder != NULL){
    printf("Id: %i \t sensorData: %lf\n", holder->id, holder->sensorData);
    holder = (holder->next);
  }
}

int isMember(struct LinkedList **first, struct LinkedList *el) {
  struct LinkedList *holder = *first;
  while(holder != NULL){
    if((*el).sensorData == (*holder).sensorData) {return 1;}
    holder = (holder->next);
  }
  return 0;
}

void removeElAziziVersion(struct LinkedList **first, struct LinkedList *el){
  if(isMember(first, el) == 1) {
    if((*el).sensorData == (**first).sensorData) {
      *first = (*first)->next;
    } else {
      struct LinkedList *holder = *first;
      while(holder->next != NULL){
        if((*(holder->next)).sensorData == (*el).sensorData) {
          holder->next = (holder->next)->next;
          return;
        }
        holder = (holder->next);
      }
    }
  }
}

struct LinkedList *readSensor(int id) {
  struct LinkedList* pointer = (struct LinkedList*)malloc(sizeof(struct LinkedList));
  if(pointer != NULL) {
    pointer->id         = id;
    pointer->sensorData = rand() / ((double)RAND_MAX);
    pointer->next       = NULL;
  }
  return pointer;
}

void sorter(struct LinkedList **first) {
  for(int i = 0; i < 10; i++) {
    struct LinkedList *holder = *first;
    for(int j = 0; j < i; j++) { holder = holder->next; }
    struct LinkedList *max = holder;
    while(holder != NULL) {
      if((max->sensorData) < (holder->sensorData)){
        max = holder;
      }
      holder = (holder->next);
    }
    removeElAziziVersion(first, max);
    insertFirst(first, max);
    //free(max);
  }
}

int main(void) {
  /*struct LinkedList *list;
  //(*list).id = 1;
  //(list)->sensorData = 10;
  //(*list).next = NULL;
  struct LinkedList initial = *readSensor(1);
  //initial.id = 1;
  //initial.sensorData = 10;
  initial.next = NULL;

  (*list).id = initial.id;
  (list)->sensorData = initial.sensorData;
  (*list).next = initial.next;

  struct LinkedList elm = *readSensor(1);
  //elm.id = 1;
  //elm.sensorData = 100;
  struct LinkedList elme = *readSensor(0);
  //elme.id = 0;
  //elme.sensorData = 0;
  struct LinkedList elme2 = *readSensor(15);
  //elme2.id = 15;
  //elme2.sensorData = 314;

  insertFirst(&list, &elm);
  insertFirst(&list, &elme);
  insertFirst(&list, &elme2);
  printLL(&list);
  struct LinkedList elme3;
  elme3.id = 415;
  elme3.sensorData = 514;
  
  printf("isMember: %i \n", isMember(&list, &elme));
  printf("isMember: %i \n", isMember(&list, &elme3));

  removeElAziziVersion(&list, &initial);
  printLL(&list);
  //printf("First up: %lf \n", list->sensorData);*/

  struct LinkedList *list = readSensor(1);
  for(int i = 2; i <= 10; i++) {insertFirst(&list, readSensor(i));}
  printLL(&list);
  sorter(&list);
  printf("\n");
  printLL(&list);

  return 0;
}
