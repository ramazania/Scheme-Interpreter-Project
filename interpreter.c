#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "parser.h"
#include "value.h"
#include "linkedlist.h"
#include "tokenizer.h"
#include "talloc.h"
#include "interpreter.h"

void printHelp(Value *hello) {
  if (hello->type == INT_TYPE) {
    printf("%i", hello->i);
  } else if (hello->type == DOUBLE_TYPE) {
    printf("%lf", hello->d);
  } else if (hello->type == BOOL_TYPE) {
    if (hello->i == 1) {
      printf("#t");
    } else {
      printf("#f");
    }
  } else if (hello->type == STR_TYPE) {
    printf("%s", hello->s);
  } else if (hello->type == SYMBOL_TYPE) {
    printf("%s", hello->s);
  } else if (hello->type == NULL_TYPE) {
    printf("()");
  } else if (hello->type == VOID_TYPE) {
  } else if (hello->type == CLOSURE_TYPE) {
    printf("#<procedure>");
  }
}

void print(Value *tree) {
  if (tree->type != CONS_TYPE) {
    printHelp(tree);
  } else {
    printf("(");
      while (tree->type != NULL_TYPE) {
        if (tree->type == CONS_TYPE && car(tree)->type == CONS_TYPE) {
          print(car(tree));
        } else {
          printHelp(car(tree));
          if (cdr(tree)->type != NULL_TYPE && cdr(tree)->type != CONS_TYPE) {
            printf(" . ");
            printHelp(cdr(tree));
            break;
          }
        }
        if (cdr(tree)->type != NULL_TYPE) {
          printf(" ");
        }
        tree = cdr(tree);
      }
      printf(")");
  }
}

void bind(char *name, Value *(*function)(Value *), Frame *frame) {
  Value *newBinding = makeNull();
  newBinding->type = PRIMITIVE_TYPE;
  newBinding->pf = function;

  Value *binding = makeNull();
  binding->type = SYMBOL_TYPE;
  binding->s = name;

  frame->bindings = cons(cons(binding, newBinding), frame->bindings);
}

int checkDuplicates(Value *param, Value *paramList) {
  while (paramList->type != NULL_TYPE) {
    Value *curr = car(paramList);
    if (curr->type == CONS_TYPE) {
      curr = car(curr);
    }
    if (curr->type == SYMBOL_TYPE && !strcmp(param->s, curr->s)) {
      return 1;
    }
    paramList = cdr(paramList);
  }
  return 0;
}

void evaluationError(char *error) {
  printf("Evaluation error: %s\n", error);
  texit(1);
}

Value *lookUpSymbol(Value *symbol, Frame *frame) {
  while (frame != NULL) {
    Value *bindings = frame->bindings;
    while (bindings->type != NULL_TYPE) {
      Value *curr = car(bindings);
      if (!strcmp(car(curr)->s, symbol->s)) {
        return cdr(curr);
      }
      bindings = cdr(bindings);
    }
    frame = frame->parent;
  }
  evaluationError("lookUpSymbol");
  return NULL;
}

Value *evalIf(Value *args, Frame *frame) {
  if (length(args) != 3) {
    evaluationError("evalIf: args != 3");
  }
  
  Value *boolean = eval(car(args), frame);
  if (boolean->type != BOOL_TYPE) {
    evaluationError("evalIf: first arg is not of BOOL_TYPE");
  } else {
    if (boolean->i == 1) {
      return eval(car(cdr(args)), frame);
    } else {
      return eval(car(cdr(cdr(args))), frame);
    }
  }

  return NULL;
}


Value *andOrHelper(Value *tree, int andOr, Frame *frame) {
	Value *boolean = makeNull();
	while(tree->type != NULL_TYPE){
		boolean = eval(car(tree), frame);
        if (boolean->i == andOr) {
            return boolean;
        }
        tree = cdr(tree);
	}
    return boolean;
}

