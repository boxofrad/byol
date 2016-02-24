#define LASSERT_NUM_ARGS(args, func, exp) \
  if ((args)->count != exp) { \
    char *msg; \
    asprintf(&msg, "Wrong number of arguments for function '%s' (%d for %d)", (func), (args)->count, (exp)); \
    lval_t *err = lval_err(msg); \
    free(msg); \
    lval_del((args)); \
    return err; \
  }

#define LASSERT_ARG_TYPE(args, func, idx, exp) \
  { \
    lval_type_t argType = (args)->cell[(idx)]->type; \
    if (argType != (exp)) { \
      char *expType = lval_type_desc((exp)); \
      char *gotType = lval_type_desc(argType); \
      char *msg; \
      asprintf(&msg, "Invalid type for argument %d to function '%s' (expected: '%s', got: '%s')", (idx), (func), expType, gotType); \
      lval_t *err = lval_err(msg); \
      free(expType); \
      free(gotType); \
      free(msg); \
      lval_del((args)); \
      return err; \
    } \
  }

#define LASSERT_NOT_EMPTY(args, func, idx) \
  if ((args)->cell[(idx)]->count == 0) { \
    char *msg; \
    asprintf(&msg, "Function '%s' cannot operate on empty lists, an empty list was found at argument %d", func, idx); \
    lval_t *err = lval_err(msg); \
    free(msg); \
    lval_del((args)); \
    return err; \
  }
