#include <stdlib.h>
#include <stdio.h>
#include "parser.h"
#include "value.h"
#include "linkedlist.h"
#include "tokenizer.h"
#include "talloc.h"

Value *treeHelp(int *depth, Value *token, Value *tree) {
  if (token->type != CLOSE_TYPE) {
    if (token->type == OPEN_TYPE) {
      *depth = *depth + 1;
    }
    tree = cons(token, tree);
  } else {
    if (*depth < 1) {
      printf("Syntax error: Parentheses\n");
      texit(1);
    }
    Value *temp = makeNull();
    *depth = *depth - 1;
    while (car(tree)->type != OPEN_TYPE) {
      temp = cons(car(tree), temp);
      tree = cdr(tree);
    }
    tree->c.car = temp;
  }
  return tree;
}

// Takes a list of tokens from a Scheme program, and returns a pointer to a
// parse tree representing that program.
Value *parse(Value *tokens) {
  if (tokens == NULL) {
    printf("Null list");
    texit(1);
  }
  Value *tree = makeNull();
  Value *list = tokens;
  int depth = 0;

  while (list->type != NULL_TYPE) {
    tree = treeHelp(&depth, car(list), tree);
    list = cdr(list);
  }

  if (depth != 0) {
    printf("Syntax error: Parentheses\n");
    texit(1);
  }
  
  return reverse(tree);
}

void printTokenHelp(Value *tree) {
  if (tree->type == INT_TYPE) {
    printf("%i", tree->i);
  } else if (tree->type == DOUBLE_TYPE) {
    printf("%lf", tree->d);
  } else if (tree->type == STR_TYPE) {
    printf("%s", tree->s);
  } else if (tree->type == NULL_TYPE) {
    printf("()");
  } else {
    printf("%s", tree->s);
  }
}

void printTreeHelp(Value *tree) {
  if (tree->type != CONS_TYPE) {
    printTokenHelp(tree);
  } else {
    printf("(");
    while (tree->type != NULL_TYPE) {
      if (car(tree)->type != CONS_TYPE) {
        printTokenHelp(car(tree));
      } else {
        printTreeHelp(car(tree));
      }
      if (cdr(tree)->type != NULL_TYPE) {
        printf(")");
      }
      tree = cdr(tree);
    }
    printf(")");
  }
}

// Prints the tree to the screen in a readable fashion. It should look just like
// Scheme code; use parentheses to indicate subtrees.
void printTree(Value *tree) {
  while (tree->type != NULL_TYPE) {
    printTreeHelp(car(tree));
    tree = cdr(tree);
    printf(" ");
  }
}