Value *evalAnd(Value *tree, Frame *frame) {
    if (length(tree) < 1) {
        evaluationError("too few arguments in and");
    }
    return andOrHelper(tree, 0, frame);
}

Value *evalOr(Value *tree, Frame *frame) {
	if (length(tree) < 1) {
        evaluationError("too few arguments in or");
    }
	return andOrHelper(tree, 1, frame);
}


Value *evalLet(Value *tree, Frame *frame) {
  if (length(tree) < 2) {
    evaluationError("evalLet: not enough arguments");
  }

  Value *bindings;
  if (tree->type == CONS_TYPE) {
    bindings = car(tree);
    if (bindings->type != CONS_TYPE && bindings->type != NULL_TYPE) {
      evaluationError("evalLet: invalid arguments");
    }
  } else if (tree->type == NULL_TYPE) {
    bindings = makeNull();
  } else {
    evaluationError("evalLet: invalid let");
  }

  Frame *newFrame = talloc(sizeof(Frame));
  newFrame->parent = frame;
  newFrame->bindings = makeNull();

  while (bindings->type != NULL_TYPE) {
    Value *binding = car(bindings);
    if (binding->type != CONS_TYPE) {
      evaluationError("evalLet: invalid let");
    } else if (length(binding) < 2) {
      evaluationError("evalLet: not enough arguments in the binding");
    }
    Value *var = car(binding);
    if (var->type != SYMBOL_TYPE) {
      evaluationError("evalLet: wrong variable type");
    }
    Value *expression = eval(car(cdr(binding)), frame);
    Value *newBinding = cons(var, expression);
    newFrame->bindings = cons(newBinding, newFrame->bindings);

    bindings = cdr(bindings);
  }

  while (cdr(cdr(tree))->type != NULL_TYPE) {
    if (cdr(tree)->type == NULL_TYPE) {
      evaluationError("evalError: empty body");
    }
    eval(car(cdr(tree)), newFrame);
    tree = cdr(tree);
  }
  return eval(car(cdr(tree)), newFrame);
}

Value *letStar(Value *tree, Frame *frame) {
  Value *bindings;
  if (tree->type == CONS_TYPE) {
    bindings = car(tree);
  }

  while (bindings->type != NULL_TYPE) {
    Frame *newFrame = talloc(sizeof(Frame));
    newFrame->parent = frame;
    newFrame->bindings = makeNull();

    Value *binding = car(bindings);
    if (binding->type != CONS_TYPE) {
      evaluationError("evalLet: invalid let");
    } else if (length(binding) < 2) {
      evaluationError("evalLet: not enough arguments in the binding");
    }
    Value *var = car(binding);
    if (var->type != SYMBOL_TYPE) {
      evaluationError("evalLet: wrong variable type");
    }
    Value *expression = eval(car(cdr(binding)), frame);
    Value *newBinding = cons(var, expression);
    newFrame->bindings = cons(newBinding, newFrame->bindings);

    bindings = cdr(bindings);
    frame = newFrame;
  }

  while (cdr(cdr(tree))->type != NULL_TYPE) {
    if (cdr(tree)->type == NULL_TYPE) {
      evaluationError("evalError: empty body");
    }
    eval(car(cdr(tree)), frame);
    tree = cdr(tree);
  }
  return eval(car(cdr(tree)), frame);
}

Value *evalLetBody(Value *letBody, Frame *letFrame) {
    if (letBody->type == NULL_TYPE) {
        evaluationError("no body in let");
        return NULL;
    } else {
        while (cdr(letBody)->type != NULL_TYPE) {
            eval(car(letBody), letFrame);
            letBody = cdr(letBody);
        }
        return eval(car(letBody), letFrame);
    }
}

