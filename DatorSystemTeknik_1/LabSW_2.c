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

void printList(struct LinkedList *first) {
  struct LinkedList *holder = first;
  while(holder != NULL){
    printf("Id: %i \t sensorData: %lf\n", holder->id, holder->sensorData);
    holder = (holder->next);
  }
}

int LLL(struct LinkedList **first) { // Linked-List length
  struct LinkedList *holder = *first;
  int count = 0;
  while(holder != NULL){
    count++;
    holder = (holder->next);
  }

  return count;
}

int isMember(struct LinkedList **first, struct LinkedList *el) {
  struct LinkedList *holder = *first;
  while(holder != NULL){
    if((*el).id == (*holder).id 
    && (*el).sensorData == (*holder).sensorData) {return 1;}
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

void sorter(struct LinkedList **first, int len) {
  for(int i = 0; i < len; i++) {
    struct LinkedList *holder = *first;
    for(int j = 0; j < i; j++) { holder = holder->next; }
    struct LinkedList *max = holder;
    while(holder != NULL) {
      if((max->sensorData) < (holder->sensorData)){max = holder;}
      holder = (holder->next);
    }
    removeElAziziVersion(first, max);
    insertFirst(first, max);
    //free(max);
  }
}

void freeLL(struct LinkedList **first) {
  struct LinkedList *holder = *first;
  struct LinkedList *holderHolder;
  while(holder != NULL){
    holderHolder = holder;
    free(holder);
    holder = (holderHolder->next);
  }
}

int main(void) {
  /* Hello Yousra, this is the code for Gr 24. */
  /*
  struct LinkedList *list;
  struct LinkedList initial;

  initial.id         = 1;
  initial.sensorData = 10;
  initial.next       = NULL;

  list->id         = initial.id;
  list->sensorData = initial.sensorData;
  list->next       = initial.next;*/

  //freeLL(&list);
  
  return 0;
}
