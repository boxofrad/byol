#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"
#include "lval.h"
#include "assertions.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

lval_t *ast_node_to_lval(mpc_ast_t *node);
int is_valid_expr(mpc_ast_t *node);

void lenv_add_builtin(lenv_t *env, char *name, lbuiltin func);
void lenv_add_builtins(lenv_t *env);

lval_t *lval_eval_sexpr(lenv_t *env, lval_t *val);
lval_t *lval_eval(lenv_t *env, lval_t *val);

lval_t *builtin_func(lenv_t *env, lval_t *val, char *symbol);
lval_t *builtin_op(lenv_t *env, lval_t *val, char *op);

lval_t *builtin_add(lenv_t *env, lval_t *val);
lval_t *builtin_sub(lenv_t *env, lval_t *val);
lval_t *builtin_mul(lenv_t *env, lval_t *val);
lval_t *builtin_div(lenv_t *env, lval_t *val);

lval_t *builtin_head(lenv_t *env, lval_t *val);
lval_t *builtin_tail(lenv_t *env, lval_t *val);
lval_t *builtin_list(lenv_t *env, lval_t *val);
lval_t *builtin_eval(lenv_t *env, lval_t *val);
lval_t *builtin_join(lenv_t *env, lval_t *val);
lval_t *builtin_cons(lenv_t *env, lval_t *val);
lval_t *builtin_len(lenv_t *env, lval_t *val);
lval_t *builtin_init(lenv_t *env, lval_t *val);
lval_t *builtin_def(lenv_t *env, lval_t *val);

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
  mpc_parser_t *Qexpr   = mpc_new("qexpr");
  mpc_parser_t *Expr    = mpc_new("expr");
  mpc_parser_t *Program = mpc_new("program");

  mpca_lang(MPCA_LANG_DEFAULT,
      "                                                     \
        number  : /-?[0-9]+/ ;                              \
        symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;        \
        sexpr   : '(' <expr>* ')' ;                         \
        qexpr   : '{' <expr>* '}' ;                         \
        expr    : <number> | <symbol> | <sexpr> | <qexpr>;  \
        program : /^/ <expr>* /$/ ;                         \
      ",
      Number, Symbol, Sexpr, Qexpr, Expr, Program
  );

  lenv_t *env = lenv_new();
  lenv_add_builtins(env);

  while (1) {
    char *input = readline("byol> ");
    add_history(input);

    mpc_result_t result;
    if (mpc_parse("<stdin>", input, Program, &result)) {
      lval_t *program = ast_node_to_lval(result.output);
      lval_t *computedResult = lval_eval(env, program);

      lval_print(computedResult);
      putchar('\n');
      lval_del(computedResult);

      /*
      puts("\n\n=== Abstract Syntax Tree ===");

      printf(
          "Leaves: %d, Branches: %d, Most Children: %d\n",
          num_leaves(result.output),
          num_branches(result.output),
          most_children(result.output)
      );

      mpc_ast_print(result.output);
      */

      mpc_ast_delete(result.output);
    } else {
      mpc_err_print(result.error);
      mpc_err_delete(result.error);
    }

    free(input);
  }

  lenv_del(env);

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Program);

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

  lval_t *val;

  if (strstr(node->tag, "qexpr")) {
    val = lval_qexpr();
  } else {
    /* Anything else must be either the root node (>) or an S-Expression */
    val = lval_sexpr();
  }

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
    case LVAL_FUN:   printf("<function>");           break;
    case LVAL_SEXPR: lval_expr_print(val, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(val, '{', '}'); break;
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

lval_t *lval_eval_sexpr(lenv_t *env, lval_t *val) {
  /* Evaluate children */
  for (int i = 0; i < val->count; i++) {
    val->cell[i] = lval_eval(env, val->cell[i]);

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

  /* Ensure first element is a function */
  lval_t *first = lval_pop(val, 0);
  if (first->type != LVAL_FUN) {
    lval_del(first);
    lval_del(val);
    return lval_err("first element is not a function");
  }

  lval_t *result = first->builtin(env, val);
  lval_del(first);
  return result;
}

lval_t *lval_eval(lenv_t *env, lval_t *val) {
  if (val->type == LVAL_SYM) {
    lval_t *resolvedVal = lenv_get(env, val);
    lval_del(val);
    return resolvedVal;
  }

  if (val->type == LVAL_SEXPR) {
    return lval_eval_sexpr(env, val);
  }

  return val;
}

lval_t *builtin_func(lenv_t *env, lval_t *val, char *symbol) {
  if (strcmp("list", symbol) == 0) { return builtin_list(env, val); }
  if (strcmp("head", symbol) == 0) { return builtin_head(env, val); }
  if (strcmp("tail", symbol) == 0) { return builtin_tail(env, val); }
  if (strcmp("join", symbol) == 0) { return builtin_join(env, val); }
  if (strcmp("eval", symbol) == 0) { return builtin_eval(env, val); }
  if (strcmp("cons", symbol) == 0) { return builtin_cons(env, val); }
  if (strcmp("len", symbol) == 0)  { return builtin_len(env, val);  }
  if (strcmp("init", symbol) == 0) { return builtin_init(env, val); }

  if (strcmp("min", symbol) == 0 || strcmp("max", symbol) == 0) {
    return builtin_op(env, val, symbol);
  }

  if (strstr("+-/*^", symbol)) {
    return builtin_op(env, val, symbol);
  }

  lval_del(val);

  /* TODO: this error message should say which function was unknown */
  return lval_err("Unknown function!");
}

lval_t *builtin_add(lenv_t *env, lval_t *val) {
  return builtin_op(env, val, "+");
}

lval_t *builtin_sub(lenv_t *env, lval_t *val) {
  return builtin_op(env, val, "-");
}

lval_t *builtin_mul(lenv_t *env, lval_t *val) {
  return builtin_op(env, val, "*");
}

lval_t *builtin_div(lenv_t *env, lval_t *val) {
  return builtin_op(env, val, "/");
}

lval_t *builtin_op(lenv_t *env, lval_t *val, char *op) {
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

lval_t *builtin_head(lenv_t *env, lval_t *val) {
  LASSERT_NUM_ARGS(val, "head", 1);
  LASSERT_ARG_TYPE(val, "head", 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY(val, "head", 0);

  lval_t *qexpr = lval_take(val, 0);

  /* Delete all but the first argument */
  while (qexpr->count > 1) {
    lval_del(lval_pop(qexpr, 1));
  }

  return qexpr;
}

lval_t *builtin_tail(lenv_t *env, lval_t *val) {
  LASSERT_NUM_ARGS(val, "tail", 1);
  LASSERT_ARG_TYPE(val, "tail", 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY(val, "tail", 0);

  lval_t *qexpr = lval_take(val, 0);

  /* Delete the first argument */
  lval_del(lval_pop(qexpr, 0));

  return qexpr;
}

/* TODO: should this only operate on S-Expessions? */
lval_t *builtin_list(lenv_t *env, lval_t *val) {
  val->type = LVAL_QEXPR;
  return val;
}

lval_t *builtin_eval(lenv_t *env, lval_t *val) {
  LASSERT_NUM_ARGS(val, "eval", 1);
  LASSERT_ARG_TYPE(val, "eval", 0, LVAL_QEXPR);

  lval_t *expr = lval_take(val, 0);
  expr->type = LVAL_SEXPR;
  return lval_eval(env, expr);
}

lval_t *builtin_join(lenv_t *env, lval_t *val) {
  for (int i = 0; i < val->count; i++) {
    LASSERT_ARG_TYPE(val, "join", i, LVAL_QEXPR);
  }

  lval_t *qexpr = lval_pop(val, 0);
  while (val->count > 0) {
    qexpr = lval_join(qexpr, lval_pop(val, 0));
  }

  lval_del(val);
  return qexpr;
}

lval_t *builtin_cons(lenv_t *env, lval_t *val) {
  LASSERT_NUM_ARGS(val, "cons", 2);
  LASSERT_ARG_TYPE(val, "cons", 0, LVAL_QEXPR);

  /* Create a list containing the second arg */
  lval_t *qexpr = lval_qexpr();
  lval_add(qexpr, val->cell[1]);

  /* Join the new list to the front of the first arg */
  return lval_join(qexpr, val->cell[0]);
}

lval_t *builtin_len(lenv_t *env, lval_t *val) {
  LASSERT_NUM_ARGS(val, "len", 1);
  LASSERT_ARG_TYPE(val, "len", 0, LVAL_QEXPR);

  lval_t *len = lval_num(val->cell[0]->count);
  lval_del(val);
  return len;
}

lval_t *builtin_init(lenv_t *env, lval_t *val) {
  LASSERT_NUM_ARGS(val, "init", 1);
  LASSERT_ARG_TYPE(val, "init", 0, LVAL_QEXPR);

  lval_t *qexpr = lval_take(val, 0);

  if (qexpr->count > 0) {
    lval_del(lval_pop(qexpr, qexpr->count - 1));
  }

  return qexpr;
}

lval_t *builtin_def(lenv_t *env, lval_t *val) {
  LASSERT_ARG_TYPE(val, "def", 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY(val, "def", 0);

  /* Ensure all items in first argument are symbols */
  lval_t *symbols = val->cell[0];
  for (int i = 0; i < symbols->count; i++) {
    LASSERT(val, symbols->cell[i]->type == LVAL_SYM,
        "Function 'def' cannot define non-symbol");
  }

  /* Check number of symbols matches number of values */
  LASSERT(val, symbols->count == (val->count - 1),
      "Number of values must match number of symbols in 'def'");

  /* Store the values in the environment */
  for (int i = 0; i < symbols->count; i++) {
    lenv_put(env, symbols->cell[i], val->cell[i + 1]);
  }

  lval_del(val);
  return lval_sexpr();
}

void lenv_add_builtin(lenv_t *env, char *name, lbuiltin fun) {
  lval_t *key = lval_sym(name);
  lval_t *val = lval_fun(fun);
  lenv_put(env, key, val);
  lval_del(key);
  lval_del(val);
}

void lenv_add_builtins(lenv_t *env) {
  lenv_add_builtin(env, "list", builtin_list);
  lenv_add_builtin(env, "head", builtin_head);
  lenv_add_builtin(env, "tail", builtin_tail);
  lenv_add_builtin(env, "eval", builtin_eval);
  lenv_add_builtin(env, "join", builtin_join);

  lenv_add_builtin(env, "+", builtin_add);
  lenv_add_builtin(env, "-", builtin_sub);
  lenv_add_builtin(env, "*", builtin_mul);
  lenv_add_builtin(env, "/", builtin_div);

  lenv_add_builtin(env, "def", builtin_def);
}
