#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "linkedlist.h"
#include <assert.h>
#include <string.h>
#include "talloc.h"
#include "value.h"

// skips comment
int commentCheck(char charNext) {
    while (charNext != '\n' && charNext != EOF) {
        charNext = (char)fgetc(stdin);
    }
    if (charNext == EOF) {
        return 1;
    } else {
        return 0;
    }
}

// checks to see if it is a symbol
int isSymbol(char charNext) {
  char symbols[] = {'!' , '$' , '%' , '&' , '*' , '/' , ':' , '<' , '=' , '>' , '?' , '~' , '_', '^'};
  int size = 14;

  for (int i = 0 ; i < size ; i ++) {
    if (charNext == symbols[i]) {
      return 1;
    }
  }

  return 0;
}

// boolean
Value *boolHelp() {
  char charNext = (char)fgetc(stdin);
  Value *newToken = talloc(sizeof(Value));
  newToken->type = BOOL_TYPE;

  int *Token = talloc(sizeof(int) * 2);
  if (charNext == 't') {
    Token [0] = 1;
    Token [1] = '\0';
    newToken->i = *Token;
  } else if (charNext == 'f') {
    Token [0] = 0;
    Token [1] = '\0';
    newToken->i = *Token;
  } else {
    printf("Syntax error: Boolean untokenizeable \n");
    texit(0);
  }

  return newToken;
}

// string
Value *stringHelp() {
  char charNext = (char)fgetc(stdin);
  char *string = talloc(301);
  int i = 1;
  string [0] = '"';

  while (charNext != '"') {
    string[i] = charNext;
    i++;
    charNext = (char)fgetc(stdin);
  }

  string [i] = '"';
  string[i+1] = '\0';
  Value *newToken = talloc(sizeof(Value));
  newToken->type = STR_TYPE;
  newToken->s = string;

  return newToken;
}

// symbol help
Value *symbolHelp(char charNext) {
  char *symbol = talloc(301);
  int i = 0;

  while (charNext != EOF && charNext != ' ' && charNext != '\n' && charNext != '(' && charNext != ')') {
    symbol[i] = charNext;
    i++;
    charNext = (char)fgetc(stdin);
  }

  symbol[i] = '\0';
  Value *newToken = talloc(sizeof(Value));
  newToken->type = SYMBOL_TYPE;
  newToken->s = symbol;

  fseek(stdin, -1L, SEEK_CUR);
  return newToken;
}

// int / double
Value *numHelp(char charNext) {
    char *number = talloc(301);
    int isDouble = 0;
    int i = 0;

    if (charNext == '+' || charNext == '-') {
      number[0] = charNext;
      i++;
      charNext = (char)fgetc(stdin);

      if (!isdigit(charNext)) {
        charNext = fseek(stdin, -1L, SEEK_CUR);
        charNext = fseek(stdin, -1L, SEEK_CUR);
        charNext = (char)fgetc(stdin);
        return symbolHelp(charNext);
      }
    }

    while (isdigit(charNext) || charNext == '.') {
        if (charNext == '.') {
            isDouble = 1;
        }
        number[i] = charNext;
        i++;
        charNext = (char)fgetc(stdin);
    }

    number[i] = '\0';
    char *pointer;
    Value *newToken = talloc(sizeof(Value));
    if (isDouble) {
        newToken->type = DOUBLE_TYPE;
        newToken->d = strtod(number, &pointer);
    } else {
        newToken->type = INT_TYPE;
        newToken->i = strtod(number, &pointer);
    }

    fseek(stdin, -1L, SEEK_CUR);
    return newToken;
}

// open
Value *openHelp() {
  Value *newToken = talloc(sizeof(Value));
  newToken->type = OPEN_TYPE;
  
  char *Open = talloc(sizeof(char) * 2);
  Open [0] = '(';
  Open [1] = '\0';

  newToken->s = Open;

  return newToken;
}

// close
Value *closeHelp() {
  Value *newToken = talloc(sizeof(Value));
  newToken->type = CLOSE_TYPE;
  
  char *Close = talloc(sizeof(char) * 2);
  Close [0] = ')';
  Close [1] = '\0';

  newToken->s = Close;

  return newToken;
}

// Read all of the input from stdin, and return a linked list consisting of the
// tokens.
Value *tokenize() {
    char charRead = (char)fgetc(stdin);
    Value *list = makeNull();

    while (charRead != EOF) {
      if (charRead == ';') {
          commentCheck(charRead);
      } else if (charRead == ' ' || charRead == '\n') {     // space / empty line check
      } else if (isdigit(charRead) || charRead == '.' || 
                  charRead == '+' || charRead == '-') {     // int / double
          list = cons(numHelp(charRead), list);
      } else if (charRead == '"') {                         // string
          list = cons(stringHelp(), list);
      } else if (charRead == '(') {                         // open
          list = cons(openHelp(), list);
      } else if (charRead == ')') {                         // close
          list = cons(closeHelp(), list);
      } else if (charRead == '#') {                         // boolean
          list = cons(boolHelp(), list);
      } else if (isalpha(charRead) || isSymbol(charRead)) {  // symbol
        list = cons(symbolHelp(charRead), list);
      } else {
        printf("Syntax error");
        break;
      }
      charRead = (char)fgetc(stdin);
    }
    
    Value *revList = reverse(list);
    return revList;
}

// Displays the contents of the linked list as tokens, with type information
void displayTokens(Value *list) {
  while (!isNull(list) && !isNull(car(list))) {
    switch (car(list)->type) {
      case BOOL_TYPE:
          if (car(list)->i == 0) {
              printf("#f:boolean\n"); 
            } else {
              printf("#t:boolean\n");
            }
            break;
      case INT_TYPE:
          printf("%i:integer\n", car(list)->i);
          break;
      case DOUBLE_TYPE:
          printf("%lf:double\n", car(list)->d);
          break;
      case STR_TYPE:
          printf("%s:string\n", car(list)->s);
          break;
      case SYMBOL_TYPE:
          printf("%s:symbol\n", car(list)->s);
          break;
      case OPEN_TYPE:
          printf("%s:open\n", car(list)->s);
          break;
      case CLOSE_TYPE:
          printf("%s:close\n", car(list)->s);
          break;
      case CONS_TYPE:
          break;
      case NULL_TYPE:
          break;
      case PTR_TYPE:
          break;
      case OPENBRACKET_TYPE:
        printf("%s:openbracket\n", car(list)->s);
        break; 
      case CLOSEBRACKET_TYPE:
        printf("%s:closebracket\n", car(list)->s);
        break; 
      case DOT_TYPE:
        printf("%s:dot\n", car(list)->s);
        break; 
      case SINGLEQUOTE_TYPE:
        printf("%s:singlequote\n", car(list)->s);
        break; 
      case VOID_TYPE:
        break;
      case CLOSURE_TYPE:
        printf("%s:closure\n", car(list)->s);
        break;
      case PRIMITIVE_TYPE:
        printf("%s:primitive\n", car(list)->s);
        break;
    }
    list = cdr(list);
  }
}