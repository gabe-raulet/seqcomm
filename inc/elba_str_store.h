#ifndef ELBA_STR_STORE_H_
#define ELBA_STR_STORE_H_

#include "elba_str.h"
#include <string.h>

typedef struct
{
    elba_str_t buf;
    size_t *displs;
    size_t size, mem;
} elba_str_store_t;

#define ELBA_STR_STORE_INIT (elba_str_store_t){0}

int elba_str_store_pushl(elba_str_store_t *store, char const *s, size_t len);
int elba_str_store_get_len(elba_str_store_t const store, size_t id, size_t *len);
int elba_str_store_get_maxlen(elba_str_store_t const store, size_t *maxlen);
int elba_str_store_get_strcpy(elba_str_store_t const store, size_t id, char *s);
int elba_str_store_get_strdup(elba_str_store_t const store, size_t id, char **s);
int elba_str_store_free(elba_str_store_t *store);

#define elba_str_store_push(store, s) elba_str_store_pushl((store), (s), strlen((s)))

#endif
