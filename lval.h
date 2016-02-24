typedef enum {
  LVAL_ERR,
  LVAL_NUM,
  LVAL_SYM,
  LVAL_SEXPR,
  LVAL_QEXPR
} lval_type_t;

typedef struct lval {
  lval_type_t type;
  long num;
  char *err;
  char *sym;
  int count;
  struct lval **cell;
} lval_t;

lval_t *lval_num(long num);
lval_t *lval_err(char *msg);
lval_t *lval_sym(char *sym);
lval_t *lval_sexpr();
lval_t *lval_qexpr();

void lval_add(lval_t *dest, lval_t *src);
void lval_del(lval_t *val);

lval_t *lval_take(lval_t *val, int i);
lval_t *lval_pop(lval_t *val, int i);
lval_t *lval_join(lval_t *a, lval_t *b);

char *lval_type_desc(lval_type_t type);
