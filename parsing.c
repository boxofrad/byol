#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

typedef enum {
  LVAL_TYPE_NUMBER,
  LVAL_TYPE_ERROR
} lval_type_t;

typedef enum {
  LVAL_ERR_DIV_ZERO,
  LVAL_ERR_BAD_OP,
  LVAL_ERR_BAD_NUMBER
} lval_err_t;

typedef struct {
  lval_type_t type;
  long        number;
  lval_err_t  err;
} lval_t;

lval_t lval_err(lval_err_t err);
lval_t lval_number(long number);
char *lval_describe(lval_t lval);

lval_t eval(mpc_ast_t *node);
lval_t eval_op(char *op, lval_t a, lval_t b);

int num_leaves(mpc_ast_t *node);
int num_branches(mpc_ast_t *node);
int most_children(mpc_ast_t *node);

int main(int argc, char **argv) {
  puts("BYOL Version 0.0.1");
  puts("Press CTRL-C to Exit\n");

  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expression = mpc_new("expression");
  mpc_parser_t *Program = mpc_new("program");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                                      \
        number     : /-?[0-9]+(\\.[0-9]+)?/ ;                                \
        operator   : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\" ; \
        expression : <number> | '(' <operator> <expression>+ ')' ;           \
        program    : /^/ <operator> <expression>+ /$/ ;                      \
      ",
      Number, Operator, Expression, Program
  );

  while (1) {
    char *input = readline("byol> ");
    add_history(input);

    mpc_result_t result;
    if (mpc_parse("<stdin>", input, Program, &result)) {
      lval_t computedResult = eval(result.output);

      char *resultDescription = lval_describe(computedResult);
      printf("Result: %s\n", resultDescription);
      free(resultDescription);

      printf("Leaves: %d, Branches: %d, Most Children: %d\n",
          num_leaves(result.output),
          num_branches(result.output),
          most_children(result.output));
      mpc_ast_print(result.output);
      mpc_ast_delete(result.output);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expression, Program);

  return 0;
}

lval_t eval(mpc_ast_t *node) {
  // Node is a number and cannot be reduced further
  if(strstr(node->tag, "number")) {
    errno = 0;
    long number = strtol(node->contents, NULL, 10);
    return errno == ERANGE
      ? lval_err(LVAL_ERR_BAD_NUMBER)
      : lval_number(number);
  }

  // Operator is the second child (after the opening paren)
  char *op = node->children[1]->contents;

  // Evaluate the first argument
  lval_t val = eval(node->children[2]);

  // Apply the operator to the remaining arguments
  int i = 3;
  while (strstr(node->children[i]->tag, "expr")) {
    val = eval_op(op, val, eval(node->children[i]));
    i++;
  }

  // Minus operator with only one operand should negate the number
  if (strcmp(op, "-") == 0 && i == 3) {
    val = lval_number(0 - val.number);
  }

  return val;
}

lval_t eval_op(char *op, lval_t a, lval_t b) {
  if (a.type == LVAL_TYPE_ERROR) { return a; }
  if (b.type == LVAL_TYPE_ERROR) { return b; }

  if (strcmp(op, "+") == 0) {
    return lval_number(a.number + b.number);
  }

  if (strcmp(op, "-") == 0) {
    return lval_number(a.number - b.number);
  }

  if (strcmp(op, "*") == 0) {
    return lval_number(a.number * b.number);
  }

  if (strcmp(op, "%") == 0) {
    return lval_number(a.number % b.number);
  }

  if (strcmp(op, "^") == 0) {
    return lval_number(pow(a.number, b.number));
  }

  if (strcmp(op, "min") == 0) {
    return lval_number(MIN(a.number, b.number));
  }

  if (strcmp(op, "max") == 0) {
    return lval_number(MAX(a.number, b.number));
  }

  if (strcmp(op, "/") == 0) {
    // Prevent divide by zero
    return b.number == 0
      ? lval_err(LVAL_ERR_DIV_ZERO)
      : lval_number(a.number / b.number);
  }

  return lval_err(LVAL_ERR_BAD_OP);
}

int num_leaves(mpc_ast_t *node) {
  if (node->children_num == 0) {
    return 1;
  }

  int num = 0;
  for (int i = 0; i < node->children_num; i++) {
    num += num_leaves(node->children[i]);
  }

  return num;
}

int num_branches(mpc_ast_t *node) {
  if (node->children_num == 0) {
    return 0;
  }

  int num = 1;
  for (int i = 0; i < node->children_num; i++) {
    num += num_branches(node->children[i]);
  }

  return num;
}

int most_children(mpc_ast_t *node) {
  if (node->children_num == 0) {
    return 0;
  }

  int most = node->children_num;

  for (int i = 0; i < node->children_num; i++) {
    most = MAX(most, most_children(node->children[i]));
  }

  return most;
}

lval_t lval_number(long number) {
  lval_t lval;
  lval.type = LVAL_TYPE_NUMBER;
  lval.number = number;
  return lval;
}

lval_t lval_err(lval_err_t err) {
  lval_t lval;
  lval.type = LVAL_TYPE_ERROR;
  lval.err = err;
  return lval;
}

char *lval_describe(lval_t lval) {
  char *description = malloc(sizeof(char) * 100);

  if (lval.type == LVAL_TYPE_NUMBER) {
    sprintf(description, "%li", lval.number);
  } else {
    if (lval.err == LVAL_ERR_DIV_ZERO) {
      strcpy(description, "Error: Division By Zero!");
    }

    if (lval.err == LVAL_ERR_BAD_OP) {
      strcpy(description, "Error: Invalid Operation!");
    }

    if (lval.err == LVAL_ERR_BAD_NUMBER) {
      strcpy(description, "Error: Invalid Number!");
    }
  }

  return description;
}