Value *letrec(Value *args, Frame *frame) {
    Value *bindingList = makeNull();
    Value *letBody = makeNull();
    if (args->type == CONS_TYPE) {
        bindingList = car(args);
        letBody = cdr(args);
    }
    if (bindingList->type != CONS_TYPE && bindingList->type != NULL_TYPE) {
        evaluationError("improper variable binding format in letrec");
    }
    
	Frame *letFrame = talloc(sizeof(Frame));
    letFrame->parent = frame;
    letFrame->bindings = makeNull();

	Value *variableList = makeNull();
	Value *expressionList = makeNull();
	while(bindingList->type != NULL_TYPE){
		Value *binding = car(bindingList);
		if (binding->type != CONS_TYPE) {
        	evaluationError("improper variable binding format in letrec");
    	} else if (length(binding) != 2) {
        	evaluationError("wrong number of items in letrec binding");
		}
		Value *variable = car(binding);
		if (variable->type != SYMBOL_TYPE) {
			evaluationError("improper variable type for binding in letrec");
		}
		Value *temp1 = cons(variable, variableList);
		variableList = temp1;
		Value *expression = eval(car(cdr(binding)), letFrame);
		Value *temp2 = cons(expression, expressionList);
		expressionList = temp2;
		bindingList = cdr(bindingList);
	}

	while(variableList->type != NULL_TYPE){
		Value *newBinding = cons(car(variableList), car(expressionList));
    Value *temp = cons(newBinding, letFrame->bindings);
    letFrame->bindings = temp;
		variableList = cdr(variableList);
		expressionList = cdr(expressionList);
	}
	return evalLetBody(letBody, letFrame);
}

Value *evalCond(Value *args, Frame *frame) {
    if (length(args) == 0) {
        evaluationError("no arguments in cond");
    }
    while (args->type != NULL_TYPE) {
        Value *condition = car(car(args));
        if (condition->type == SYMBOL_TYPE && 
            !strcmp(condition->s, "else")) {
            if (cdr(args)->type != NULL_TYPE) {
                evaluationError("else is not last test in cond");
            } 
            return eval(car(cdr(car(args))), frame);
        }
        condition = eval(condition, frame);
        if (condition->type != BOOL_TYPE) {
            evaluationError("non-boolean condition for if");
        } else if (condition->i) {
            return eval(car(cdr(car(args))), frame);
        }
        args = cdr(args);
    }
    Value *voidVal = makeNull();
    voidVal->type = VOID_TYPE;
    return voidVal;
}

Value *evalQuote(Value *tree) {
    if (tree->type == NULL_TYPE) {
      evaluationError("evalQuote: more than 1 argument");
    } else if (length(tree) != 1) {
      evaluationError("evalQuote: more than 1 argument");
    }
    return car(tree);
}

Value *evalDefine(Value *tree, Frame *frame) {
  if (tree->type == NULL_TYPE) {
    evaluationError("evalDefine: empty arguments");
  } else if (car(tree)->type != SYMBOL_TYPE) {
    evaluationError("evalDefine: not a symbol");
  } else if (cdr(tree)->type == NULL_TYPE || car(cdr(tree))->type == NULL_TYPE) {
    evaluationError("evalDefine: empty body");
  }
  Value *binding = cons(car(tree), eval(car(cdr(tree)), frame));
  frame->bindings = cons(binding, frame->bindings);

  Value *temp = talloc(sizeof(Value));
  temp->type = VOID_TYPE;
  return temp;
}

Value *evalLambda(Value *tree, Frame *frame) {
  if (tree->type == NULL_TYPE) {
    evaluationError("evalLambda: nothing after lambda");
  } else if (length(tree) != 2) {
    evaluationError("evalLambda: either no params or body");
  } else if (car(tree)->type == CONS_TYPE && car(car(tree))->type != SYMBOL_TYPE) {
    evaluationError("evalLambda: params must be symbols");
  }

  Value *closure = makeNull();
  closure->type = CLOSURE_TYPE;
  Value *param = car(tree);
  Value *paramList = makeNull();
  while (param->type != NULL_TYPE) {
    if (checkDuplicates(car(param), paramList)) {
      evaluationError("evalLambda: multiple parameters of same name");
    }
    paramList = cons(car(param), paramList);
    param = cdr(param);
  }
  closure->cl.paramNames = paramList;
  closure->cl.functionCode = car(cdr(tree));
  closure->cl.frame = frame;

  return closure;
}

