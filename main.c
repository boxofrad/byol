#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"
#include "lval.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

lval_t *ast_node_to_lval(mpc_ast_t *node);
int is_valid_expr(mpc_ast_t *node);

lval_t *lval_eval_sexpr(lval_t *val);
lval_t *lval_eval();
lval_t *builtin_op(lval_t *val, char *op);

void lval_print(lval_t *val);
void lval_expr_print(lval_t *val, char open, char close);

int num_leaves(mpc_ast_t *node);
int num_branches(mpc_ast_t *node);
int most_children(mpc_ast_t *node);

int main(int argc, char **argv) {
  puts("BYOL Version 0.0.1");
  puts("Press CTRL-C to Exit\n");

  mpc_parser_t *Number  = mpc_new("number");
  mpc_parser_t *Symbol  = mpc_new("symbol");
  mpc_parser_t *Sexpr   = mpc_new("sexpr");
  mpc_parser_t *Expr    = mpc_new("expr");
  mpc_parser_t *Program = mpc_new("program");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                                   \
        number  : /-?[0-9]+/ ;                                            \
        symbol  : '+' | '-' | '*' | '/' | '^' | \"min\" | \"max\" ;       \
        sexpr   : '(' <expr>* ')' ;                                       \
        expr    : <number> | <symbol> | <sexpr> ;                         \
        program : /^/ <expr>* /$/ ;                                       \
      ",
      Number, Symbol, Sexpr, Expr, Program
  );

  while (1) {
    char *input = readline("byol> ");
    add_history(input);

    mpc_result_t result;
    if (mpc_parse("<stdin>", input, Program, &result)) {
      lval_t *program = ast_node_to_lval(result.output);
      lval_t *computedResult = lval_eval(program);

      lval_print(computedResult);
      lval_del(computedResult);

      puts("\n\n=== Abstract Syntax Tree ===");

      printf(
          "Leaves: %d, Branches: %d, Most Children: %d\n",
          num_leaves(result.output),
          num_branches(result.output),
          most_children(result.output)
      );

      mpc_ast_print(result.output);
      mpc_ast_delete(result.output);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Program);

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
    most = MAX(most, most_children(node->children[i]));
  }

  return most;
}

lval_t *ast_node_to_lval(mpc_ast_t *node) {
  if (strstr(node->tag, "number")) {
    errno = 0;
    long num = strtol(node->contents, NULL, 10);
    return errno == ERANGE ? lval_err("Invalid Number") : lval_num(num);
  }

  if (strstr(node->tag, "symbol")) {
    return lval_sym(node->contents);
  }

  /* Anything else must be either the root node (>) or an S-Expression */
  lval_t *val = lval_sexpr();

  /* Add the children */
  for (int i = 0; i < node->children_num; i++) {
    mpc_ast_t *child = node->children[i];

    if (is_valid_expr(child)) {
      lval_add(val, ast_node_to_lval(child));
    }
  }

  return val;
}

int is_valid_expr(mpc_ast_t *node) {
  if (strcmp(node->contents, "(") == 0) { return 0; }
  if (strcmp(node->contents, ")") == 0) { return 0; }
  if (strcmp(node->contents, "{") == 0) { return 0; }
  if (strcmp(node->contents, "}") == 0) { return 0; }
  if (strcmp(node->tag, "regex") == 0)  { return 0; }

  return 1;
}

void lval_print(lval_t *val) {
  switch (val->type) {
    case LVAL_NUM:   printf("%li", val->num);        break;
    case LVAL_ERR:   printf("Error: %s", val->err);  break;
    case LVAL_SYM:   printf("%s", val->sym);         break;
    case LVAL_SEXPR: lval_expr_print(val, '(', ')'); break;
  }
}

void lval_expr_print(lval_t *val, char open, char close) {
  putchar(open);

  for (int i = 0; i < val->count; i++) {
    lval_print(val->cell[i]);

    /* Print space between elements */
    if (i != (val->count - 1)) {
      putchar(' ');
    }
  }

  putchar(close);
}

lval_t *lval_eval_sexpr(lval_t *val) {
  /* Evaluate children */
  for (int i = 0; i < val->count; i++) {
    val->cell[i] = lval_eval(val->cell[i]);

    /* Check for errors */
    if (val->cell[i]->type == LVAL_ERR) {
      return lval_take(val, i);
    }
  }

  /* Empty expressions */
  if (val->count == 0) {
    return val;
  }

  /* Single expressions */
  if (val->count == 1) {
    return lval_take(val, 0);
  }

  /* Ensure first element is a symbol */
  lval_t *first = lval_pop(val, 0);
  if (first->type != LVAL_SYM) {
    lval_del(first);
    lval_del(val);
    return lval_err("S-Expression must begin with a symbol!");
  }

  lval_t *result = builtin_op(val, first->sym);
  lval_del(first);
  return result;
}

lval_t *lval_eval(lval_t *val) {
  /* Only S-Expressions must be evaluated */
  if (val->type != LVAL_SEXPR) {
    return val;
  }

  return lval_eval_sexpr(val);
}

lval_t *builtin_op(lval_t *val, char *op) {
  /* Ensure all arguments are numbers */
  for (int i = 0; i < val->count; i++) {
    if (val->cell[i]->type != LVAL_NUM) {
      lval_del(val);
      return lval_err("Cannot operate on non-number!");
    }
  }

  lval_t *computedVal = lval_pop(val, 0);

  /* If no arguments and operation is subtraction, simply negate the number */
  if ((strcmp(op, "-") == 0) && val->count == 0) {
    computedVal->num = (0 - computedVal->num);
  }

  while (val->count > 0) {
    lval_t *nextArg = lval_pop(val, 0);

    if (strcmp(op, "min") == 0) { computedVal->num = MIN(computedVal->num, nextArg->num); }
    if (strcmp(op, "max") == 0) { computedVal->num = MAX(computedVal->num, nextArg->num); }

    if (strcmp(op, "+") == 0) { computedVal->num += nextArg->num; }
    if (strcmp(op, "-") == 0) { computedVal->num -= nextArg->num; }
    if (strcmp(op, "*") == 0) { computedVal->num *= nextArg->num; }

    if (strcmp(op, "/") == 0) {
      if (nextArg->num == 0) {
        lval_del(computedVal);
        lval_del(nextArg);
        computedVal = lval_err("Division By Zero!");
        break;
      }

      computedVal->num /= nextArg->num;
    }

    lval_del(nextArg);
  }

  lval_del(val);
  return computedVal;
}
