#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include "lval.h"

lval_t *lval_num(long num) {
  lval_t *val = malloc(sizeof(lval_t));
  val->type = LVAL_NUM;
  val->num = num;
  return val;
}

lval_t *lval_err(char *msg) {
  lval_t *val = malloc(sizeof(lval_t));
  val->type = LVAL_ERR;
  val->err = strdup(msg);
  return val;
}

lval_t *lval_sym(char *sym) {
  lval_t *val = malloc(sizeof(lval_t));
  val->type = LVAL_SYM;
  val->sym = strdup(sym);
  return val;
}

lval_t *lval_sexpr() {
  lval_t *val = malloc(sizeof(lval_t));
  val->type = LVAL_SEXPR;
  val->count = 0;
  val->cell = NULL;
  return val;
}

void lval_del(lval_t *val) {
  switch (val->type) {
    /* Number has nothing special to free */
    case LVAL_NUM: break;

    /* Errors and Symbols have strings we need to free */
    case LVAL_ERR: free(val->err); break;
    case LVAL_SYM: free(val->sym); break;

    /* S-Expressions have nested values we need to free */
    case LVAL_SEXPR:
      for (int i = 0; i < val->count; i++) {
        lval_del(val->cell[i]);
      }

      /* Also free the memory allocated to the pointers */
      free(val->cell);
      break;
  }

  free(val);
}

void lval_add(lval_t *dest, lval_t *src) {
  dest->count++;
  dest->cell = realloc(dest->cell, sizeof(lval_t *) * dest->count);
  dest->cell[dest->count - 1] = src;
}

lval_t *lval_pop(lval_t *val, int i) {
  lval_t *child = val->cell[i];

  /* Shift memory after the child backwards to close the gap in the list */
  memmove(&val->cell[i], &val->cell[i + 1],
      sizeof(lval_t *) * (val->count - i - 1));

  val->count--;
  val->cell = realloc(val->cell, sizeof(lval_t *) * val->count);

  return child;
}

lval_t *lval_take(lval_t *val, int i) {
  lval_t *child = lval_pop(val, i);
  lval_del(val);
  return child;
}