int setBinding(Value *variable, Value *newVal, Frame *frame){
    int isBound = 0;
    do {
        Value *bindings = frame->bindings;
        while(bindings->type != NULL_TYPE){
            Value *binding = car(bindings);
            if (!strcmp(car(binding)->s, variable->s)) {
                isBound = 1;
                binding->c.cdr = newVal;
                break;
            }
            bindings = cdr(bindings);
        }
        if (frame->parent != NULL) {
            frame = frame->parent;
        } else {
            break;
        }  
    } while (frame->parent != NULL);
    return isBound;
}

Value *set(Value *args, Frame *frame){
	 if (args->type == NULL_TYPE) {
        evaluationError("no arguments passed to set!");
    } else if (cdr(args)->type == NULL_TYPE) {
        evaluationError("no value to bind variable to in set!");
    } else if (car(args)->type != SYMBOL_TYPE) {
        evaluationError("non-symbol cannot be bound to a value in set!");
    } 
	Value *variable = car(args);
	Value *expression = eval(car(cdr(args)),frame);
    int varWasSet = setBinding(variable, expression, frame);
    if (!varWasSet) {
        evaluationError("no binding to modify in set!");
    } 
    Value *voidVal = makeNull();
    voidVal->type = VOID_TYPE;
    return voidVal;
}


Value *begin(Value *tree, Frame *frame) {
  while (tree->type != NULL_TYPE) {
        Value *evaluation = eval(car(tree), frame);
        if (cdr(tree)->type == NULL_TYPE) {
            return evaluation;
        }
		tree = cdr(tree);
    }
    Value *voidVal = makeNull();
    voidVal->type = VOID_TYPE;
    return voidVal;
}


Value *apply(Value *function, Value *args) {
  if (function->type == CLOSURE_TYPE) {
    Value *bindings = makeNull();
    Value *names = function->cl.paramNames;
    Value *body = function->cl.functionCode;
    while (args->type != NULL_TYPE && names->type != NULL_TYPE) {
      Value *binding = cons(car(names), car(args));
      bindings = cons(binding, bindings);
      names = cdr(names);
      args = cdr(args);
    }
    if (args->type != NULL_TYPE || names->type != NULL_TYPE) {
      evaluationError("apply: invalid");
    }

    Frame *newFrame = talloc(sizeof(Frame));
    newFrame->bindings = bindings;
    newFrame->parent = function->cl.frame;

    return eval(body, newFrame);

  } else if (function->type == PRIMITIVE_TYPE) {
    return (function->pf)(args);
  } else {
    evaluationError("apply: invalid function");
  }
  return NULL;
}

Value *sumHelp(Value *tree) {
  Value *sum = makeNull();
  double value = 0;
  int isDouble = 0;

  while (tree->type != NULL_TYPE) {
    if (car(tree)->type == DOUBLE_TYPE) {
      isDouble = 1;
      value = value + car(tree)->d;
    } else if (car(tree)->type == INT_TYPE) {
      value = value + car(tree)->i;
    } else {
      evaluationError("sumHelp: not a number");
    }
    tree = cdr(tree);
  }

  if (isDouble) {
    sum->type = DOUBLE_TYPE;
    sum->d = value;
    return sum;
  } else {
    sum->type = INT_TYPE;
    sum->i = (int) value;
    return sum;
  }
}

Value *moduloHelp(Value *args) {
    if (length(args) != 2) {
        evaluationError("wrong number of args for modulo");
    } else if (car(args)->type != INT_TYPE ||
        car(cdr(args))->type != INT_TYPE) {
        evaluationError("wrong argument type in modulo");
    } 
    Value *result = makeNull();
    result->type = INT_TYPE;
    result->i = car(cdr(args))->i % car(args)->i;
	return result;
}

