struct lval;
struct lenv;
typedef struct lval lval_t;
typedef struct lenv lenv_t;

typedef enum {
  LVAL_ERR,
  LVAL_NUM,
  LVAL_SYM,
  LVAL_FUN,
  LVAL_SEXPR,
  LVAL_QEXPR
} lval_type_t;

typedef lval_t*(*lbuiltin)(lenv_t*, lval_t*);

struct lval {
  lval_type_t type;

  long num;
  char *err;
  char *sym;
  lbuiltin builtin;

  int count;
  struct lval **cell; /* TODO: Use a linked-list */
};

/* TODO: Use an actual hash */
struct lenv {
  int count;
  char **syms;
  lval_t **vals;
};

lval_t *lval_num(long num);
lval_t *lval_err(char *msg);
lval_t *lval_sym(char *sym);
lval_t *lval_fun(lbuiltin fun);
lval_t *lval_sexpr();
lval_t *lval_qexpr();

void lval_add(lval_t *dest, lval_t *src);
void lval_del(lval_t *val);

lval_t *lval_take(lval_t *val, int i);
lval_t *lval_pop(lval_t *val, int i);
lval_t *lval_join(lval_t *a, lval_t *b);
lval_t *lval_copy(lval_t *old);

char *lval_type_desc(lval_type_t type);

lenv_t *lenv_new();
lval_t *lenv_get(lenv_t *env, lval_t *key);
void lenv_del(lenv_t *env);
void lenv_put(lenv_t *env, lval_t *key, lval_t *val);
