#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "linkedlist.h"
#include <assert.h>
#include <string.h>
#include "talloc.h"
#include "value.h"

typedef struct Value Value;

Value *activeList;  //global variable

// Replacement for malloc that stores the pointers allocated. It should store
// the pointers in some kind of list; a linked list would do fine, but insert
// here whatever code you'll need to do so; don't call functions in the
// pre-existing linkedlist.h. Otherwise you'll end up with circular
// dependencies, since you're going to modify the linked list to use talloc.

void *talloc(size_t size) {
  if (activeList == NULL) {
      Value *null = malloc(sizeof(Value));
      null->type = NULL_TYPE;
      activeList = null;
  }

  Value *newPointer = malloc(sizeof(Value)); 
  newPointer->type = PTR_TYPE;
  void *newVal = malloc(size);
  newPointer->p = newVal;

  Value *newCons = malloc(sizeof(Value));
  newCons->type = CONS_TYPE;

  newCons->c.car = newPointer;
  newCons->c.cdr = activeList;

  activeList = newCons;

  return newVal;
}

// Free all pointers allocated by talloc, as well as whatever memory you
// allocated in lists to hold those pointers.

//clear the pointer's p value first. 
// Make temp variable for active list;
//similar to clean up function in previous lab.

void tfree() {
  
  Value *temp = activeList;

  while (activeList != NULL && !isNull(activeList)) {
    if (activeList->c.car != NULL && activeList->c.car->type == PTR_TYPE) {
      free(activeList->c.car->p);
    }
    free(activeList->c.car);
    temp = activeList;
    activeList = activeList->c.cdr;
    free(temp);
  }

  free(activeList);
  activeList = NULL;
}

// Replacement for the C function "exit", that consists of two lines: it calls
// tfree before calling exit. It's useful to have later on; if an error happens,
// you can exit your program, and all memory is automatically cleaned up.

void texit(int status) {
  tfree();
  exit(status);
}