Value *multiplyHelp(Value *tree) {
  Value *result = makeNull();
  double value = 1;
  int isDouble = 0;

  while (tree->type != NULL_TYPE) {
    if (car(tree)->type == DOUBLE_TYPE) {
      isDouble = 1;
      value =  value * car(tree)->d ;
    } else if (car(tree)->type == INT_TYPE) {
      value = value * car(tree)->i;    
    } else {
      evaluationError("multiplyHelp: not a number");
    }
    tree = cdr(tree);
  }

  if (isDouble) {
    result->type = DOUBLE_TYPE;
    result->d = value;
    return result;
  } else {
    result->type = INT_TYPE;
    result->i = (int) value;
    return result;
  }
}

Value *divideHelp(Value *tree) {
  Value *result = makeNull();
  double value = 1;
  int isDouble = 0;

  while (tree->type != NULL_TYPE) {
    int temp = (int) value;
    if (car(tree)->type == DOUBLE_TYPE) {
      isDouble = 1;
      value =  car(tree)->d / value;
    } else if (car(tree)->type == INT_TYPE && car(tree)->i % temp != 0) {
      isDouble = 1;
      value =  car(tree)->i / value;
    } else if (car(tree)->type == INT_TYPE) {
      value = car(tree)->i / value;
    } else {
      evaluationError("divideHelp: not a number");
    }
    tree = cdr(tree);
  }

  if (isDouble) {
    result->type = DOUBLE_TYPE;
    result->d = value;
    return result;
  } else {
    result->type = INT_TYPE;
    result->i = (int) value;
    return result;
  }
}


double doubleHelp(Value *value) {
  double result = 0;
  if (value->type == DOUBLE_TYPE) {
    result = value->d;
  } else if (value->type == INT_TYPE) {
    result = value->i;
  } else {
    evaluationError("doubleHelp: not a number type");
  }

  return result;
}

Value *subtractHelp(Value *tree) {
  Value *result = makeNull();
  double value = 0;
  int isDouble = 0;

  while (tree->type != NULL_TYPE) {
    if (car(tree)->type == DOUBLE_TYPE) {
      isDouble = 1;
      value = car(tree)->d - value;
    } else if (car(tree)->type == INT_TYPE) {
      value = car(tree)->i - value;
    } else {
      evaluationError("subtractHelp: not a number");
    }
    tree = cdr(tree);
  }

  if (isDouble) {
    result->type = DOUBLE_TYPE;
    result->d = value;
    return result;
  } else {
    result->type = INT_TYPE;
    result->i = (int) value;
    return result;
  }
}

Value *lessHelp(Value *tree) { 
  Value *result = makeNull();
  result->type = BOOL_TYPE;
  
  double first = doubleHelp(car(tree));
  double second = doubleHelp(car(cdr(tree)));

  if (first > second) {
    result->i = 1;
  } else {
    result->i = 0;
  }

  return result;
}

Value *greaterHelp(Value *tree) {
  Value *result = makeNull();
  result->type = BOOL_TYPE;
  
  double first = doubleHelp(car(tree));
  double second = doubleHelp(car(cdr(tree)));

  if (first < second) {
    result->i = 1;
  } else {
    result->i = 0;
  }

  return result;
}

Value *equalHelp(Value *tree) {
  Value *result = makeNull();
  result->type = BOOL_TYPE;
  
  double first = doubleHelp(car(tree));
  double second = doubleHelp(car(cdr(tree)));

  if (first == second) {
    result->i = 1;
  } else {
    result->i = 0;
  }

  return result;
}

Value *nullHelp(Value *arg) {
  if (length(arg) != 1) {
    evaluationError("nullHelp: invalid length");
  }
  Value *boolean = makeNull();
  boolean->type = BOOL_TYPE;
  
  if (car(arg)->type == NULL_TYPE) {
    boolean->i = 1;
  } else {
    boolean->i = 0;
  }
  return boolean;
}

