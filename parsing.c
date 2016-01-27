#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

long eval(mpc_ast_t *node);
long eval_op(char *op, long a, long b);
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
      "                                                            \
        number     : /-?[0-9]+(\\.[0-9]+)?/ ;                      \
        operator   : '+' | '-' | '*' | '/' | '%' ;                 \
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
      printf("Result: %li\n", eval(result.output));
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
  if (strcmp(op, "%") == 0) { return a % b; }
  return 0;
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
    int childMost = most_children(node->children[i]);

    if (childMost > most) {
      most = childMost;
    }
  }

  return most;
}
