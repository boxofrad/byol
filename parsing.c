#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

long eval(mpc_ast_t *node);
long eval_op(char *op, long a, long b);

int main(int argc, char **argv) {
  puts("BYOL Version 0.0.1");
  puts("Press CTRL-C to Exit\n");

  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator = mpc_new("operator");
  mpc_parser_t *Expression = mpc_new("expression");
  mpc_parser_t *Program = mpc_new("program");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                            \
        number     : /-?[0-9]+(\\.[0-9]+)?/ ;                      \
        operator   : '+' | '-' | '*' | '/' ;                       \
        expression : <number> | '(' <operator> <expression>+ ')' ; \
        program    : /^/ <operator> <expression>+ /$/ ;            \
      ",
      Number, Operator, Expression, Program
  );

  while (1) {
    char *input = readline("byol> ");
    add_history(input);

    mpc_result_t result;
    if (mpc_parse("<stdin>", input, Program, &result)) {
      long computedValue = eval(result.output);
      printf("%li\n", computedValue);
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

long eval(mpc_ast_t *node) {
  // Node is a number and cannot be reduced further
  if(strstr(node->tag, "number")) {
    return atoi(node->contents);
  }

  // Operator is the second child (after the opening paren)
  char *op = node->children[1]->contents;

  // Evaluate the first argument
  long val = eval(node->children[2]);

  // Apply the operator to the remaining arguments
  int i = 3;
  while (strstr(node->children[i]->tag, "expr")) {
    val = eval_op(op, val, eval(node->children[i]));
    i++;
  }

  return val;
}

long eval_op(char *op, long a, long b) {
  if (strcmp(op, "+") == 0) { return a + b; }
  if (strcmp(op, "-") == 0) { return a - b; }
  if (strcmp(op, "*") == 0) { return a * b; }
  if (strcmp(op, "/") == 0) { return a / b; }
  return 0;
}