Value *carHelp(Value *arg) {
  if (length(arg) != 1) {
    evaluationError("carHelp: invalid length");
  } else if (car(arg)->type != CONS_TYPE) {
    evaluationError("carHelp: not of CONS type");
  }
  return car(car(arg));
}

Value *cdrHelp(Value *arg) {
  if (length(arg) != 1) {
    evaluationError("cdrHelp: invalid length");
  } else if (car(arg)->type != CONS_TYPE) {
    evaluationError("carHelp: not of CONS type");
  }
  return cdr(car(arg));
}

Value *consHelp(Value *tree) {
  if (length(tree) != 2) {
    evaluationError("consHelp: invalid length");
  }
  return cons(car(cdr(tree)), car(tree));
}

Value *eval(Value *tree, Frame *frame) {
  if (tree == NULL) {
    evaluationError("eval: NULL tree");
  }
  switch (tree->type) {
    case INT_TYPE: {
      return tree;
    }
    case DOUBLE_TYPE: {
      return tree;
    }
    case STR_TYPE: {
      return tree;
    }
    case BOOL_TYPE: {
      return tree;
    }
    case SYMBOL_TYPE: {
      return lookUpSymbol(tree, frame);
    }
    case NULL_TYPE: {
      return tree;
    }
    case CONS_TYPE: {
      Value *result = makeNull();
      Value *first = car(tree);
      Value *args = cdr(tree);

      if (!strcmp(first->s, "if")) {
          result = evalIf(args, frame);
      } else if (!strcmp(first->s, "let")) {
        if (car(cdr(tree))->type == NULL_TYPE) {
          return eval(car(cdr(cdr(tree))), frame);
        }
          result = evalLet(args, frame);
      } else if (!strcmp(first->s, "quote")) {
          result = evalQuote(args);
      } else if (!strcmp(first->s, "define")) {
          result = evalDefine(args, frame);
      } else if (!strcmp(first->s, "lambda")) {
          result = evalLambda(args, frame);
      } else if (!strcmp(first->s, "let*")) {
          result = letStar(args, frame);
      } else if (!strcmp(first->s, "letrec")) {
          result = letrec(args, frame);
      } else if (!strcmp(first->s, "cond")) {
          result = evalCond(args, frame);
      } else if (!strcmp(first->s, "set!")) {
          result = set(args, frame);
      } else if (!strcmp(first->s, "begin")) {
          result = begin(args, frame);
      } else if (!strcmp(first->s, "and")) {
          result = evalAnd(args, frame);
      } else if (!strcmp(first->s, "or")) {
          result = evalOr(args, frame);
      } else {
        Value *newArgs = makeNull();
        while (args->type != NULL_TYPE) {
          newArgs = cons(eval(car(args), frame), newArgs);
          args = cdr(args);
        }
        first = eval(first, frame);
        result = apply(first, newArgs);
      }

      return result;
    }
    default:
        evaluationError("eval: default error");
  }
  
  return NULL;
}

void interpret(Value *tree) {
  Frame *first = talloc(sizeof(Frame));
  first->bindings = makeNull();
  first->parent = NULL;
  while (tree->type != NULL_TYPE) {
    bind("car", carHelp, first);
    bind("cdr", cdrHelp, first);
    bind("cons", consHelp, first);
    bind("null?", nullHelp, first);
    bind("modulo", moduloHelp, first);
    bind("*", multiplyHelp, first);
    bind("/", divideHelp, first);
    bind("+", sumHelp, first);
    bind("-", subtractHelp, first);
    bind("<", lessHelp, first);
    bind(">", greaterHelp, first);
    bind("=", equalHelp, first);
    print(eval(car(tree), first));
    tree = cdr(tree);
    printf("\n");
  }
}